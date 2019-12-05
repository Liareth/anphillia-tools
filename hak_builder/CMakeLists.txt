add_executable(hak_builder Main.cpp)
target_link_libraries(hak_builder FileFormats)

if (UNIX)
    target_link_libraries(hak_builder stdc++fs)
endif()
