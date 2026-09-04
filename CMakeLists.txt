cmake_minimum_required(VERSION 3.10)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(Simou3CameraSDK VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
set(ROOT ${CMAKE_SOURCE_DIR})

add_library(Simou3CameraSDK SHARED
    src/Simou3Camera.cpp
    src/Simou3Net.cpp
    src/Simou3CameraTransfer.cpp
    src/Simou3CameraSettings.cpp
    src/Simou3CameraScan.cpp
    src/Simou3Version.cpp
    src/crc.cpp
)
target_include_directories(Simou3CameraSDK PUBLIC ${ROOT}/include PRIVATE ${ROOT}/src)
set_target_properties(Simou3CameraSDK PROPERTIES
    OUTPUT_NAME "Simou3CameraSDK"
    DEBUG_POSTFIX "D"
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

target_compile_definitions(Simou3CameraSDK PRIVATE
    SIMOU3_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    SIMOU3_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    SIMOU3_VERSION_PATCH=${PROJECT_VERSION_PATCH})

if(MSVC)
    set_property(TARGET Simou3CameraSDK PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(UNIX)
    target_link_libraries(Simou3CameraSDK pthread)
    set_target_properties(Simou3CameraSDK PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc")
endif()

find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found (${OpenCV_VERSION}); building Sample sample.")
    add_subdirectory(Sample)
else()
    message(WARNING "OpenCV not found; skipping Sample sample. The SDK library itself has no OpenCV dependency and will still build.")
endif()

message(STATUS "system name: ${CMAKE_SYSTEM_NAME}   build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "make out: ${CMAKE_BINARY_DIR}")
