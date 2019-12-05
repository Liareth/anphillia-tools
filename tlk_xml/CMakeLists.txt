add_executable(tlk_xml Main.cpp)
target_link_libraries(tlk_xml FileFormats tinyxml2)

if (UNIX)
    target_link_libraries(tlk_xml stdc++fs)
endif()

