# to generate project for Eclipse.
# cmake -G "Eclipse CDT4 - Unix Makefiles" ./

cmake_minimum_required(VERSION 3.13)

project(CA_UnitTesting)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-O0 -g3")
set(CMAKE_C_FLAGS "-O0 -g3")

# Create executables for each test file
set(TARGET_NAMES
    ADC_TEST
    GOERTZEL_TEST
)

# Create an includes list per executable separated by line breaks and encapsulated in quotes
set(TARGET_INCLUDES_LIST
    "Inc ../ADCMonitor/Inc ../Util/Inc ../USBprint/Inc ../FLASH_readwrite/Inc"
    "../TransformationFunctions/Inc"
)

# Create an source list per executable separated by line breaks and encapsulated in quotes
set(TARGET_SOURCES_LIST
    "src/stubs.cpp ../ADCMonitor/Src/ADCmonitor.c ../Util/Src/CAProtocol.c"
    "src/testGoertzel.cpp ../TransformationFunctions/Src/goertzel.c"
)

list(LENGTH TARGET_NAMES TARGET_COUNT)
math(EXPR TARGET_COUNT_MINUS_ONE "${TARGET_COUNT} - 1")

foreach(target_index RANGE ${TARGET_COUNT_MINUS_ONE})

    list(GET TARGET_NAMES ${target_index} target_name)
    list(GET TARGET_SOURCES_LIST ${target_index} target_sources)
    list(GET TARGET_INCLUDES_LIST ${target_index} target_includes)

    # Split target_includes and target_sources by spaces
    separate_arguments(target_sources_list UNIX_COMMAND "${target_sources}")
    separate_arguments(target_includes_list UNIX_COMMAND "${target_includes}")

    include_directories(${target_includes_list})
    add_compile_definitions(STM32F401xC)

    enable_testing()
    # Add target
    add_executable(${target_name} ${target_sources_list})
    target_link_libraries(${target_name} PRIVATE m)
endforeach()