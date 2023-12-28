cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by KDE release team automatically
# https://community.kde.org/Guidelines_and_HOWTOs/Application_Versioning
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "01")
set (RELEASE_SERVICE_VERSION_MICRO "80")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(juk VERSION ${RELEASE_SERVICE_VERSION})

set(KF6_MIN_VERSION "5.240.0")
# Any higher and many Qt keywords get disabled
set(KDE_COMPILERSETTINGS_LEVEL "5.84.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${ECM_MODULE_PATH})

include(CMakePushCheckState)
include(CheckIncludeFileCXX)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Concurrent Gui Svg Network Test Widgets)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Completion
    Config
    Crash
    GlobalAccel
    I18n
    IconThemes
    DBusAddons
    DocTools
    KIO
    JobWidgets
    Notifications
    TextWidgets
    XmlGui
    Wallet
    WidgetsAddons
    WindowSystem
    StatusNotifierItem
)
find_package(Phonon4Qt6 4.6.60 REQUIRED NO_MODULE)

find_package(Taglib 1.6 REQUIRED)

#add_subdirectory( doc )
add_subdirectory( tests )
add_subdirectory( pics )

#set_package_properties(FEATURE PROPERTIES DESCRIPTION "A library for developing MusicBrainz enabled tagging applications" URL "https://wiki.musicbrainz.org/History:libtunepimp" TYPE OPTIONAL PURPOSE "Provides MusicBrainz tagging in Juk.")

# TODO: tunepimp is fully unsupported, replace this when tunepimp is replaced
# with whatever is actually current this decade.
set(HAVE_TUNEPIMP 0)

########### next target ###############

add_definitions(-DQT_STL -DQT_NO_URL_CAST_FROM_STRING)

configure_file (config-juk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-juk.h )

add_executable(juk
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
   directoryloader.cpp
   dynamicplaylist.cpp
   exampleoptions.cpp
   folderplaylist.cpp
   filehandle.cpp
   filerenamer.cpp
   filerenameroptions.cpp
   filerenamerconfigdlg.cpp
   webimagefetcher.cpp
   historyplaylist.cpp
   iconsupport.cpp
   juk.cpp
   juktag.cpp
   keydialog.cpp
   lyricswidget.cpp
   main.cpp
   mediafiles.cpp
   mpris2/mediaplayer2.cpp
   mpris2/mediaplayer2player.cpp
   mpris2/mpris2.cpp
   nowplaying.cpp
   playermanager.cpp
   playlist.cpp
   playlistbox.cpp
   playlistcollection.cpp
   playlistinterface.cpp
   playlistitem.cpp
   playlistsearch.cpp
   playlistsharedsettings.cpp
   playlistsplitter.cpp
   scrobbler.cpp
   scrobbleconfigdlg.cpp
   searchplaylist.cpp
   searchwidget.cpp
   slideraction.cpp
   statuslabel.cpp
   stringshare.cpp
   systemtray.cpp
   tageditor.cpp
   tagguesser.cpp
   tagguesserconfigdlg.cpp
   tagrenameroptions.cpp
   tagtransactionmanager.cpp
   treeviewitemplaylist.cpp
   upcomingplaylist.cpp
   viewmode.cpp

   juk.qrc
)

ecm_qt_declare_logging_category(juk HEADER juk_debug.h
                                IDENTIFIER JUK_LOG CATEGORY_NAME org.kde.juk)

set(JUK_DBUS_SRC "")

qt_add_dbus_adaptor(JUK_DBUS_SRC org.kde.juk.collection.xml
    dbuscollectionproxy.h DBusCollectionProxy )
qt_add_dbus_adaptor(JUK_DBUS_SRC org.kde.juk.player.xml playermanager.h PlayerManager)
qt_add_dbus_adaptor(JUK_DBUS_SRC org.kde.juk.search.xml searchwidget.h SearchWidget)

target_sources(juk PRIVATE ${JUK_DBUS_SRC})

ki18n_wrap_ui(juk
    filerenamerbase.ui
    filerenameroptionsbase.ui
    directorylistbase.ui
    tagguesserconfigdlgwidget.ui
    exampleoptionsbase.ui
    coverdialogbase.ui
    deletedialogbase.ui
    tageditor.ui
)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*-apps-juk.png")
ecm_add_app_icon(juk ICONS ${ICONS_SRCS})

target_include_directories(juk SYSTEM PRIVATE ${CMAKE_BINARY_DIR} ${PHONON_INCLUDES})

kde_target_enable_exceptions(juk PRIVATE)
target_compile_definitions(juk PRIVATE QT_USE_QSTRINGBUILDER)
set_target_properties(juk PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    )

if(NOT MSVC AND NOT ( WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" ) )
    set( LIBMATH m )
endif()

target_link_libraries(juk ${LIBMATH}
    Qt::Concurrent
    Qt::Gui
    Qt::Svg
    Qt::Widgets
    Qt::Network
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::Completion
    KF6::Crash
    KF6::DBusAddons
    KF6::GlobalAccel
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::KIOFileWidgets
    KF6::JobWidgets
    KF6::Notifications
    KF6::I18n
    KF6::IconThemes
    KF6::TextWidgets
    KF6::XmlGui
    KF6::WindowSystem
    KF6::WidgetsAddons
    KF6::Wallet
    KF6::StatusNotifierItem
    Phonon::phonon4qt6
    Taglib::Taglib
)

if(TUNEPIMP_FOUND)
    target_link_libraries(juk ${TUNEPIMP_LIBRARIES})
endif(TUNEPIMP_FOUND)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

install(TARGETS juk  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS org.kde.juk.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install(
    FILES juk.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR}
)

install( FILES org.kde.juk.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
install( PROGRAMS jukservicemenu.desktop  DESTINATION ${KDE_INSTALL_DATADIR}/kio/servicemenus )
install( FILES org.kde.juk.collection.xml org.kde.juk.player.xml org.kde.juk.search.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

ki18n_install(po)
#kdoctools_install(po)

ecm_install_icons(ICONS
    128-apps-juk.png
    16-apps-juk.png
    32-apps-juk.png
    48-apps-juk.png
    64-apps-juk.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
