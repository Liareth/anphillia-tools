add_library(gff_xml_core STATIC GffXml.cpp GffXml.hpp)
target_link_libraries(gff_xml_core FileFormats tinyxml2)

if (UNIX)
    target_link_libraries(gff_xml_core stdc++fs)
endif()
