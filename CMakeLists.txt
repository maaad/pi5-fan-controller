cmake_minimum_required(VERSION 3.10)
project(pi5_fan_controller)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific optimizations
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        # Architecture-specific optimizations
        add_compile_options(-march=native -mtune=native)

        # Link Time Optimization
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            # Aggressive optimizations for Release
            add_compile_options(
                -O3
                -DNDEBUG
                -flto
                -funroll-loops
                -finline-functions
                -fomit-frame-pointer
                -fno-strict-aliasing
                -ffunction-sections
                -fdata-sections
            )
            # Linker optimizations
            add_link_options(
                -flto
                -Wl,--gc-sections
                -Wl,--as-needed
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            # Balanced optimizations with debug info
            add_compile_options(
                -O2
                -DNDEBUG
                -g
                -flto
            )
            add_link_options(-flto)
        endif()
    endif()
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/fan_controller.cpp
    src/config_parser.cpp
    src/main.cpp
)

set(HEADERS
    include/fan_controller.hpp
    include/config_parser.hpp
)

# Executable
add_executable(pi5_fan_controller ${SOURCES} ${HEADERS})

target_include_directories(pi5_fan_controller PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Install executable
install(TARGETS pi5_fan_controller
    RUNTIME DESTINATION bin
)

# Install systemd service
install(FILES systemd/pi5-fan-controller.service
    DESTINATION /etc/systemd/system
)

# Install config file
install(FILES config/pi5-fan-controller.conf
    DESTINATION /etc/pi5-fan-controller
)

