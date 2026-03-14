# PBR Material Demo

add_executable(PBRMaterialDemo
    main.cpp
    PBRMaterialApp.h
    PBRMaterialApp.cpp
)

target_link_libraries(PBRMaterialDemo PRIVATE
    HappyCatEngine
)

set_target_properties(PBRMaterialDemo PROPERTIES FOLDER "HappyCat/Samples")
