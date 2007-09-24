set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

# hack, should taglib maybe give the .. directory as well in it's config file?
add_definitions(${TAGLIB_CFLAGS} "${TAGLIB_CFLAGS}/..")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

   
########### next target ###############
set(tagguessertest_SRCS tagguessertest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../tagguesser.cpp )


kde4_add_executable(tagguessertest TEST ${tagguessertest_SRCS})

target_link_libraries(tagguessertest  ${KDE4_KDECORE_LIBS} )
