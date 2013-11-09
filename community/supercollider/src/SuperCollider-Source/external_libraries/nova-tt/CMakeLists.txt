PROJECT(nova-tt)
cmake_minimum_required(VERSION 2.6)

set(Boost_USE_MULTITHREADED      ON)
find_package( Boost 1.50.0 COMPONENTS  thread system test_exec_monitor )

enable_testing()

include_directories (boost_lockfree)
include_directories (nova-tt)

find_library(pthread pthread)
add_subdirectory (testsuite)
