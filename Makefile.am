bin_PROGRAMS = juk
check_PROGRAMS = tagguessertest mbtest

juk_SOURCES = \
	advancedsearchdialog.cpp \
	actioncollection.cpp \
	artsplayer.cpp \
	cache.cpp \
	categoryreaderinterface.cpp \
	collectionlist.cpp \
	deletedialog.cpp \
	deletedialogbase.ui \
	directorylist.cpp \
	directorylistbase.ui \
	dynamicplaylist.cpp \
	exampleoptions.cpp \
	exampleoptionsbase.ui \
	folderplaylist.cpp \
	filehandle.cpp \
	filerenamer.cpp \
	filerenamerbase.ui \
	filerenameroptions.cpp \
	filerenameroptionsbase.ui \
	filerenamerconfigdlg.cpp \
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
	tagrenameroptions.cpp \
	tagtransactionmanager.cpp \
	trackpickerdialog.cpp \
	trackpickerdialogbase.ui \
	tracksequenceiterator.cpp \
	tracksequencemanager.cpp \
	treeviewitemplaylist.cpp \
	upcomingplaylist.cpp \
	ktrm.cpp \
	viewmode.cpp

tagguessertest_SOURCES = tagguessertest.cpp tagguesser.cpp
mbtest_SOURCES = mbtest.cpp musicbrainzquery.cpp

INCLUDES= $(all_includes) -I$(arts_includes) $(taglib_includes) $(GST_CFLAGS)

##################################################
# check to see if MusicBrainz is available
##################################################
if link_lib_MB
mblibs = -lmusicbrainz -ltunepimp
endif
##################################################

juk_LDADD = $(LIB_GST) $(mblibs) -lartskde $(LIB_KIO) $(taglib_libs)
juk_LDFLAGS = $(all_libraries) $(KDE_RPATH)

tagguessertest_LDADD = $(LIB_KDECORE)
tagguessertest_LDFLAGS = $(all_libraries)

mbtest_LDADD = $(LIB_KDECORE) $(mblibs)
mbtest_LDFLAGS = $(all_libraries)

SUBDIRS = pics

rcdir = $(kde_datadir)/juk
rc_DATA = jukui.rc jukui-rtl.rc

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
