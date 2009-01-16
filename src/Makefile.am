## Process this file with automake to produce Makefile.in

## Created by Anjuta

#gladedir = $(datadir)/fences/glade
#glade_DATA = fences.glade

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(FENCES_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = fences

fences_SOURCES = \
	callbacks.c callbacks.h \
	main.c \
	draw.c \
	gamedata.c gamedata.h \
	fences.glade \
	tile_cache.c tile_cache.h \
	geometry_tools.c geometry_tools.h

fences_LDFLAGS = 

fences_LDADD = $(FENCES_LIBS)

#EXTRA_DIST = $(glade_DATA)

# Tell automake to include fences.xml as data (and where)
fencesdir = $(datadir)/fences/
fences_DATA = fences.xml

# Tell automake how to build fences.xml from fences.glade
.glade.xml:
	gtk-builder-convert $< $@
