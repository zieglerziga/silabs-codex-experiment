cmake_minimum_required(VERSION 3.25)

project(silabs_ble_scanner_host LANGUAGES C)

include(CTest)

add_library(scan_tracker STATIC
  firmware/ble_scanner/src/scan_tracker.c
)

target_include_directories(scan_tracker PUBLIC
  firmware/ble_scanner/inc
)

target_compile_features(scan_tracker PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(scan_tracker PRIVATE
    -Wall
    -Wextra
    -Wconversion
    -Werror
    -Wpedantic
  )
endif()

if(BUILD_TESTING)
  add_executable(scan_tracker_tests
    tests/scan_tracker_tests.c
  )
  target_link_libraries(scan_tracker_tests PRIVATE scan_tracker)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(scan_tracker_tests PRIVATE
      -Wall
      -Wextra
      -Wconversion
      -Werror
      -Wpedantic
    )
  endif()
  add_test(NAME scan_tracker_tests COMMAND scan_tracker_tests)

  add_executable(rtt_command_tests
    tests/rtt_command_tests.c
  )
  target_include_directories(rtt_command_tests PRIVATE
    firmware/ble_scanner/inc
  )
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rtt_command_tests PRIVATE
      -Wall
      -Wextra
      -Wconversion
      -Werror
      -Wpedantic
    )
  endif()
  add_test(NAME rtt_command_tests COMMAND rtt_command_tests)
endif()
