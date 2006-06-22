project(juk)
configure_file (config-juk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-juk.h )


add_subdirectory( pics ) 



set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

add_definitions(${TAGLIB_CFLAGS})

set(juk_SRCS searchadaptor.cpp  
   advancedsearchdialog.cpp 
   actioncollection.cpp 
   cache.cpp 
   categoryreaderinterface.cpp 
   collectionlist.cpp 
   coverdialog.cpp 
   covericonview.cpp 
   coverinfo.cpp 
   covermanager.cpp 
   deletedialog.cpp 
   directorylist.cpp 
   dynamicplaylist.cpp 
   exampleoptions.cpp 
   folderplaylist.cpp 
   filehandle.cpp 
   filerenamer.cpp 
   filerenameroptions.cpp 
   filerenamerconfigdlg.cpp 
   googlefetcher.cpp 
   googlefetcherdialog.cpp 
   historyplaylist.cpp 
   juk.cpp 
   k3bexporter.cpp 
   keydialog.cpp 
   main.cpp 
   mediafiles.cpp 
   musicbrainzquery.cpp 
   nowplaying.cpp 
   playermanager.cpp 
   player.cpp
   playlist.cpp 
   playlistbox.cpp 
   playlistcollection.cpp 
   playlistinterface.cpp 
   playlistitem.cpp 
   playlistsearch.cpp 
   playlistsplitter.cpp 
   searchplaylist.cpp 
   searchwidget.cpp 
   slideraction.cpp 
   sortedstringlist.cpp 
   splashscreen.cpp 
   statuslabel.cpp 
   stringshare.cpp 
   systemtray.cpp 
   tag.cpp 
   tageditor.cpp 
   tagguesser.cpp 
   tagguesserconfigdlg.cpp 
   tagrenameroptions.cpp 
   tagtransactionmanager.cpp 
   trackpickerdialog.cpp 
   tracksequenceiterator.cpp 
   tracksequencemanager.cpp 
   treeviewitemplaylist.cpp 
   upcomingplaylist.cpp 
   ktrm.cpp 
   viewmode.cpp )

kde4_automoc(${juk_SRCS})

kde4_add_ui_files(juk_SRCS
	filerenamerbase.ui
	filerenameroptionsbase.ui
)

kde4_add_ui3_files(juk_SRCS 
   coverdialogbase.ui 
   deletedialogbase.ui 
   directorylistbase.ui 
   exampleoptionsbase.ui 
   tagguesserconfigdlgwidget.ui 
   trackpickerdialogbase.ui )

kde4_add_dcop_skels(juk_SRCS jukIface.h )

kde4_add_executable(juk ${juk_SRCS})

# Add gstreamer here too...

target_link_libraries(juk  ${KDE4_KDECORE_LIBS} m kio khtml kdeui tag phononcore)

install_targets(/bin juk )


########### next target ###############

set(tagguessertest_SRCS tagguessertest.cpp tagguesser.cpp )

kde4_automoc(${tagguessertest_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(tagguessertest ${tagguessertest_SRCS})

target_link_libraries(tagguessertest  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install_files( ${XDG_APPS_DIR} FILES juk.desktop )
install_files( ${DATA_INSTALL_DIR}/juk FILES jukui.rc jukui-rtl.rc )
install_files( ${DATA_INSTALL_DIR}/konqueror/servicemenus FILES jukservicemenu.desktop )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )




#original Makefile.am contents follow:

#bin_PROGRAMS = juk
#check_PROGRAMS = tagguessertest
#
#juk_SOURCES = \
#	advancedsearchdialog.cpp \
#	actioncollection.cpp \
#	akodeplayer.cpp \
#	artsplayer.cpp \
#	cache.cpp \
#	categoryreaderinterface.cpp \
#	collectionlist.cpp \
#	coverdialog.cpp \
#	coverdialogbase.ui \
#	covericonview.cpp \
#	coverinfo.cpp \
#	covermanager.cpp \
#	deletedialog.cpp \
#	deletedialogbase.ui \
#	directorylist.cpp \
#	directorylistbase.ui \
#	dynamicplaylist.cpp \
#	exampleoptions.cpp \
#	exampleoptionsbase.ui \
#	folderplaylist.cpp \
#	filehandle.cpp \
#	filerenamer.cpp \
#	filerenamerbase.ui \
#	filerenameroptions.cpp \
#	filerenameroptionsbase.ui \
#	filerenamerconfigdlg.cpp \
#	gstreamerplayer.cpp \
#	googlefetcher.cpp \
#	googlefetcherdialog.cpp \
#	historyplaylist.cpp \
#	juk.cpp \
#	jukIface.skel \
#	k3bexporter.cpp \
#	keydialog.cpp \
#	main.cpp \
#	mediafiles.cpp \
#	musicbrainzquery.cpp \
#	nowplaying.cpp \
#	playermanager.cpp \
#	playlist.cpp \
#	playlistbox.cpp \
#	playlistcollection.cpp \
#	playlistinterface.cpp \
#	playlistitem.cpp \
#	playlistsearch.cpp \
#	playlistsplitter.cpp \
#	searchplaylist.cpp \
#	searchwidget.cpp \
#	slideraction.cpp \
#	sortedstringlist.cpp \
#	splashscreen.cpp \
#	statuslabel.cpp \
#	stringshare.cpp \
#	systemtray.cpp \
#	tag.cpp \
#	tageditor.cpp \
#	tagguesser.cpp \
#	tagguesserconfigdlg.cpp \
#	tagguesserconfigdlgwidget.ui \
#	tagrenameroptions.cpp \
#	tagtransactionmanager.cpp \
#	trackpickerdialog.cpp \
#	trackpickerdialogbase.ui \
#	tracksequenceiterator.cpp \
#	tracksequencemanager.cpp \
#	treeviewitemplaylist.cpp \
#	upcomingplaylist.cpp \
#	ktrm.cpp \
#	viewmode.cpp
#
#tagguessertest_SOURCES = tagguessertest.cpp tagguesser.cpp
#
#INCLUDES= $(all_includes) $(taglib_includes) $(akode_includes) $(GST_CFLAGS) $(ARTS_CFLAGS)
#
###################################################
## check to see if MusicBrainz is available
###################################################
#if link_lib_TP
#tplibs = -ltunepimp
#endif
###################################################
#
#juk_LDADD = -lm $(LDADD_GST) $(tplibs) $(LIB_KIO) $(taglib_libs) $(akode_libs) $(LIB_KHTML) $(LIB_ARTS)
#juk_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LDFLAGS_GST)
#
#KDE_CXXFLAGS = $(USE_EXCEPTIONS)
#
#tagguessertest_LDADD = $(LIB_KDECORE)
#tagguessertest_LDFLAGS = $(all_libraries)
#
#SUBDIRS = pics
#
#rcdir = $(kde_datadir)/juk
#rc_DATA = jukui.rc jukui-rtl.rc
#
#servicemenudir = $(kde_datadir)/konqueror/servicemenus
#servicemenu_DATA = jukservicemenu.desktop
#
#METASOURCES = AUTO
#KDE_ICON = AUTO
#POFILES = AUTO
#
#xdg_apps_DATA = juk.desktop
#
#messages: rc.cpp
#	$(EXTRACTRC) *.rc >> rc.cpp
#	$(XGETTEXT) *.rc *.cpp *.h -o $(podir)/juk.pot
#	-rm rc.cpp
#KDE_OPTIONS=nofinal
