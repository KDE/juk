####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = juk

## INCLUDES were found outside kdevelop specific part

juk_SOURCES = sortedstringlist.cpp systemtray.cpp genrelisteditor.cpp genrelisteditorbase.ui splashscreen.cpp statuslabel.cpp cachedtag.cpp oggtag.cpp id3tag.cpp collectionlist.cpp playlistitem.cpp playlist.cpp playlistsplitter.cpp listboxpixmap.cpp playlistbox.cpp tageditor.cpp cache.cpp audiodata.cpp genrelistreader.cpp genrelistlist.cpp genrelist.cpp genre.cpp player.cpp tag.cpp customaction.cpp slideraction.cpp juk.cpp main.cpp 
juk_LDADD   = -lid3 -lsoundserver_idl $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)

SUBDIRS = pics data 

EXTRA_DIST = main.cpp juk.cpp juk.h juk.desktop jukui.rc slideraction.cpp slideraction.h customaction.h customaction.cpp tag.cpp tag.h player.cpp player.h genre.h genre.cpp hi16-app-juk.png hi32-app-juk.png hi48-app-juk.png genrelist.cpp genrelist.h genrelistlist.cpp genrelistlist.h genrelistreader.cpp genrelistreader.h audiodata.cpp audiodata.h cache.cpp cache.h tageditor.cpp tageditor.h playlistbox.cpp playlistbox.h listboxpixmap.cpp listboxpixmap.h playlistsplitter.cpp playlistsplitter.h playlist.cpp playlist.h playlistitem.cpp playlistitem.h collectionlist.cpp collectionlist.h id3tag.cpp id3tag.h oggtag.cpp oggtag.h cachedtag.cpp cachedtag.h statuslabel.cpp statuslabel.h splashscreen.cpp splashscreen.h genrelisteditorbase.ui genrelisteditor.cpp genrelisteditor.h systemtray.h systemtray.cpp sortedstringlist.cpp sortedstringlist.h 

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
