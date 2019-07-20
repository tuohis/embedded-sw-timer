include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# If pkg-config is not installed on the system, then the
# CHECK_INSTALL_DIR variable must be set to the install
# location of Check. For example, on Windows, this may
# be: C:/Program Files/check
# set(CHECK_INSTALL_DIR "C:/Program Files/check")

find_package(Check REQUIRED)
message("Check include dirs: '${CHECK_INCLUDE_DIRS}'")
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

set(TEST_SOURCES
  sw_timer_test.c
  hwtimer_mock.c
)

add_executable(sw_timer_test ${TEST_SOURCES})
target_link_libraries(sw_timer_test sw_timer ${CHECK_LIBRARIES} -pthread  -pthread -lcheck_pic -lrt -lm)

add_test(NAME sw_timer_test COMMAND sw_timer_test)
