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
set(INC_LIB ${LIB}/ADCMonitor/Inc ${LIB}/circularBuffer/Inc ${LIB}/Filtering/Inc 
            ${LIB}/FLASH_readwrite/Inc ${LIB}/I2C/Inc ${LIB}/jumpToBootloader/Inc 
            ${LIB}/Regulation/Inc ${LIB}/SPI/Inc ${LIB}/TransformationFunctions/Inc 
            ${LIB}/USBprint/Inc ${LIB}/Util/Inc)
set(UT_FAKES ../fakes)
set(UT_STUBS ../stubs)

####################################################################################################
## List of tests to run
###################################################################################################

enable_testing()

include(GoogleTest)

# Array math tests
add_executable(arraymath_test arraymath_tests.cpp ${SRC}/Filtering/Src/array-math.c)
target_include_directories(arraymath_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB})
target_link_libraries(arraymath_test GTest::gtest_main gmock_main)
gtest_discover_tests(arraymath_test)

# Lowpass filter tests
add_executable(lpfilter_test lowpassfilter_tests.cpp ${SRC}/Filtering/Src/lowpassFilter.c ${SRC}/Filtering/Src/array-math.c)
target_include_directories(lpfilter_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB})
target_link_libraries(lpfilter_test GTest::gtest_main gmock_main)
gtest_discover_tests(lpfilter_test)

# Notch filter tests
add_executable(notchfilter_test notchfilter_tests.cpp ${SRC}/Filtering/Src/notchFilter.c ${SRC}/Filtering/Src/array-math.c)
target_include_directories(notchfilter_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB})
target_link_libraries(notchfilter_test GTest::gtest_main gmock_main)
gtest_discover_tests(notchfilter_test)