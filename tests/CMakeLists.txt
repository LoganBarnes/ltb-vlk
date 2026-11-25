# ##############################################################################
# A Logan Thomas Barnes project
# ##############################################################################
include(GoogleTest)

# Creates one test executable from every `*_tests.cpp` file under `directory`
# and registers each test case with CTest under the given label.
#
#   unit: CPU only, always runnable.            `ctest -L unit`
#   gpu:  needs a Vulkan device (skips if none). `ctest -L gpu`
function(ltb_vlk_add_test_suite directory label)
  file(
    GLOB_RECURSE
    ${directory}_TEST_SOURCES
    LIST_DIRECTORIES
    false
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/${directory}/*_tests.cpp
  )

  set(target ltb_vlk_${directory}_tests)

  add_executable(
    ${target}
    ${${directory}_TEST_SOURCES}
  )
  target_link_libraries(
    ${target}
    PRIVATE
    LtbVlk::LtbVlk
    LtbVlk::warnings
    GTest::gtest_main
  )
  target_include_directories(
    ${target}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/${directory}
  )
  gtest_discover_tests(
    ${target}
    PROPERTIES
    LABELS
    ${label}
  )
endfunction()

ltb_vlk_add_test_suite(unit unit)
ltb_vlk_add_test_suite(gpu gpu)
