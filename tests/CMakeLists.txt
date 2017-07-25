set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### next target ###############

include(ECMAddTests)

set(tagguessertest_SRCS tagguessertest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../tagguesser.cpp )

add_executable(tagguessertest ${tagguessertest_SRCS})
add_test(tagguesser tagguessertest)
ecm_mark_as_test(tagguessertest)

target_link_libraries(tagguessertest ${KDE4_KDECORE_LIBS} Qt5::Test)
