#
#	PPrzbus, C interface
#
#	Copyright (C) 1997-2002
#	Centre d'Études de la Navigation Aérienne
#
#	Makefile
#
#	Authors: François-Régis Colin <fcolin@cena.fr>
#		 Stéphane Chatty <chatty@cena.fr>
#		 Yannick Jestin <jestin@cena.fr>
#
#	Please refer to file version.h for the
#	copyright notice regarding this software
#


# change this in version.h too !!!!
MAJOR=1
MINOR=0

PERHAPS64 := $(shell getconf LONG_BIT | perl -ne "print /64/ ? '64' : '';")
# LIB = /lib$(PERHAPS64)
LIB = /lib

# by default use fPIC on all systems
FPIC ?= -fPIC

ifndef PREFIX
	export PREFIX=/usr/local
endif

X11_PREFIX:= $(shell pkg-config --variable=prefix x11)
ifndef X11_PREFIX
	X11_PREFIX = /usr/X11R6
endif

# Export all variables (Petr Mejzlik)
export

XTINC = -I$(X11_PREFIX)/include
XTLIB = -L$(X11_PREFIX)$(LIB) -lXt -lX11 -lSM -lICE
HAVE_GLIB = $(shell pkg-config --exists glib-2.0 && echo 'yes')
GLIBINC = `pkg-config --cflags glib-2.0`
GLIBLIB = `pkg-config --libs glib-2.0`
GLUTINC = -I/usr/include -I.
GLUTLIB = -L. -lglut -lGLU -lGL -lX11 -lXmu

REDISINC = `pkg-config --cflags hiredis`
REDISLIB = `pkg-config --libs hiredis`
# use these if you want standard Tcl ( 8.3 on debian woody ... )

#TCLVERS = `perl -e '@_=sort map (m|/usr/lib/libtcl(\d\.\d)\.so|, glob ("/usr/lib/libtcl*")); print pop @_'`
TCLVERS:= $(shell perl -e '@_=sort map (m|/usr/lib/libtcl(\d\.\d)\.so|, glob ("/usr/lib/libtcl*")); print pop @_')
ifndef TCLVERS
TCLVERS:= $(shell perl -e '@_=sort map (m|/usr/lib64/libtcl(\d\.\d)\.so|, glob ("/usr/lib64/libtcl*")); print pop @_')
endif
ifndef TCLVERS
#TCLVERS=8.4
TCLVERS=
endif

TCLINCL = -I/usr/include/tcl$(TCLVERS)
TCLLIB = -ltcl$(TCLVERS)
#TCLINCL	= -I/usr/include/tcl8.4
#TCLLIB = -ltcl8.4
# use these if you want to use Aqua Tcl on macOSX
#TCLINC = -I/Library/Frameworks/Tcl.framework/Headers
#TCLLIB = -framework Tcl
PCREINC = $(shell pcre-config --cflags)
PCRELIB = $(shell pcre-config --libs)
CASESENSITIVE = "yes"
ifeq ($(CASESENSITIVE), "no")
	PCRE_OPT=PCRE_CASELESS
	REGCOMP_OPT=REG_ICASE
else
	PCRE_OPT=0
	REGCOMP_OPT=REG_EXTENDED
endif

#PCREOBJ = `pcre-config --prefix`/lib/libpcre.a
PCREOBJ =

ifeq ($(shell uname -s),SunOS)
	EXTRALIB=# for solaris
else
	EXTRALIB=
endif


export CC=gcc
export CPP='g++'
LIBTOOL=ar q # linux and solaris
#LIBTOOL=libtool -static -o

#REGEXP = -DGNU_REGEXP	-DREGCOMP_OPT=$(REGCOMP_OPT) # deprecated !
REGEXP= -DUSE_PCRE_REGEX -DPCRE_OPT=$(PCRE_OPT)
# on activeTCL , set #define CHANNEL to null, and add pprzbusloop.o in the pprzbustcl target,
# see below
CHANNEL = -DTCL_CHANNEL_INTEGRATION

# DEBUG
CFLAGS_DEBUG = -g -Wall -Wshadow -DDEBUG $(FPIC)
# RELEASE
CFLAGS_RELEASE = -O2 -Wall -Wshadow $(FPIC)

ifdef DEBUG
	export CFLAGS += $(CFLAGS_DEBUG)
	LIB:= $(LIB)/debug
else
	export CFLAGS += $(CFLAGS_RELEASE)
endif



BUGGY_DEBIAN_OPTION = -DPCRECPP_EXP_DEFN="" -DPCRECPP_EXP_DECL=""
OMPCFLAGS = $(CFLAGS) -fopenmp -DOPENMP=1 -D_REENTRANT
OMPLIB = -lgomp -lpthread

OBJ = pprzbusloop.o timer.o pprzbus.o pprzbusbuffer.o 
OMPOBJ = pprzbusloop.o timer.o pprzbusbuffer.o 
GOBJ = pprzbusloop.o timer.o gpprzbus.o pprzbusbuffer.o 
XTOBJ = pprzbusxtloop.o pprzbus.o pprzbusbuffer.o 
GLIBOBJ = pprzbusglibloop.o pprzbus.o pprzbusbuffer.o 
GLUTOBJ = pprzbusglutloop.o pprzbus.o pprzbusbuffer.o 
TCLOBJ = pprzbustcl.o  timer.o gpprzbus.o pprzbusbuffer.o 
# WINDOWS add pprzbusloop.o if TCL_CHANNEL_INTEGRATION is not set

TARGETLIBS=libpprzbus.so.$(MAJOR).$(MINOR) libgpprzbus.so.$(MAJOR).$(MINOR) libxtpprzbus.so.$(MAJOR).$(MINOR)  libtclpprzbus.so.$(MAJOR).$(MINOR)
STATICTARGETLIBS=libpprzbus.a libgpprzbus.a libxtpprzbus.a libtclpprzbus.a

ifeq ($(HAVE_GLIB),yes)
		TARGETLIBS += libglibpprzbus.so.$(MAJOR).$(MINOR)
		STATICTARGETLIBS += libglibpprzbus.a
endif


# not yet need Modified Glut pprzbusglutprobe

.c.o:
	$(CC) $(CFLAGS) -c $*.c
.cpp.o:
	$(CPP) $(CFLAGS) $(BUGGY_DEBIAN_OPTION) -c $*.cpp

all: static-libs shared-libs pkgconf  
libomp: libpprzbus_omp.a libpprzbus_omp.so.$(MAJOR).$(MINOR)
omp: libomp

static-libs:  $(STATICTARGETLIBS)
# not yet need Modified Glut libglutpprzbus.a

shared-libs: $(TARGETLIBS)
# not yet need Modified Glut libglutpprzbus.so.$(MAJOR).$(MINOR)


pprzbusbind.o: pprzbusbind.c
	$(CC) -c $(CFLAGS) $(REGEXP) $(PCREINC) pprzbusbind.c


pprzbustcl.o: pprzbustcl.c
	$(CC) -c $(CFLAGS) $(TCLINCL) $(CHANNEL) pprzbustcl.c

gpprzbus.o: pprzbus.c
	$(CC) -c $(CFLAGS) $(REDISINC) -DDEBUG -o gpprzbus.o pprzbus.c

pprzbus_omp.o: pprzbus.c
	$(CC) -c $(OMPCFLAGS)  -o pprzbus.c

pprzbussocket_omp.o: pprzbussocket.c
	$(CC) -c $(OMPCFLAGS)  -o pprzbussocket.c

pprzbusglutloop.o: pprzbusglutloop.c pprzbusglutloop.h
	$(CC) -c $(CFLAGS) $(GLUTINC) pprzbusglutloop.c

pprzbusglibloop.o: pprzbusglibloop.c pprzbusglibloop.h
	$(CC) -c $(CFLAGS) $(GLIBINC) pprzbusglibloop.c


libpprzbus.a: $(OBJ)
	rm -f $@
	$(LIBTOOL) $@ $(OBJ)

libpprzbus_omp.a: $(OMPOBJ)
	rm -f $@
	$(LIBTOOL) $@ $(OMPOBJ)

libgpprzbus.a: $(GOBJ)
	rm -f $@
	$(LIBTOOL) $@ $(GOBJ)

libxtpprzbus.a: $(XTOBJ)
	rm -f $@
	$(LIBTOOL) $@ $(XTOBJ)

libglibpprzbus.a: $(GLIBOBJ)
	rm -f $@
	$(LIBTOOL) $@ $(GLIBOBJ)

libglutpprzbus.a: $(GLUTOBJ)
	rm -f $@
	$(LIBTOOL) $@ $(GLUTOBJ)

libtclpprzbus.a: $(TCLOBJ)
	rm -f $@
	$(LIBTOOL) $@ $(TCLOBJ) $(PCREOBJ)
# TODO this PCREOBJ is a dirty hack

libpprzbus.so.$(MAJOR).$(MINOR): $(OBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libpprzbus.so.$(MAJOR) -o $@ $(OBJ) $(PCRELIB) $(REDISLIB)
#	$(CC) -G -Wl,-h,libpprzbus.so.$(MAJOR) -o $@ $(OBJ)		#solaris
#	libtool -dynamic -o $@ $(OBJ) $(PCRELIB) -lc

libpprzbus_omp.so.$(MAJOR).$(MINOR): $(OMPOBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libpprzbus.so.$(MAJOR) -o $@ $(OMPOBJ) $(PCRELIB) $(OMPLIB) $(REDISLIB)
#	$(CC) -G -Wl,-h,libpprzbus.so.$(MAJOR) -o $@ $(OBJ)		#solaris
#	libtool -dynamic -o $@ $(OBJ) $(PCRELIB) -lc

libgpprzbus.so.$(MAJOR).$(MINOR): $(GOBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libgpprzbus.so.$(MAJOR) -o $@ $(GOBJ) $(PCRELIB) $(REDISLIB)
#	$(CC) -G -Wl,-h,libgpprzbus.so.$(MAJOR) -o $@ $(GOBJ) #solaris
#	libtool -dynamic -o $@ $(GOBJ) $(PCRELIB) -lc

libxtpprzbus.so.$(MAJOR).$(MINOR): $(XTOBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libxtpprzbus.so.$(MAJOR) -o $@ $(XTOBJ) $(XTLIB) $(PCRELIB) $(REDISLIB)
#	$(CC) -G -Wl,-h,libxtpprzbus.so.$(MAJOR) -o $@ $(XTOBJ) $(XTLIB) #solaris
#	libtool -dynamic -o $@ $(XTOBJ) $(XTLIB) $(PCRELIB) -lc

libglibpprzbus.so.$(MAJOR).$(MINOR): $(GLIBOBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libglibpprzbus.so.$(MAJOR) -o $@ $(GLIBOBJ) $(GLIBLIB) $(PCRELIB) $(REDISLIB)
#	$(CC) -G -Wl,-h,libglibpprzbus.so.$(MAJOR) -o $@ $(GLIBOBJ) $(GLIBLIB) #solaris
#	libtool -dynamic -o $@ $(GLIBOBJ) $(GLIBLIB) $(PCRELIB) -lc

libglutpprzbus.so.$(MAJOR).$(MINOR): $(GLIBOBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libglutpprzbus.so.$(MAJOR) -o $@ $(GLUTOBJ) $(GLUTLIB) $(PCRELIB) $(REDISLIB)
#	$(CC) -G -Wl,-h,libglutpprzbus.so.$(MAJOR) -o $@ $(GLUTOBJ) $(GLUTLIB) #	solaris
#	libtool -dynamic -o $@ $(GLUTOBJ) $(GLUTLIB) $(PCRELIB) -lc

libtclpprzbus.so.$(MAJOR).$(MINOR): $(TCLOBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libtclpprzbus.so.$(MAJOR) -o $@ $(TCLOBJ) $(TCLLIB) $(PCREOBJ) $(PCRELIB) $(REDISLIB)
#	$(CC) -G -Wl,-h,libtclpprzbus.so.$(MAJOR) -o $@ $(TCLOBJ) $(TCLLIB) $(PCREOBJ) # solaris
#	libtool -dynamic -o $@ $(TCLOBJ) $(TCLLIB) $(PCREOBJ) $(PCRELIB) -lc

distclean: clean
clean:
	-rm -f $(TARGETS) $(TARGETLIBS)  *.o *.a *.so *.so.* *~ *.pc



installlibs: static-libs shared-libs
	test -d $(DESTDIR)$(PREFIX)$(LIB)  || mkdir -p $(DESTDIR)$(PREFIX)$(LIB)
	-test -d $(DESTDIR)$(X11_PREFIX)$(LIB)  || mkdir -p $(DESTDIR)$(X11_PREFIX)$(LIB)

	install -m644 libpprzbus.a						$(DESTDIR)$(PREFIX)$(LIB)
	install -m644 libgpprzbus.a						$(DESTDIR)$(PREFIX)$(LIB)
	-install -m644 libxtpprzbus.a					$(DESTDIR)$(X11_PREFIX)$(LIB)
	install -m644 libtclpprzbus.a					$(DESTDIR)$(PREFIX)$(LIB)
	install -m644 libglibpprzbus.a					$(DESTDIR)$(PREFIX)$(LIB)

	install -m644 libpprzbus.so.$(MAJOR).$(MINOR)			$(DESTDIR)$(PREFIX)$(LIB)
	install -m644 libgpprzbus.so.$(MAJOR).$(MINOR)			$(DESTDIR)$(PREFIX)$(LIB)
	-install -m644 libxtpprzbus.so.$(MAJOR).$(MINOR)			$(DESTDIR)$(X11_PREFIX)$(LIB)
	install -m644 libtclpprzbus.so.$(MAJOR).$(MINOR)			$(DESTDIR)$(PREFIX)$(LIB)
	install -m644 libglibpprzbus.so.$(MAJOR).$(MINOR)			$(DESTDIR)$(PREFIX)$(LIB)

installlibs_omp: static-libs shared-libs
	test -d $(DESTDIR)$(PREFIX)$(LIB)  || mkdir -p $(DESTDIR)$(PREFIX)$(LIB)
	-test -d $(DESTDIR)$(X11_PREFIX)$(LIB)  || mkdir -p $(DESTDIR)$(X11_PREFIX)$(LIB)

	install -m644 libpprzbus_omp.a					$(DESTDIR)$(PREFIX)$(LIB)
	install -m644 libpprzbus_omp.so.$(MAJOR).$(MINOR)			$(DESTDIR)$(PREFIX)$(LIB)

includes:
	test -d $(DESTDIR)$(PREFIX)/include/Pprzbus || mkdir -p $(DESTDIR)$(PREFIX)/include/Pprzbus
	install -m644 pprzbus.h						$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbusbuffer.h					$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbuschannel.h					$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbusdebug.h					$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbusglibloop.h					$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbusglutloop.h*					$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbusloop.h						$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbustcl.h*						$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 pprzbusxtloop.h					$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 list.h						$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 timer.h						$(DESTDIR)$(PREFIX)/include/Pprzbus/
	install -m644 version.h						$(DESTDIR)$(PREFIX)/include/Pprzbus/

installliblinks: installlibs
	ln -fs $(PREFIX)$(LIB)/libpprzbus.so.$(MAJOR).$(MINOR)		$(DESTDIR)$(PREFIX)$(LIB)/libpprzbus.so
	ln -fs $(PREFIX)$(LIB)/libpprzbus.so.$(MAJOR).$(MINOR)		$(DESTDIR)$(PREFIX)$(LIB)/libpprzbus.so.$(MAJOR)
	ln -fs $(PREFIX)$(LIB)/libgpprzbus.so.$(MAJOR).$(MINOR)		$(DESTDIR)$(PREFIX)$(LIB)/libgpprzbus.so
	ln -fs $(PREFIX)$(LIB)/libgpprzbus.so.$(MAJOR).$(MINOR)		$(DESTDIR)$(PREFIX)$(LIB)/libgpprzbus.so.$(MAJOR)
	-ln -fs $(X11_PREFIX)$(LIB)/libxtpprzbus.so.$(MAJOR).$(MINOR)	$(DESTDIR)$(X11_PREFIX)$(LIB)/libxtpprzbus.so
	-ln -fs $(X11_PREFIX)$(LIB)/libxtpprzbus.so.$(MAJOR).$(MINOR)	$(DESTDIR)$(X11_PREFIX)$(LIB)/libxtpprzbus.so.$(MAJOR)
	ln -fs $(PREFIX)$(LIB)/libtclpprzbus.so.$(MAJOR).$(MINOR)		$(DESTDIR)$(PREFIX)$(LIB)/libtclpprzbus.so
	ln -fs $(PREFIX)$(LIB)/libtclpprzbus.so.$(MAJOR).$(MINOR)		$(DESTDIR)$(PREFIX)$(LIB)/libtclpprzbus.so.$(MAJOR)
	ln -fs $(PREFIX)$(LIB)/libpprzbus_omp.so.$(MAJOR).$(MINOR)		$(DESTDIR)$(PREFIX)$(LIB)/libpprzbus_omp.so

install: installlibs installliblinks includes installpkgconf
install_omp: installlibs_omp
rpm::
	/usr/bin/rpmize

pkgconf:
	for f in *.pc.in ; do \
		sed -e 's,@PREFIX@,$(PREFIX),; s,@MAJOR@,$(MAJOR),; s,@MINOR@,$(MINOR),; s,@PCRELIB@,$(PCRELIB),; s,@EXTRALIB@,$(EXTRALIB),' $$f > $$(echo $$f | cut -f 1,2 -d .); \
	done

installpkgconf: pkgconf
	test -d $(DESTDIR)$(PREFIX)/lib/pkgconfig || mkdir -p $(DESTDIR)$(PREFIX)/lib/pkgconfig
	install -m 644 *.pc	$(DESTDIR)$(PREFIX)/lib/pkgconfig
