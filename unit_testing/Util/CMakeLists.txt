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
set(INC_LIB ${LIB}/Util/Inc ${LIB}/Util/Src ${LIB}/Crc/Inc ${LIB}/jumpToBootloader/Inc 
            ${LIB}/FLASH_readwrite/Inc ${LIB}/USBprint/Inc Inc)
set(UT_FAKES ../fakes)
set(UT_STUBS ../stubs)
set(DRIVERS ../../STM32/FirmwarePackages/STM32F401CCUx/Drivers/STM32F4xx_HAL_Driver/Inc)
set(CMSIS   ../../STM32/FirmwarePackages/STM32F401CCUx/Drivers/CMSIS/Device/ST/STM32F4xx/Include)


####################################################################################################
## List of tests to run
###################################################################################################

enable_testing()

include(GoogleTest)

# CAProtocol tests
add_executable(caprotocol_test caprotocol_tests.cpp ${UT_STUBS}/stub_CAProtocolStm.cpp)
target_include_directories(caprotocol_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB})
target_link_libraries(caprotocol_test GTest::gtest_main gmock_main)
target_compile_definitions(caprotocol_test PUBLIC UNIT_TESTING)
target_compile_options(caprotocol_test PRIVATE -Wall)
gtest_discover_tests(caprotocol_test)

# Systeminfo tests
add_executable(systeminfo_test systeminfo_tests.cpp ${UT_FAKES}/fake_HAL_otp.cpp)
target_include_directories(systeminfo_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB})
target_link_libraries(systeminfo_test GTest::gtest_main gmock_main)
target_compile_definitions(systeminfo_test PUBLIC UNIT_TESTING)
target_compile_options(systeminfo_test PRIVATE -Wall)
gtest_discover_tests(systeminfo_test)

# Systeminfo tests
add_executable(uptime_test uptime_tests.cpp 
               ${UT_FAKES}/fake_FLASH_readwrite.cpp 
               ${UT_FAKES}/fake_stm32xxxx_hal.cpp
               ${UT_FAKES}/fake_USBprint.cpp
               ${UT_FAKES}/fake_HAL_otp.cpp
               ${LIB}/Util/Src/time32.c
               ${LIB}/Util/Src/systeminfo.c
               ${LIB}/Crc/Src/crc.c
               ${UT_STUBS}/stub_jumpToBootloader.cpp)
target_include_directories(uptime_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB} ${DRIVERS} ${CMSIS})
target_link_libraries(uptime_test GTest::gtest_main gmock_main)
target_compile_definitions(uptime_test PUBLIC UNIT_TESTING)
target_compile_options(uptime_test PRIVATE -Wall)
gtest_discover_tests(uptime_test)