bin_PROGRAMS = juk
check_PROGRAMS = tagguessertest mbtest

juk_SOURCES = \
	advancedsearchdialog.cpp \
	actioncollection.cpp \
	artsplayer.cpp \
	cache.cpp \
	collectionlist.cpp \
	directorylist.cpp \
	directorylistbase.ui \
	dynamicplaylist.cpp \
	folderplaylist.cpp \
	filehandle.cpp \
	filerenamer.cpp \
	filerenamerconfigdlg.cpp \
	filerenamerconfigdlgwidget.ui \
	gstreamerplayer.cpp \
	historyplaylist.cpp \
	juk.cpp \
	jukIface.skel \
	k3bexporter.cpp \
	keydialog.cpp \
	main.cpp \
	mediafiles.cpp \
	musicbrainzquery.cpp \
	painteater.cpp \
	playermanager.cpp \
	playlist.cpp \
	playlistbox.cpp \
	playlistcollection.cpp \
	playlistinterface.cpp \
	playlistitem.cpp \
	playlistsearch.cpp \
	playlistsplitter.cpp \
	searchplaylist.cpp \
	searchwidget.cpp \
	slideraction.cpp \
	sortedstringlist.cpp \
	splashscreen.cpp \
	statuslabel.cpp \
	stringshare.cpp \
	systemtray.cpp \
	tag.cpp \
	tageditor.cpp \
	tagguesser.cpp \
	tagguesserconfigdlg.cpp \
	tagguesserconfigdlgwidget.ui \
	trackpickerdialog.cpp \
	trackpickerdialogbase.ui \
	treeviewitemplaylist.cpp \
	ktrm.cpp \
	viewmode.cpp

tagguessertest_SOURCES = tagguessertest.cpp tagguesser.cpp
mbtest_SOURCES = mbtest.cpp musicbrainzquery.cpp

INCLUDES= $(all_includes) -I$(arts_includes) $(taglib_includes)

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
mblibs = -lmusicbrainz -ltunepimp
endif
##################################################

juk_LDADD = $(gstlibs) $(mblibs) -lartskde $(LIB_KIO) $(taglib_libs)
juk_LDFLAGS = $(all_libraries) $(KDE_RPATH)

tagguessertest_LDADD = $(LIB_KDECORE)
tagguessertest_LDFLAGS = $(all_libraries)

mbtest_LDADD = $(LIB_KDECORE) $(mblibs)
mbtest_LDFLAGS = $(all_libraries)

SUBDIRS = pics

rcdir = $(kde_datadir)/juk
rc_DATA = jukui.rc

servicemenudir = $(kde_datadir)/konqueror/servicemenus
servicemenu_DATA = jukservicemenu.desktop

METASOURCES = AUTO
KDE_ICON = AUTO
POFILES = AUTO

xdg_apps_DATA = juk.desktop

messages: rc.cpp
	$(EXTRACTRC) *.rc >> rc.cpp
	$(XGETTEXT) *.rc *.cpp *.h -o $(podir)/juk.pot
	-rm rc.cpp

KDE_OPTIONS = nofinal
