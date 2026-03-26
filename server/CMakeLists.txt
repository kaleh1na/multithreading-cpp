cmake_minimum_required(VERSION 3.20)
project(psp_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
endif()

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/boost")
endif()

find_package(Boost 1.70 REQUIRED CONFIG)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

add_library(psp_core
    src/store/account_store.cpp
    src/store/transaction_store.cpp
    src/service/payment_service.cpp
    src/worker/task_queue.cpp
)
target_include_directories(psp_core PUBLIC src)
target_link_libraries(psp_core PUBLIC Threads::Threads)

add_library(psp_net
    src/net/request_handler.cpp
    src/net/session.cpp
    src/net/server.cpp
)
target_include_directories(psp_net PUBLIC src)
target_link_libraries(psp_net PUBLIC psp_core Boost::headers Threads::Threads)

add_executable(psp_server main.cpp)
target_link_libraries(psp_server psp_net)

enable_testing()
add_executable(psp_tests
    tests/test_account_store.cpp
    tests/test_payment_service.cpp
    tests/test_task_queue.cpp
    tests/test_request_handler.cpp
)
target_link_libraries(psp_tests psp_net GTest::gtest_main Threads::Threads)
add_test(NAME psp_tests COMMAND psp_tests)
