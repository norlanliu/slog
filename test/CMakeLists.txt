cmake_minimum_required (VERSION 2.8)

project (slog)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++0x -Wall -DBOOST_LOG_DYN_LINK")

include_directories(${PROJECT_SOURCE_DIR}/../include)
################################
# Testing
################################
# This adds another subdirectory, which has 'project(gtest)'.
add_subdirectory(lib/gtest-1.6.0)

enable_testing()

# Include the gtest library. gtest_SOURCE_DIR is available due to
# 'project(gtest)' above.
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

##############
# Unit Tests
##############
add_executable(test_initialization test_initialization.cc)

add_test(Test-Slogger-Initialization test_initialization)

add_executable(test_slogger test_slogger.cc)

add_test(Test-Slogger test_slogger)

#####################
# link boost libraries
#####################
find_library (BOOST_THREAD_LIBRARY
boost_thread)
target_link_libraries (test_slogger
${BOOST_THREAD_LIBRARY})
find_library (BOOST_SYSTEM_LIBRARY
boost_system)
target_link_libraries (test_slogger
${BOOST_SYSTEM_LIBRARY})
find_library (BOOST_FILESYSTEM_LIBRARY
boost_filesystem)
target_link_libraries (test_slogger
${BOOST_FILESYSTEM_LIBRARY})
find_library (BOOST_LOG_LIBRARY boost_log)
target_link_libraries (test_slogger
${BOOST_LOG_LIBRARY})
target_link_libraries (test_slogger
gtest)
find_library (BOOST_DATE_TIME_LIBRARY boost_date_time)
target_link_libraries (test_slogger
${BOOST_DATE_TIME_LIBRARY})

target_link_libraries (test_initialization
${BOOST_THREAD_LIBRARY})
target_link_libraries (test_initialization
${BOOST_SYSTEM_LIBRARY})
target_link_libraries (test_initialization
${BOOST_FILESYSTEM_LIBRARY})
target_link_libraries (test_initialization
${BOOST_LOG_LIBRARY})
target_link_libraries (test_initialization
gtest)

