bin_PROGRAMS = juk
check_PROGRAMS = tagguessertest

juk_SOURCES = directorylistbase.ui genrelisteditorbase.ui \
	gstreamerplayer.cpp artsplayer.cpp directorylist.cpp stringhash.cpp \
	sortedstringlist.cpp systemtray.cpp genrelisteditor.cpp  splashscreen.cpp \
	statuslabel.cpp cachedtag.cpp oggtag.cpp id3tag.cpp collectionlist.cpp \
	playlistitem.cpp playlist.cpp playlistsplitter.cpp listboxpixmap.cpp \
	playlistbox.cpp tageditor.cpp cache.cpp genrelistreader.cpp \
	genrelistlist.cpp genrelist.cpp genre.cpp player.cpp tag.cpp customaction.cpp \
	slideraction.cpp keydialog.cpp juk.cpp main.cpp tagguesser.cpp \
	tagguesserconfigdlgwidget.ui tagguesserconfigdlg.cpp

tagguessertest_SOURCES = tagguessertest.cpp tagguesser.cpp

INCLUDES= $(all_includes) -I$(arts_includes)

##################################################
# check to see if GStreamer is available
##################################################
if link_lib_GST
gstlibs = -lkdegst -lkdegstplay
else
gstlibs =
endif
##################################################

juk_LDADD = $(gstlibs) -lid3 -lsoundserver_idl $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KFILE) $(LIB_QT)
juk_LDFLAGS = $(all_libraries) $(KDE_RPATH)

tagguessertest_LDADD = $(LIB_KDECORE)
tagguessertest_LDFLAGS = $(all_libraries)

SUBDIRS = pics data 

rcdir = $(kde_datadir)/juk
rc_DATA = jukui.rc

METASOURCES = AUTO
KDE_ICON = AUTO

xdg_apps_DATA = juk.desktop

messages: rc.cpp
	$(EXTRACTRC) *.rc >> rc.cpp
	$(XGETTEXT) *.rc *.cpp *.h -o $(podir)/juk.pot
	-rm rc.cpp
