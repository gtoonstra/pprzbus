/*
 *
 *	Pprzbus, C interface
 *
 *	Copyright 1997-2008
 *	Centre d'Etudes de la Navigation Aerienne
 *
 *	Main functions
 *
 *	Authors: Francois-Regis Colin,Stephane Chatty, Alexandre Bustico
 *
 *	$Id$
 *
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 */

/*
  TODO :  ° faire un configure
*/

#ifdef OPENMP
#include <omp.h>
#endif

#include <stdlib.h>
#ifdef WIN32
#include <Ws2tcpip.h>
#include <windows.h>
#include "timer.h"
#define snprintf _snprintf
#ifdef __MINGW32__
// should be removed in when defined in MinGW include of ws2tcpip.h
extern const char * WSAAPI inet_ntop(int af, const void *src,
                             char *dst, socklen_t size);
extern int WSAAPI inet_pton(int af, const char *src, void *dst);

#endif
#else
#include <sys/time.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include <fcntl.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include "version.h"
#include "pprzbuschannel.h"
#include "list.h"
#include "pprzbusbuffer.h"
#include "pprzbusdebug.h"
#include "pprzbus.h"

#define ARG_START "\002"
#define ARG_END "\003"

#ifdef __APPLE__
#define DEFAULT_DOMAIN 127.0.0.1
#else
#define DEFAULT_DOMAIN 127.255.255.255
#endif

#define IVY_DEFAULT_BUS 2010
#define MAX_MATCHING_ARGS 120

/* stringification et concatenation du domaine et du port en 2 temps :
 * Obligatoire puisque la substitution de domain, et de bus n'est pas
 * effectuée si on stringifie directement dans la macro GenerateIvyBus */
#define str(bus) #bus
#define GenerateIvyBus(domain,bus) str(domain)":"str(bus)
#define       MIN(a, b)   ((a) > (b) ? (b) : (a))

static const char* DefaultIvyBus = GenerateIvyBus(DEFAULT_DOMAIN,IVY_DEFAULT_BUS);

typedef enum {
	Bye,			/* l'application emettrice se termine */
	AddRegexp,		/* expression reguliere d'un client */
	Msg,			/* message reel */
	Error,			/* error message */
	DelRegexp,		/* Remove expression reguliere */
	EndRegexp,		/* end of the regexp list */
	StartRegexp,		/* debut des expressions */
	DirectMsg,		/* message direct a destination de l'appli */
	Die,			/* demande de terminaison de l'appli */
	Ping,			/* message de controle ivy */
	Pong			/* ivy doit renvoyer ce message à la reception d'un ping */
} MsgType;	


typedef struct _msg_snd_dict	*MsgSndDictPtr;
typedef struct _global_reg_lst	*GlobRegPtr;


struct _msg_rcv {			/* requete d'emission d'un client */
	MsgRcvPtr next;
	int id;
	MsgCallback callback;		/* callback a declancher a la reception */
	void *user_data;		/* stokage d'info client */
    int groupcount;
    char *srcfilter;
};

/* liste de regexps source */
struct _global_reg_lst {		/* liste des regexp source */
	GlobRegPtr next;
	char *str_regexp;		/* la regexp sous forme source */
  	int id;                         /* son id, differente pour chaque client */
};


/* liste de clients, champ de la struct _msg_snd_dict qui est valeur du dictionnaire */
/* typedef IvyClientPtr */

struct _ping_timestamp {
  struct timeval ts;
  int		 id;
};

/* flag pour le debug en cas de Filter de regexp */
int debug_filter = 0;

/* flag pour le debug en cas de Filter de regexp */
int debug_binary_msg = 0;

/* numero de port TCP en mode serveur */

/* numero de port UDP */
static unsigned short SupervisionPort;

static const char *ApplicationName = NULL;

/* callback appele sur reception d'un message direct */
static MsgDirectCallback direct_callback = NULL;
static  void *direct_user_data = NULL;

/* callback appele sur changement d'etat d'application */
static IvyApplicationCallback application_callback;
static void *application_user_data = NULL;

/* callback appele sur ajout suppression de regexp */
static IvyBindCallback application_bind_callback;
static void *application_bind_data = NULL;

/* callback appele sur demande de terminaison d'application */
static IvyDieCallback application_die_callback;
static void *application_die_user_data = NULL;

/* callback appele sur reception d'une trame PONG */
static IvyPongCallback application_pong_callback = NULL;


/* liste des messages a recevoir */
static MsgRcvPtr msg_recv = NULL;

extern redisAsyncContext *sub_ac;
extern redisAsyncContext *pub_ac;

static const char *ready_message = NULL;

#define MAXPORT(a,b)      ((a>b) ? a : b)

static int extract_capitals( const char *data, char *token, int *groupsBeforeName ) 
{
    int i;
    int count = 0;
    int numseenbrackets = 0;
    int counting = 0;

    for ( i = 0; i < strlen(data); i++ ) {
        if ( data[ i ] == '(' ) {
            if ( count > 0 ) {
                numseenbrackets = -1;
            } else {
                numseenbrackets++;
            }
            if ( numseenbrackets != -1 ) {
                *groupsBeforeName = *groupsBeforeName + 1;
            }
        }
        if ( (( isupper( data[ i ] )) || (data[i] == '_')) && (numseenbrackets > 0) ) {
            token[count] = data[i];
            count++;
        }
    }
    token[count] = '*';
    count++;
    return count;
}

static char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = reply;
    int j;
	int argc = 0;
	char *argv[MAX_MATCHING_ARGS];
    char channel[128] = {"\0"};
    char *procname, *msgname;
    char *saveptr;
    char *token;
    char data[1024] = {"\0"};

    if (reply == NULL) return;

    if ( strcasecmp( r->element[0]->str, "PSUBSCRIBE" ) == 0 ) {
        // don't bubble up the psubscribe reply
        return;
    }

    if (r->type == REDIS_REPLY_ARRAY) {
        // 0 = pmessage
        // 1 = subscribed channel name (AIRCRAFTS*)
        // 2 = actual msg name
        // 3 = data        
        strcpy( channel, r->element[2]->str );
        msgname = strtok_r( channel, ".", &saveptr );
        procname = strtok_r( NULL, ".", &saveptr );

        // ivy wants procname (or id), then msgname and sometimes something in between
        argv[argc++] = procname;
        argv[argc++] = procname;

        strcpy( data, msgname );
        strcat( data, " " );
        strcat( data, r->element[3]->str+1);
        data[strlen(data) - 1 ] = '\0';
        trimwhitespace( data );

        /*        
        saveptr = NULL;
        token = strtok_r( data, " ", &saveptr );
        if ( token != NULL ) {
            argv[argc++] = token;
            while (( token = strtok_r( NULL, " ", &saveptr ) ) != NULL ) {
                argv[argc++] = token;
            }
        }
        */

        if ( privdata ) {
            MsgRcvPtr msg = (MsgRcvPtr)privdata;
            argv[msg->groupcount-1] = data;
            argc = msg->groupcount;

            // printf( "Returning '%s' for message '%s'\n", argv[argc-1], msgname );
            if ( msg->srcfilter != NULL ) {
                if ( strcasecmp( msg->srcfilter, procname ) == 0 ) {
                    (*msg->callback)( NULL, msg->user_data, argc, argv ) ;
                }
            } else {
                (*msg->callback)( NULL, msg->user_data, argc, argv ) ;
            }
        }
    }
}

void onPublish(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = reply;
    int j;

    if (reply == NULL) return;

    // assume all went well...
/*
    if (r->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < r->elements; j++) {
            printf("%u) %s\n", j, r->element[j]->str);
        }
    }
*/
}

void IvyInit (const char *appname, const char *ready, 
			 IvyApplicationCallback callback, void *data,
			 IvyDieCallback die_callback, void *die_data
			 )
{
	if ( appname )
		ApplicationName = strdup(appname);

    printf("I'm called %s\n", ApplicationName );

	application_callback = callback;
	application_user_data = data;
	application_die_callback = die_callback;
	application_die_user_data = die_data;
	if ( ready )
		ready_message = strdup(ready);

    // GT: Initialization goes here, but for redis that's done in IvyStart.
	IvyChannelInit();

	if ( getenv( "IVY_DEBUG_BINARY" )) debug_binary_msg = 1;
}

void IvyTerminate()
{
    printf( "IvyTerminate\n" );
	if ( ApplicationName )
	  free((void *) ApplicationName );
	if ( ready_message )
	  free((void *) ready_message );
    
    // GT: Add termination code here..
     /* Disconnects and frees the context */

}

void IvySetBindCallback( IvyBindCallback bind_callback, void *bind_data )
{
    printf( "IvySetBindCallback\n" );
  application_bind_callback=bind_callback;
  application_bind_data=bind_data;
}

void IvySetPongCallback( IvyPongCallback pong_callback )
{
    printf( "IvySetPongCallback\n" );
  application_pong_callback = pong_callback;
}

void IvySetFilter( int argc, const char **argv)
{
    // This adds all words from argv as filters...
    // GT: Do the same?
    printf( "IvySetFilter\n" );
	if ( getenv( "IVY_DEBUG_FILTER" )) debug_filter = 1;

}
void IvyAddFilter( const char *arg)
{
    // GT: Adds a filter
    printf( "IvyAddFilter\n" );
	if ( getenv( "IVY_DEBUG_FILTER" )) debug_filter = 1;

}
void IvyRemoveFilter( const char *arg)
{
    printf( "IvyRemoveFilter\n" );
}

void IvyStop (void)
{
    printf( "IvyStop\n" );
	IvyChannelStop();
}


void IvyStart (const char* bus)
{
	const char* p = bus;	/* used for decoding address list */
	const char* q;			/* used for decoding port number */
	char addr[1024];	/* used for decoding addr */
	unsigned short port=0;

    printf( "IvyStart\n" );
	
	/*
	 * Find network list as well as broadcast port
	 * (we accept things like 123.231,123.123:2000 or 123.231 or :2000),
	 * Initialize UDP port
	 * Send a broadcast handshake on every network
	 */

	/* first, let's find something to parse */
	if (!p || !*p)
		p = getenv ("IVYBUS");
	if (!p || !*p) 
		p = DefaultIvyBus;

	/* then, let's get a port number */
	q = strrchr (p, ':');
	if (q && (port = atoi (q+1)))
	{
		SupervisionPort = port;
		strncpy( addr, p, q-p );
		addr[q-p] ='\0';
	}
	else
		SupervisionPort = IVY_DEFAULT_BUS;

	/* then, if we only have a port number, resort to default value for network */
	if (p == q)
		p = DefaultIvyBus;

    // ---- specific code ----
    // GT: Initialize pprzbus here...
    // initialization is done in each *loop.c file.
}

/* desabonnements */
void
IvyUnbindMsg (MsgRcvPtr msg)
{
    // GT: Unbind from message here
    printf( "IvyUnbindMsg\n" );
}

/* demande de reception d'un message */

MsgRcvPtr
IvyBindMsg ( const char *srcFilter, MsgCallback callback, void *user_data, const char *fmt_regex, ... )
{
    printf( "IvyBindMsg\n" );

	static IvyBuffer buffer = { NULL, 0, 0};
	va_list ap;
	static int recv_id = 0;
	MsgRcvPtr msg;
    char token[1024] = {"\0"};
    int groupCount = 0;

	va_start (ap, fmt_regex );
	buffer.offset = 0;
	make_message( &buffer, fmt_regex, ap );
	va_end  (ap );

	/* add Msg to the query list */
	IVY_LIST_ADD_START( msg_recv, msg )
		msg->id = recv_id++;
		msg->callback = callback;
		msg->user_data = user_data;
        if (( srcFilter != NULL ) && ( strlen( srcFilter ) > 0 )) {
            msg->srcfilter = strdup(srcFilter);
        } else {
            msg->srcfilter = NULL;
        }
	IVY_LIST_ADD_END( msg_recv, msg )

    printf( "buffer.data = %s\n", buffer.data );

    // ^([0-9]+\.[0-9]+ )?([^ ]*) +(NEW_AIRCRAFT( .*|$))
    int count = extract_capitals( buffer.data, token, &groupCount );
    if ( count > 0 ) {
        printf( "Pattern subscribing to %s, groupcount: %d\n", token, groupCount );
        msg->groupcount = groupCount;

        // GT: bind msg.    
        redisAsyncCommand(sub_ac, onMessage, msg, "PSUBSCRIBE %s", token );
    }

	return msg;
}


int IvySendMsg(const char *fmt, ...) /* version dictionnaire */
{
  int match_count = 0;

  static IvyBuffer buffer = { NULL, 0, 0}; /* Use static mem to eliminate multiple call to malloc /free */
  va_list ap;
  char *procname = NULL;
  char *msgname = NULL;
  char *procid = 0;
  char channel[128] = {"\0"};
  char data[1024] = {"\0"};
  char *saveptr;
  char *token;

  /* construction du buffer message à partir du format et des arguments */
  if( fmt == 0 || strlen(fmt) == 0 ) return 0;	

  va_start( ap, fmt );
  buffer.offset = 0;
  make_message( &buffer, fmt, ap );
  va_end ( ap );

  // ^11553_1 ([^ ]*) +(AIRCRAFTS.*)
  // or
  // ^1 AIRCRAFT_DATA "something"
  procid = strtok_r( buffer.data, " ", &saveptr );
  procname = strtok_r( NULL, " ", &saveptr );
  msgname = strtok_r( NULL, " ", &saveptr );
  if ( isupper( procname[0] ) ) {
     strcpy( data, msgname );
     msgname = procname;
     procname = procid;
  }

  while (( token = strtok_r( NULL, " ", &saveptr ) ) != NULL ) {
     if ( data[0] != 0 ) {
         strcat( data, " " );
     }
     strcat( data, token );
  }

  sprintf( channel, "%s.%s", msgname, procname );

  // GT: Send msg
  redisAsyncCommand(pub_ac, onPublish, NULL, "PUBLISH %s \"%s\"", channel, data );

  return match_count;
}


void IvySendError(IvyClientPtr app, int id, const char *fmt, ... )
{
	static IvyBuffer buffer = { NULL, 0, 0}; /* Use static mem to eliminate multiple call to malloc /free */
	va_list ap;
	
	va_start( ap, fmt );
	buffer.offset = 0;
	make_message( &buffer, fmt, ap );
	va_end ( ap );

    // GT: Send error?  Add error count?
}

void IvyBindDirectMsg( MsgDirectCallback callback, void *user_data)
{
	direct_callback = callback;
	direct_user_data = user_data;
}

void IvySendDirectMsg(IvyClientPtr app, int id, char *msg )
{
    // GT: Send direct msg?
}

void IvySendPing( IvyClientPtr app)
{
  if (application_pong_callback != NULL) {
       // GT: Send ping?
  } else {
    fprintf(stderr,"Application: %s useless IvySendPing issued since no pong callback defined\n",
	    IvyGetApplicationName( app ));
  }
}

void IvySendDieMsg(IvyClientPtr app )
{
    // GT: Send die msg? how?
}

const char *IvyGetApplicationName(IvyClientPtr app )
{
    return "Unknown";
}

const char *IvyGetApplicationHost(IvyClientPtr app )
{
    return 0;
}

void IvyDefaultApplicationCallback(IvyClientPtr app, void *user_data, IvyApplicationEvent event)
{
	switch ( event )  {
	case IvyApplicationConnected:
		printf("Application: %s ready on %s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app));
		break;
	case IvyApplicationDisconnected:
		printf("Application: %s bye on %s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app));
		break;
	case IvyApplicationCongestion:
		printf("Application: %s congestion on %s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app));
		break;
	case IvyApplicationDecongestion:
		printf("Application: %s  decongestion on %s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app));
		break;
	case IvyApplicationFifoFull:
		printf("Application: %s  fifo full, msg on %s will be lost until decongestion\n", 
		 IvyGetApplicationName( app ), IvyGetApplicationHost(app));
		break;
	default:
		printf("Application: %s unkown event %d\n",IvyGetApplicationName( app ), event);
		break;
	}
}

void IvyDefaultBindCallback(IvyClientPtr app, void *user_data, int id, const char* regexp,  IvyBindEvent event)
{
	switch ( event )  {
	case IvyAddBind:
		printf("Application: %s on %s add regexp %d : %s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app), id, regexp);
		break;
	case IvyRemoveBind:
		printf("Application: %s on %s remove regexp %d :%s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app), id, regexp);
		break;
	case IvyFilterBind:
		printf("Application: %s on %s as been filtred regexp %d :%s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app), id, regexp);
		break;
	case IvyChangeBind:
	        printf("Application: %s on %s change regexp %d : %s\n", IvyGetApplicationName( app ), IvyGetApplicationHost(app), id, regexp);
		break;
		break;
	default:
		printf("Application: %s unkown event %d\n",IvyGetApplicationName( app ), event);
		break;
	}
}

IvyClientPtr IvyGetApplication( char *name )
{
	IvyClientPtr app = 0;
	return app;
}

char *IvyGetApplicationList(const char *sep)
{
	static char applist[4096]; /* TODO remove that ugly Thing */
	applist[0] = '\0';
	return applist;
}

char **IvyGetApplicationMessages( IvyClientPtr app )
{
#define MAX_REGEXP 4096
	static char *messagelist[MAX_REGEXP+1];/* TODO remove that ugly Thing */
	memset( messagelist, 0 , sizeof( messagelist ));
	return messagelist;
}

