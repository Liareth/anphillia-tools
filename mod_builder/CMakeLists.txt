add_executable(mod_builder Main.cpp)
target_link_libraries(mod_builder FileFormats)

if (UNIX)
    target_link_libraries(mod_builder stdc++fs)
endif()

