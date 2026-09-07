cmake_minimum_required(VERSION 3.10)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(MoveSense_X95_SDK VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Threads REQUIRED)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

add_library(MoveSense_X95_SDK SHARED
    src/Simou3Camera.cpp
    src/Simou3Net.cpp
    src/Simou3CameraTransfer.cpp
    src/Simou3CameraSettings.cpp
    src/Simou3CameraScan.cpp
    src/Simou3Version.cpp
    src/crc.cpp)
add_library(MoveSense::X95_SDK ALIAS MoveSense_X95_SDK)

target_include_directories(MoveSense_X95_SDK PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE src)

target_compile_definitions(MoveSense_X95_SDK PRIVATE
    SIMOU3_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    SIMOU3_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    SIMOU3_VERSION_PATCH=${PROJECT_VERSION_PATCH})

set_target_properties(MoveSense_X95_SDK PROPERTIES
    EXPORT_NAME X95_SDK
    DEBUG_POSTFIX D
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

target_link_libraries(MoveSense_X95_SDK PUBLIC Threads::Threads)

if(MSVC)
    set_property(TARGET MoveSense_X95_SDK PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

install(TARGETS MoveSense_X95_SDK EXPORT MoveSense_X95_SDKTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT MoveSense_X95_SDKTargets
    FILE MoveSense_X95_SDKTargets.cmake
    NAMESPACE MoveSense::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MoveSense_X95_SDK)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MoveSense_X95_SDKConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
configure_package_config_file(
    cmake/MoveSense_X95_SDKConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/MoveSense_X95_SDKConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MoveSense_X95_SDK)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MoveSense_X95_SDKConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MoveSense_X95_SDKConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MoveSense_X95_SDK)

if(NOT TARGET uninstall)
    configure_file(
        cmake/cmake_uninstall.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

option(MOVESENSE_BUILD_SAMPLE "Build the Sample example (requires OpenCV)" OFF)
if(MOVESENSE_BUILD_SAMPLE)
    find_package(OpenCV QUIET)
    if(OpenCV_FOUND)
        add_subdirectory(Sample)
    else()
        message(WARNING "MOVESENSE_BUILD_SAMPLE is ON but OpenCV was not found; skipping the Sample example.")
    endif()
endif()
