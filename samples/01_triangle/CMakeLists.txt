# Triangle Demo

add_executable(TriangleDemo
    main.cpp
    TriangleApp.h
    TriangleApp.cpp
    TrianglePass.h
    TrianglePass.cpp
)

target_link_libraries(TriangleDemo PRIVATE
    HappyCatEngine
)

# Copy shaders
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../shaders/triangle
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

set_target_properties(TriangleDemo PROPERTIES FOLDER "HappyCat/Samples")
