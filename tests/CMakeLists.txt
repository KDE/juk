include(ECMAddTests)

# Simple unit tests of guessing tags from a filename
ecm_add_test("${CMAKE_SOURCE_DIR}/tagguesser.cpp" tagguessertest.cpp
    LINK_LIBRARIES Qt5::Test KF5::ConfigCore KF5::CoreAddons
    TEST_NAME tagguessertest)
target_include_directories(tagguessertest PRIVATE ${CMAKE_SOURCE_DIR})
