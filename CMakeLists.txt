cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(juk)

find_package(ECM 5.38 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakePushCheckState)
include(CheckIncludeFileCXX)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMInstallIcons)
include(ECMAddAppIcon)

find_package(Qt5 5.2 REQUIRED COMPONENTS Gui Svg Network Test Widgets)
find_package(KF5 REQUIRED COMPONENTS CoreAddons Completion GlobalAccel
	I18n IconThemes DocTools TextWidgets XmlGui Wallet
	WidgetsAddons WindowSystem KDELibs4Support)
find_package(Phonon4Qt5 4.6.60 REQUIRED NO_MODULE)

find_package(Taglib 1.6 REQUIRED)

include_directories( ${PHONON_INCLUDES} ${KDE4_INCLUDES} ${QT_INCLUDES} )

add_subdirectory( doc )
add_subdirectory( tests )
add_subdirectory( pics )

find_package(TunePimp)
set_package_properties(FEATURE PROPERTIES DESCRIPTION "A library for developing MusicBrainz enabled tagging applications" URL "http://www.musicbrainz.org/products/tunepimp" TYPE OPTIONAL PURPOSE "Provides MusicBrainz tagging in Juk.")
if(TUNEPIMP_FOUND)
	set(HAVE_TUNEPIMP 1)
	if(TUNEPIMP_FOUND_VERSION_4)
		set(HAVE_TUNEPIMP 4)
        endif(TUNEPIMP_FOUND_VERSION_4)	
        if(TUNEPIMP_FOUND_VERSION_5)
                set(HAVE_TUNEPIMP 5)
        endif(TUNEPIMP_FOUND_VERSION_5) 	
else(TUNEPIMP_FOUND)
	set(HAVE_TUNEPIMP 0)
endif(TUNEPIMP_FOUND)	

########### next target ###############

include_directories( ${TAGLIB_INCLUDES} )
add_definitions(-DQT_STL)

# Look for Ogg Opus support in taglib (not released yet)
cmake_push_check_state()
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${TAGLIB_INCLUDES})
check_include_file_cxx(opusfile.h TAGLIB_HAS_OPUSFILE)
cmake_pop_check_state()

configure_file (config-juk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-juk.h )

set(tunepimp_SRCS)
if(TUNEPIMP_FOUND)
	set(tunepimp_SRCS trackpickerdialog.cpp)
	include_directories( ${TUNEPIMP_INCLUDE_DIR} )
endif(TUNEPIMP_FOUND)	

set(juk_SRCS  ${tunepimp_SRCS}
   advancedsearchdialog.cpp
   slider.cpp
   svghandler.cpp
   volumepopupbutton.cpp
   actioncollection.cpp
   cache.cpp
   categoryreaderinterface.cpp
   collectionlist.cpp
   coverdialog.cpp
   covericonview.cpp
   coverinfo.cpp
   covermanager.cpp
   coverproxy.cpp
   dbuscollectionproxy.cpp
   deletedialog.cpp
   directorylist.cpp
   dynamicplaylist.cpp
   exampleoptions.cpp
   folderplaylist.cpp
   filehandle.cpp
   filerenamer.cpp
   filerenameroptions.cpp
   filerenamerconfigdlg.cpp
   webimagefetcher.cpp
   historyplaylist.cpp
   juk.cpp
   k3bexporter.cpp
   keydialog.cpp
   lyricswidget.cpp
   main.cpp
   mediafiles.cpp
   mpris2/mediaplayer2.cpp
   mpris2/mediaplayer2player.cpp
   mpris2/mpris2.cpp
   musicbrainzquery.cpp
   nowplaying.cpp
   playermanager.cpp
   playlist.cpp
   playlistbox.cpp
   playlistcollection.cpp
   playlistexporter.cpp
   playlistinterface.cpp
   playlistitem.cpp
   playlistsearch.cpp
   playlistsplitter.cpp
   scrobbler.cpp
   scrobbleconfigdlg.cpp
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
   tracksequenceiterator.cpp
   tracksequencemanager.cpp
   treeviewitemplaylist.cpp
   upcomingplaylist.cpp
   ktrm.cpp
   viewmode.cpp )

qt5_add_dbus_adaptor( juk_SRCS org.kde.juk.collection.xml
    dbuscollectionproxy.h DBusCollectionProxy )
qt5_add_dbus_adaptor( juk_SRCS org.kde.juk.player.xml playermanager.h PlayerManager)
qt5_add_dbus_adaptor( juk_SRCS org.kde.juk.search.xml searchwidget.h SearchWidget)


qt5_wrap_ui(juk_SRCS
	filerenamerbase.ui
	filerenameroptionsbase.ui
	directorylistbase.ui
	trackpickerdialogbase.ui
	tagguesserconfigdlgwidget.ui
	exampleoptionsbase.ui
	coverdialogbase.ui
	deletedialogbase.ui
	tageditor.ui
)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*-apps-juk.png")
ecm_add_app_icon(juk_SRCS ICONS ${ICONS_SRCS})
add_executable(juk ${juk_SRCS})

kde_target_enable_exceptions(juk PRIVATE)

if(NOT MSVC AND NOT ( WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" ) )
    set( LIBMATH m )
endif()

target_link_libraries(juk ${LIBMATH} Qt5::Gui Qt5::Svg Qt5::Widgets
    Qt5::Network KF5::CoreAddons KF5::Completion KF5::GlobalAccel
    KF5::I18n KF5::IconThemes KF5::TextWidgets KF5::XmlGui KF5::WindowSystem
    KF5::WidgetsAddons KF5::Wallet KF5::KDELibs4Support ${PHONON_LIBRARY} ${TAGLIB_LIBRARIES})
if(TUNEPIMP_FOUND)
	target_link_libraries(juk ${TUNEPIMP_LIBRARIES})
endif(TUNEPIMP_FOUND)


install(TARGETS juk  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS org.kde.juk.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES juk.notifyrc jukui.rc jukui-rtl.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/juk )
install( FILES org.kde.juk.appdata.xml DESTINATION ${SHARE_INSTALL_PREFIX}/metainfo )
install( FILES jukservicemenu.desktop  DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus )
install( FILES org.kde.juk.collection.xml org.kde.juk.player.xml org.kde.juk.search.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

ecm_install_icons( ${ICON_INSTALL_DIR}   )
