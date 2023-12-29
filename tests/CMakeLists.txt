include(ECMAddTests)

# Simple unit tests of guessing tags from a filename
ecm_add_test("${CMAKE_SOURCE_DIR}/tagguesser.cpp" tagguessertest.cpp
    LINK_LIBRARIES Qt6::Test KF6::ConfigCore KF6::CoreAddons
    TEST_NAME tagguessertest)
target_include_directories(tagguessertest PRIVATE ${CMAKE_SOURCE_DIR})
