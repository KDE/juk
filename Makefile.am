####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = juk

## INCLUDES were found outside kdevelop specific part

juk_SOURCES = audiodata.cpp cacheitem.cpp filelistitemdata.cpp genrelistreader.cpp genrelistlist.cpp genrelist.cpp genre.cpp player.cpp filelistitem.cpp tag.cpp filelist.cpp customaction.cpp playlistwidget.cpp slideraction.cpp taggerwidget.cpp juk.cpp main.cpp 
juk_LDADD   = -lid3 -lsoundserver_idl $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)

SUBDIRS = pics data 

EXTRA_DIST = main.cpp juk.cpp juk.h juk.desktop jukui.rc taggerwidget.cpp taggerwidget.h slideraction.cpp slideraction.h playlistwidget.cpp playlistwidget.h customaction.h customaction.cpp filelist.cpp filelist.h tag.cpp tag.h filelistitem.cpp filelistitem.h player.cpp player.h genre.h genre.cpp hi16-app-juk.png hi32-app-juk.png hi48-app-juk.png genrelist.cpp genrelist.h genrelistlist.cpp genrelistlist.h genrelistreader.cpp genrelistreader.h filelistitemdata.cpp filelistitemdata.h cacheitem.cpp cacheitem.h audiodata.cpp audiodata.h 

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Multimedia/
	$(INSTALL_DATA) $(srcdir)/juk.desktop $(kde_appsdir)/Multimedia/juk.desktop

uninstall-local:
	-rm -f $(kde_appsdir)/Multimedia/juk.desktop

####### kdevelop will overwrite this part!!! (end)############

# set the include path for X, qt and KDE
INCLUDES= $(all_includes) $(KDE_INCLUDES)/arts

METASOURCES = AUTO
KDE_ICON = AUTO

# the library search path. 
juk_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/juk
rc_DATA = jukui.rc

messages: rc.cpp
