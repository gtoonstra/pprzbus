/*
 *	Ivy, C interface
 *
 *	Copyright 1997-2000
 *	Centre d'Etudes de la Navigation Aerienne
 *
 *	Sockets
 *
 *	Authors: Francois-Regis Colin <fcolin@cena.dgac.fr>
 *
 *	$Id$
 *
 *	Please refer to file version.h for the
 *	copyright notice regarding this software
 */

#ifndef PPRZBUSBUFFER_H
#define PPRZBUSBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
typedef struct {
	char * data; 
	int size;
	int offset;
	} IvyBuffer;


/* utility fonction do make vsprintf without buffer limit */
extern int make_message(IvyBuffer * buffer, const char *fmt, va_list ap);
extern int make_message_var(IvyBuffer* buffer, const char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif

