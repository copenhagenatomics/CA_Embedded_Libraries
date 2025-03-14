####################################################################################################
## Required to install gtest dependency
####################################################################################################

cmake_minimum_required(VERSION 3.14)
project(unit_testing)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set timestamp policy to avoid warning (default value)
if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW)
	set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

####################################################################################################
## Setup source code locations / include locations
####################################################################################################

set(SRC ../../STM32)
set(LIB ../../STM32)
set(INC_LIB ${LIB}/TransformationFunctions/Inc)

####################################################################################################
## List of tests to run
###################################################################################################

enable_testing()

include(GoogleTest)

# Goertzel tests
add_executable(goertzel_test goertzel_tests.cpp ${SRC}/TransformationFunctions/Src/goertzel.c)
target_include_directories(goertzel_test PRIVATE ${INC_LIB})
target_link_libraries(goertzel_test GTest::gtest_main gmock_main)
target_compile_options(goertzel_test PRIVATE -Wall)
gtest_discover_tests(goertzel_test)

# Hanning tests
add_executable(hanning_test hanning_tests.cpp ${SRC}/TransformationFunctions/Src/hanning.c)
target_include_directories(hanning_test PRIVATE ${INC_LIB})
target_link_libraries(hanning_test GTest::gtest_main gmock_main)
target_compile_options(hanning_test PRIVATE -Wall)
gtest_discover_tests(hanning_test)