EXTRA_DIST = LICENSE data/known_good.tsv tools/dump_neo.py tools/gendb.py \
	tools/gen_meta.py tools/port_crypto.py miniz/ChangeLog.md \
	miniz/LICENSE miniz/readme.md regen.sh

dist_doc_DATA = README

AM_CPPFLAGS = -std=c99 -Wall -Wextra

pkginclude_HEADERS = $(top_srcdir)/src/lithogen.h

noinst_LTLIBRARIES = libzipio.la
libzipio_la_SOURCES = $(top_srcdir)/src/zipio.c
if HAVE_MINIZ
libzipio_la_CFLAGS = $(MINIZ_CFLAGS)
libzipio_la_LIBADD = $(MINIZ_LIBS)
else
libzipio_la_CFLAGS = -I$(top_srcdir)/miniz
libzipio_la_LIBADD = $(top_builddir)/libminiz.la

noinst_LTLIBRARIES += libminiz.la
libminiz_la_SOURCES = \
	$(top_srcdir)/miniz/miniz.c \
	$(top_srcdir)/miniz/miniz.h
endif

lib_LTLIBRARIES = liblithogen.la
liblithogen_la_SOURCES = \
	$(top_srcdir)/src/crypt/crypt.h \
	$(top_srcdir)/src/crypt/crypt_protos.h \
	$(top_srcdir)/src/crypt/nc_cmc.c \
	$(top_srcdir)/src/crypt/nc_kof2k3bl.c \
	$(top_srcdir)/src/crypt/nc_pcm2.c \
	$(top_srcdir)/src/crypt/nc_cthd.c \
	$(top_srcdir)/src/crypt/nc_kof98.c \
	$(top_srcdir)/src/crypt/nc_pvc.c \
	$(top_srcdir)/src/crypt/nc_kof2k2.c \
	$(top_srcdir)/src/crypt/nc_misc.c \
	$(top_srcdir)/src/crypt/nc_sma.c \
	$(top_srcdir)/src/games_db.c \
	$(top_srcdir)/src/meta_db.c \
	$(top_srcdir)/src/lithogen.c \
	$(top_srcdir)/src/lithogen_internal.h \
	$(top_srcdir)/src/neowrite.c \
	$(top_srcdir)/src/recipes.c \
	$(top_srcdir)/src/regions.c
liblithogen_la_LDFLAGS = -no-undefined -export-symbols-regex lithogen_ \
	-version-info 0:0:0
liblithogen_la_LIBADD = $(top_builddir)/libzipio.la

bin_PROGRAMS = lithogen
lithogen_SOURCES = $(top_srcdir)/src/cli.c
lithogen_LDADD = $(top_builddir)/liblithogen.la
lithogen_LDFLAGS = -no-undefined

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lib$(PACKAGE).pc
