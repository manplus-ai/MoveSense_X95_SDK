add_executable(Sample
    main.cpp
    SampleApp.cpp
    CameraSession.cpp
    FrameReceiver.cpp
    ImuReceiver.cpp
    ModeCatalog.cpp)
set_target_properties(Sample PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(NOT WIN32)
    set_target_properties(Sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
target_include_directories(Sample PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(Sample PRIVATE MoveSense::X95_SDK ${OpenCV_LIBS})

if(WIN32)
    add_custom_command(TARGET Sample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:MoveSense_X95_SDK> $<TARGET_FILE_DIR:Sample>)
endif()
