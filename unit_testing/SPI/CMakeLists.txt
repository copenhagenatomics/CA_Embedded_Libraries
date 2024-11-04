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
set(INC_LIB ${LIB}/SPI/Inc ${LIB}/SPI/Src ${LIB}/Util/Inc)
set(UT_FAKES ../fakes)
set(UT_STUBS ../stubs)
set(DRIVERS ../../STM32/Drivers/STM32F4xx_HAL_Driver/Inc)
set(CMSIS ../../STM32/Drivers/CMSIS/Device/ST/STM32F4xx/Include)
set(CMSIS_DRIVER ../../STM32/Drivers/CMSIS/Include)

####################################################################################################
## List of tests to run
###################################################################################################

enable_testing()

include(GoogleTest)

# LTC2512-24 tests
add_executable(ltc2512_test ltc2512_tests.cpp ${UT_FAKES}/fake_stm32xxxx_hal.cpp ${UT_FAKES}/fake_StmGpio.cpp)
target_include_directories(ltc2512_test PRIVATE ${UT_FAKES} ${UT_STUBS} ${INC_LIB} ${DRIVERS} ${CMSIS} ${CMSIS_DRIVER} Inc)
target_link_libraries(ltc2512_test GTest::gtest_main gmock_main)
target_compile_definitions(ltc2512_test PUBLIC __LIBRARY_TEST)
gtest_discover_tests(ltc2512_test)
