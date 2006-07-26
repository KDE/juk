project(juk)
configure_file (config-juk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-juk.h )


add_subdirectory( pics ) 



set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

add_definitions(${TAGLIB_CFLAGS})

set(juk_SRCS searchadaptor.cpp collectionadaptor.cpp playeradaptor.cpp
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

kde4_add_executable(juk ${juk_SRCS})

# Add gstreamer here too...

target_link_libraries(juk  ${KDE4_KDECORE_LIBS} m ${KDE4_KIO_LIBS} khtml ${KDE4_KDEUI_LIBS} tag phononcore)

install(TARGETS juk  DESTINATION ${BIN_INSTALL_DIR} )


########### next target ###############
if(KDE4_BUILD_TESTS)
set(tagguessertest_SRCS tagguessertest.cpp tagguesser.cpp )

kde4_automoc(${tagguessertest_SRCS})

kde4_add_executable(tagguessertest ${tagguessertest_SRCS})

target_link_libraries(tagguessertest  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install( FILES juk.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES jukui.rc jukui-rtl.rc  DESTINATION  ${DATA_INSTALL_DIR}/juk )
install( FILES jukservicemenu.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/servicemenus )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )
kde4_install_icons( ${ICON_INSTALL_DIR}  crystalsvg )
