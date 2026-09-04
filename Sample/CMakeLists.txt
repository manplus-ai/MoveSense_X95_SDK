file(GLOB Sample_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(Sample ${Sample_SRC})
set_target_properties(Sample PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(NOT WIN32)
    set_target_properties(Sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
target_include_directories(Sample PRIVATE ${ROOT}/include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(Sample Simou3CameraSDK ${OpenCV_LIBS})

if(WIN32)
    target_link_libraries(Sample ws2_32)
    add_custom_command(TARGET Sample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Simou3CameraSDK> $<TARGET_FILE_DIR:Sample>)
endif()
