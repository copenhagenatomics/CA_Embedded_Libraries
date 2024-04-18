####################################################################################################
## Required to install gtest dependency
####################################################################################################

cmake_minimum_required(VERSION 3.14)
project(unit_testing)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
set(INC_LIB ${LIB}/Util/Inc ${LIB}/Util/Src 
            ${LIB}/FLASH_readwrite/Inc ${LIB}/FLASH_readwrite/Src)
set(UT_FAKES ../fakes)
set(UT_STUBS ../stubs)


####################################################################################################
## List of tests to run
###################################################################################################

enable_testing()

include(GoogleTest)

# ADCMonitor tests
add_executable(caprotocol_test caprotocol_tests.cpp ${UT_STUBS}/stub_CAProtocolStm.cpp)
target_include_directories(caprotocol_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB})
target_link_libraries(caprotocol_test GTest::gtest_main gmock_main)
target_compile_definitions(caprotocol_test PUBLIC UNIT_TESTING)
gtest_discover_tests(caprotocol_test)
