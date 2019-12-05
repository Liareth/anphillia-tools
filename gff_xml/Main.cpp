#include "gff_xml_core/GffXml.hpp"

int main(int argc, char** argv)
{
    return !convert_file(argv[2], argv[1]);
}
