cmake_minimum_required(VERSION 3.5.2 FATAL_ERROR)

project(Alchemy)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
include(GTestCMake/GTest.cmake)
# Was using "~" for my home directory, but CMake decided to stop liking that 
# at some point.  Using the "HOME" environment variable seems to be more 
# palatable for CMake.
#googletest_setup(URL "$ENV{HOME}/Projects/googletest/googletest-master.zip")
googletest_setup(URL "")



include_directories(${CMAKE_SOURCE_DIR})
googletest_add(
	fifobuff_test 
	fifobuff_test.cpp
)


