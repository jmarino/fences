## Process this file with automake to produce Makefile.in

## Created by Anjuta

#gladedir = $(datadir)/fences/glade
#glade_DATA = fences.glade

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(FENCES_CFLAGS)

# NOTE: flags "-ggdb -O0" help debugging. Remove in final version.
AM_CFLAGS =\
	 -Wall\
	 -ggdb -O0

bin_PROGRAMS = fences

fences_SOURCES = \
	main.c \
	i18n.h \
	callbacks.c callbacks.h \
	draw.c draw.h \
	geometry.c geometry.h \
	avl-tree.c avl-tree.h \
	gamedata.c gamedata.h \
	click-mesh.c \
	mesh-tools.c \
	penrose-tile.c tiles.h \
	square-tile.c \
	build-loop.c \
	game-solver.c game-solver.h \
	brute-force.c brute-force.h \
	build-game.c \
	solve-combinations.c \
	solve-tools.c \
	gui.c gui.h \
	history.c history.h \
	benchmark.c benchmark.h \
	newgame-dialog.c \
	triangle-tile.c \
	qbert-tile.c \
	hex-tile.c \
	snub-tile.c \
	cairo-tile.c \
	cartwheel-tile.c \
	trihex-tile.c \
	line-change.c

fences_LDFLAGS = 

fences_LDADD = $(FENCES_LIBS)

#EXTRA_DIST = $(glade_DATA)

# Tell automake to include fences.xml as data (and where)
fencesdir = $(datadir)/fences/
fences_DATA = menu_toolbar.ui
