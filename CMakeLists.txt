cmake_minimum_required(VERSION 3.8)
project(anphtools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(external/NWNFileFormats)
add_subdirectory(external/tinyxml2)
add_subdirectory(external/NWNFileFormats)

add_subdirectory(gff_xml)
add_subdirectory(gff_xml_core)
add_subdirectory(gff_xml_packer)
add_subdirectory(hak_builder)
add_subdirectory(mod_builder)
add_subdirectory(tlk_xml)
