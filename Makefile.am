bin_PROGRAMS = juk
check_PROGRAMS = tagguessertest mbtest

juk_SOURCES = directorylistbase.ui genrelisteditorbase.ui \
	gstreamerplayer.cpp artsplayer.cpp directorylist.cpp stringhash.cpp \
	sortedstringlist.cpp systemtray.cpp genrelisteditor.cpp  splashscreen.cpp \
	statuslabel.cpp cachedtag.cpp oggtag.cpp id3tag.cpp collectionlist.cpp \
	playlistitem.cpp playlist.cpp playlistsplitter.cpp \
	playlistbox.cpp tageditor.cpp cache.cpp genrelistreader.cpp \
	genrelistlist.cpp genrelist.cpp genre.cpp player.cpp tag.cpp customaction.cpp \
	slideraction.cpp keydialog.cpp juk.cpp main.cpp tagguesser.cpp \
	tagguesserconfigdlgwidget.ui tagguesserconfigdlg.cpp jukIface.skel \
	playlistsearch.cpp searchwidget.cpp filerenamer.cpp dynamicplaylist.cpp \
	searchplaylist.cpp musicbrainzquery.cpp mediafiles.cpp


tagguessertest_SOURCES = tagguessertest.cpp tagguesser.cpp
mbtest_SOURCES = mbtest.cpp musicbrainzquery.cpp

INCLUDES= $(all_includes) -I$(arts_includes)

##################################################
# check to see if GStreamer is available
##################################################
if link_lib_GST
gstlibs = -lkdegst -lkdegstplay
endif
##################################################

##################################################
# check to see if MusicBrainz is available
##################################################
if link_lib_MB
mblibs = -lmusicbrainz
endif
##################################################

juk_LDADD = $(gstlibs) $(mblibs) -lid3 -lsoundserver_idl $(LIB_KIO)
juk_LDFLAGS = $(all_libraries) $(KDE_RPATH)

tagguessertest_LDADD = $(LIB_KDECORE)
tagguessertest_LDFLAGS = $(all_libraries)

mbtest_LDADD = $(LIB_KDECORE) $(mblibs)
mbtest_LDFLAGS = $(all_libraries)

SUBDIRS = pics data 

rcdir = $(kde_datadir)/juk
rc_DATA = jukui.rc

servicemenudir = $(kde_datadir)/konqueror/servicemenus
servicemenu_DATA = jukservicemenu.desktop

METASOURCES = AUTO
KDE_ICON = AUTO

xdg_apps_DATA = juk.desktop

messages: rc.cpp
	$(EXTRACTRC) *.rc >> rc.cpp
	$(XGETTEXT) *.rc *.cpp *.h -o $(podir)/juk.pot
	-rm rc.cpp
