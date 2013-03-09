set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### next target ###############

set(tagguessertest_SRCS tagguessertest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../tagguesser.cpp )

kde4_add_unit_test(tagguessertest ${tagguessertest_SRCS})

target_link_libraries(tagguessertest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})
