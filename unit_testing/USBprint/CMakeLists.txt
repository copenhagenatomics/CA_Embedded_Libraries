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

set(SRC ../../STM32/USBprint/Src)
set(LIB ../../STM32)
set(INC_LIB ${LIB}/USBprint/Inc ${LIB}/USBprint/Src ${LIB}/circularBuffer/Inc)
set(UT_FAKES ../fakes)
set(UT_STUBS ../stubs)
set(UT_REDIRECTS ../redirects)
set(DRIVERS ../../STM32/FirmwarePackages/STM32F401CCUx/Drivers/STM32F4xx_HAL_Driver/Inc)
set(CMSIS ../../STM32/FirmwarePackages/STM32F401CCUx/Drivers/CMSIS/Device/ST/STM32F4xx/Include)


####################################################################################################
## List of tests to run
###################################################################################################

enable_testing()

include(GoogleTest)

# USBprint tests
add_executable(usbprint_test usbprint_tests.cpp ${LIB}/circularBuffer/Src/circular_buffer.c ${UT_FAKES}/fake_stm32xxxx_hal.cpp ${UT_FAKES}/fake_usbd_cdc.cpp)
target_include_directories(usbprint_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${UT_REDIRECTS} ${INC_LIB} ${DRIVERS} ${CMSIS} Inc)
target_link_libraries(usbprint_test GTest::gtest_main gmock_main)
target_compile_definitions(usbprint_test PUBLIC UNIT_TESTING)
target_compile_options(usbprint_test PRIVATE -Wall)
gtest_discover_tests(usbprint_test)
