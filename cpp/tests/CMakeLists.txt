enable_testing()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")

add_executable(tests ${sources})

target_include_directories(tests
    PUBLIC ../third_party/Indigo/api/c/indigo-inchi
    PUBLIC ../third_party/Indigo/third_party/googletest/include
)

target_link_libraries(tests qtrlib indigo-cpp gtest  glog::glog)
target_link_libraries(tests absl::flags absl::flags_parse)

include(GoogleTest)
gtest_discover_tests(tests)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/SetOutputDirectory.cmake)
set_output_directory(tests)