cmake_minimum_required(VERSION 3.20)
project(MarketDataFeedHandler CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /O2)
    add_definitions(-D_WIN32_WINNT=0x0601)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(feed_simulator
    src/feed_simulator_standalone.cpp
    src/message_format.cpp
)

add_executable(feed_handler
    src/feed_handler_standalone.cpp
    src/message_format.cpp
    src/metrics.cpp
)

if(WIN32)
    target_link_libraries(feed_simulator ws2_32)
    target_link_libraries(feed_handler ws2_32)
endif()

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
    
    add_executable(unit_tests
        tests/test_ring_buffer.cpp
        tests/test_message_format.cpp
        tests/test_metrics.cpp
        src/message_format.cpp
        src/metrics.cpp
    )
    target_link_libraries(unit_tests gtest_main)
    
    add_test(NAME unit_tests COMMAND unit_tests)
endif()
