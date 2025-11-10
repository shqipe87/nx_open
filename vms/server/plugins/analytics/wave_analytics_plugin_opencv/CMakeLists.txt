# Copyright 2024. All Rights Reserved.
#
# CMakeLists.txt for Wave Analytics Plugin with OpenCV

cmake_minimum_required(VERSION 3.14)

project(wave_analytics_plugin_opencv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "OpenCV version: ${OpenCV_VERSION}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
    message(STATUS "OpenCV include path: ${OpenCV_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "OpenCV not found!")
endif()

# Plugin source files
set(WAVE_OPENCV_PLUGIN_SOURCES
    src/nx/vms_server_plugins/analytics/wave_opencv/integration.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/integration.h
    src/nx/vms_server_plugins/analytics/wave_opencv/engine.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/engine.h
    src/nx/vms_server_plugins/analytics/wave_opencv/device_agent.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/device_agent.h
    src/nx/vms_server_plugins/analytics/wave_opencv/types.h
    src/nx/vms_server_plugins/analytics/wave_opencv/config_loader.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/config_loader.h
    src/nx/vms_server_plugins/analytics/wave_opencv/object_detector.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/object_detector.h
    src/nx/vms_server_plugins/analytics/wave_opencv/object_tracker.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/object_tracker.h
    src/nx/vms_server_plugins/analytics/wave_opencv/behavioral_analyzer.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/behavioral_analyzer.h
    src/nx/vms_server_plugins/analytics/wave_opencv/crowd_analyzer.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/crowd_analyzer.h
    src/nx/vms_server_plugins/analytics/wave_opencv/queue_manager.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/queue_manager.h
    src/nx/vms_server_plugins/analytics/wave_opencv/rules_engine.cpp
    src/nx/vms_server_plugins/analytics/wave_opencv/rules_engine.h
)

# Check if we're building within the Nx/Wave build tree
if(COMMAND nx_add_server_plugin)
    # Building within Wave build system
    message(STATUS "Building Wave Analytics Plugin within Wave build tree")

    # Create static library for plugin logic
    nx_add_target(wave_analytics_plugin_opencv_lib LIBRARY
        LIBRARY_TYPE STATIC
        NO_MOC
        ADDITIONAL_SOURCES ${WAVE_OPENCV_PLUGIN_SOURCES}
        PUBLIC_LIBS
            nx_sdk
            ${OpenCV_LIBS}
        FOLDER server/plugins
    )

    # Create plugin shared library
    nx_add_server_plugin(wave_analytics_plugin_opencv
        NO_MOC
        OPTIONAL
        SOURCE_DIR "plugin"
        PRIVATE_LIBS wave_analytics_plugin_opencv_lib
    )

    target_include_directories(wave_analytics_plugin_opencv_lib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${OpenCV_INCLUDE_DIRS}
    )

else()
    # Building standalone
    message(STATUS "Building Wave Analytics Plugin standalone")

    # Find Nx SDK
    if(NOT DEFINED NX_SDK_DIR)
        set(NX_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/nx_sdk" CACHE PATH "Path to Nx SDK")
    endif()

    if(NOT EXISTS "${NX_SDK_DIR}")
        message(FATAL_ERROR "Nx SDK not found at ${NX_SDK_DIR}")
    endif()

    # Add Nx SDK
    add_subdirectory(${NX_SDK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/nx_sdk)

    # Create static library
    add_library(wave_analytics_plugin_opencv_lib STATIC
        ${WAVE_OPENCV_PLUGIN_SOURCES}
    )

    target_link_libraries(wave_analytics_plugin_opencv_lib
        PUBLIC
            nx_sdk
            ${OpenCV_LIBS}
    )

    target_include_directories(wave_analytics_plugin_opencv_lib
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${OpenCV_INCLUDE_DIRS}
    )

    # Create plugin shared library
    add_library(wave_analytics_plugin_opencv SHARED
        plugin/plugin.cpp
    )

    target_link_libraries(wave_analytics_plugin_opencv
        PRIVATE
            wave_analytics_plugin_opencv_lib
    )

    # Set output directory
    set_target_properties(wave_analytics_plugin_opencv PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
    )

    # Install target
    install(TARGETS wave_analytics_plugin_opencv
        LIBRARY DESTINATION lib/plugins
        RUNTIME DESTINATION bin/plugins
    )

    # Install configuration files
    install(DIRECTORY config/
        DESTINATION etc/wave_analytics
        FILES_MATCHING PATTERN "*.json"
    )

    # Install models directory
    install(DIRECTORY models/
        DESTINATION share/wave_analytics/models
        OPTIONAL
    )
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(wave_analytics_plugin_opencv_lib PRIVATE
        -Wall -Wextra -Wpedantic
    )
elseif(MSVC)
    target_compile_options(wave_analytics_plugin_opencv_lib PRIVATE
        /W4
    )
endif()

# CUDA support (optional)
find_package(CUDA QUIET)
if(CUDA_FOUND)
    message(STATUS "CUDA found: ${CUDA_VERSION}")
    target_compile_definitions(wave_analytics_plugin_opencv_lib PUBLIC HAVE_CUDA)
else()
    message(STATUS "CUDA not found - GPU acceleration will be unavailable")
endif()

message(STATUS "Wave Analytics Plugin configuration complete")
