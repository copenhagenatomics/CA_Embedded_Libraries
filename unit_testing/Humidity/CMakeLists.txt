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

set(INC_LIB ${LIB}/Humidity/Inc
    ${LIB}/Humidity/Src
    ${LIB}/I2C/Inc
    ${LIB}/I2C/Src
    ${LIB}/Crc/Inc
    ${LIB}/Crc/Src
    ${LIB}/Util/Inc
    ${LIB}/Util/Src
    ${LIB}/FLASH_readwrite/Inc
    ${LIB}/USBprint/Inc)

set(UT_FAKES ../fakes)
set(UT_STUBS ../stubs)
set(DRIVERS ../../STM32/FirmwarePackages/STM32F401CCUx/Drivers/STM32F4xx_HAL_Driver/Inc)
set(CMSIS ../../STM32/FirmwarePackages/STM32F401CCUx/Drivers/CMSIS/Device/ST/STM32F4xx/Include)

####################################################################################################
## List of tests to run
###################################################################################################

enable_testing()

include(GoogleTest)

# humiditySm tests
add_executable(humiditySm_tests humiditySm_tests.cpp
    ${UT_FAKES}/fake_stm32xxxx_hal.cpp
    ${UT_FAKES}/fake_HAL_otp.cpp)

target_include_directories(humiditySm_tests PRIVATE ${CMSIS}
    ${DRIVERS}
    ${INC_LIB}
    ${UT_FAKES}
    ${UT_STUBS}
    Inc)

target_link_libraries(humiditySm_tests GTest::gtest_main gmock_main)
target_compile_definitions(humiditySm_tests PUBLIC __LIBRARY_TEST UNIT_TESTING)
target_compile_options(humiditySm_tests PRIVATE -Wall)
gtest_discover_tests(humiditySm_tests)
