project(juk)
configure_file (config-juk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-juk.h )

add_subdirectory( tests )

add_subdirectory( pics ) 


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

# hack, should taglib maybe give the .. directory as well in it's config file?
add_definitions(${TAGLIB_CFLAGS} "${TAGLIB_CFLAGS}/..")

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

set(tunepimp_SRCS)
if(TUNEPIMP_FOUND)
	set(tunepimp_SRCS trackpickerdialog.cpp)
endif(TUNEPIMP_FOUND)	

set(juk_SRCS  ${tunepimp_SRCS}
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
   tracksequenceiterator.cpp 
   tracksequencemanager.cpp 
   treeviewitemplaylist.cpp 
   upcomingplaylist.cpp 
   ktrm.cpp 
   viewmode.cpp )

#Laurent reactivate it
#qt4_add_dbus_adaptor( juk_SRCS org.kde.juk.collection.xml playlistcollection.h PlaylistCollection)
qt4_add_dbus_adaptor( juk_SRCS org.kde.juk.player.xml playermanager.h PlayerManager)
qt4_add_dbus_adaptor( juk_SRCS org.kde.juk.search.xml searchwidget.h SearchWidget)

kde4_automoc(${juk_SRCS})

kde4_add_ui_files(juk_SRCS
	filerenamerbase.ui
	filerenameroptionsbase.ui
	directorylistbase.ui
	trackpickerdialogbase.ui
	tagguesserconfigdlgwidget.ui
	exampleoptionsbase.ui
	coverdialogbase.ui
	deletedialogbase.ui
)


kde4_add_executable(juk ${juk_SRCS})

# Add gstreamer here too...

target_link_libraries(juk  m ${KDE4_KIO_LIBS} ${KDE4_KHTML_LIBS} ${TAGLIB_LIBRARIES}
    ${KDE4_PHONON_LIBS})
if(TUNEPIMP_FOUND)
	target_link_libraries(juk ${TUNEPIMP_LIBRARIES})
endif(TUNEPIMP_FOUND)


install(TARGETS juk  DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES juk.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES jukui.rc jukui-rtl.rc  DESTINATION  ${DATA_INSTALL_DIR}/juk )
install( FILES jukservicemenu.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/servicemenus )
install( FILES org.kde.juk.collection.xml org.kde.juk.player.xml org.kde.juk.search.xml DESTINATION ${DBUS_INTERFACES_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}   )
