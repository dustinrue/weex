bin_PROGRAMS=weex
weex_SOURCES=config.c ftplib.c ftplib.h weex.c intl.h \
parsecfg.c parsecfg.h proto.h shhopt.c shhopt.h strlib.c strlib.h variable.h \
ftpmain.c message.c cache.c sub.c filedata.c log.c

LDADD=$(INTLLIBS)
CPPFLAGS=-DLOCALEDIR=\""$(localedir)"\"
INCLUDES=-I$(top_srcdir)/intl -I$(top_builddir)/intl
