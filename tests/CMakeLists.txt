searchfiles()

include_directories (${PROJECT_SOURCE_DIR}/osort)
include_directories (${OPENSSL_INCLUDE_DIR})

add_executable(
  test_algo
  sort.cpp
)
target_compile_definitions(test_algo PRIVATE BOOST_STACKTRACE_USE_ADDR2LINE)
target_link_libraries(
  test_algo
  dl
  gtest_main
  common
)


add_executable(
  test_basic_perf
  perf_sort.cpp
)
target_compile_definitions(test_basic_perf PRIVATE BOOST_STACKTRACE_USE_ADDR2LINE)
target_link_libraries(
  test_basic_perf
  dl
  gtest_main
  common
)

add_executable(
  test_apps
  apps.cpp
)
target_compile_definitions(test_apps PRIVATE BOOST_STACKTRACE_USE_ADDR2LINE)
target_link_libraries(
  test_apps
  dl
  gtest_main
  common
)

include(GoogleTest)
gtest_discover_tests(test_algo)
gtest_discover_tests(test_basic_perf)
gtest_discover_tests(test_apps)
