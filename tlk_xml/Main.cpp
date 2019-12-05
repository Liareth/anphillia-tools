#include "FileFormats/Tlk.hpp"
#include "tinyxml2.h"

#include <cstdio>
#include <filesystem>
#include <string>

using namespace FileFormats::Tlk;
using namespace tinyxml2;

namespace {

bool read_from_xml(std::filesystem::path file, Friendly::Tlk* out)
{
    XMLDocument doc;
    if (doc.LoadFile(file.string().c_str()) != XML_SUCCESS) return false;
    XMLElement* tlk = doc.FirstChildElement("Tlk");
    out->SetLanguageId(tlk->UnsignedAttribute("LanguageId"));

    XMLElement* entry = tlk->FirstChildElement("Entry");

    while (entry)
    {
        Friendly::StrRef strref = entry->UnsignedAttribute("StrRef");

        Friendly::TlkEntry tlk_entry;

        if (XMLElement* inner_child = entry->FirstChildElement("String"))
        {
            if (const char* text = inner_child->GetText())
            {
                tlk_entry.m_String = text;
            }
        }

        if (XMLElement* inner_child = entry->FirstChildElement("SoundResRef"))
        {
            if (const char* text = inner_child->GetText())
            {
                tlk_entry.m_SoundResRef = text;
            }
        }

        if (XMLElement* inner_child = entry->FirstChildElement("SoundLength"))
        {
            tlk_entry.m_SoundLength = inner_child->FloatText();
        }

        out->Set(strref, std::move(tlk_entry));

        entry = entry->NextSiblingElement("Entry");
    }

    return true;
}

bool read_from_tlk(std::filesystem::path file, Friendly::Tlk* out)
{
    Raw::Tlk tlk_raw;
    if (!Raw::Tlk::ReadFromFile(file.string().c_str(), &tlk_raw)) return false;
    *out = Friendly::Tlk(std::move(tlk_raw));
    return true;
}

bool write_to_xml(std::filesystem::path file, const Friendly::Tlk* in)
{
    XMLDocument doc;
    XMLElement* root = doc.NewElement("Tlk");
    root->SetAttribute("Version", 1);
    root->SetAttribute("LanguageId", in->GetLanguageId());

    for (const auto& kvp : *in)
    {
        XMLElement* entry = doc.NewElement("Entry");
        entry->SetAttribute("StrRef", kvp.first);

        if (kvp.second.m_String)
        {
            XMLElement* inner_entry = doc.NewElement("String");
            inner_entry->SetText(kvp.second.m_String->c_str());
            entry->InsertEndChild(inner_entry);
        }

        if (kvp.second.m_SoundResRef)
        {
            XMLElement* inner_entry = doc.NewElement("SoundResRef");
            inner_entry->SetText(kvp.second.m_SoundResRef->c_str());
            entry->InsertEndChild(inner_entry);
        }

        if (kvp.second.m_SoundLength)
        {
            XMLElement* inner_entry = doc.NewElement("SoundLength");
            inner_entry->SetText(*kvp.second.m_SoundLength);
            entry->InsertEndChild(inner_entry);
        }

        root->InsertEndChild(entry);
    }

    doc.InsertFirstChild(root);
    return doc.SaveFile(file.string().c_str()) == XML_SUCCESS;
}

bool write_to_tlk(std::filesystem::path file, const Friendly::Tlk* in)
{
    return in->WriteToFile(file.string().c_str());
}
bool convert_file(std::filesystem::path path_in, std::filesystem::path path_out)
{
    bool read_xml = path_in.extension() == ".xml";
    bool write_xml = path_out.extension() == ".xml";

    std::printf("Processing %s -> %s.\n", path_in.string().c_str(), path_out.string().c_str());

    Friendly::Tlk tlk;

    if (bool read_success = read_xml ? read_from_xml(path_in, &tlk) : read_from_tlk(path_in, &tlk); !read_success)
    {
        std::printf("Failed to read.\n");
        return false;
    }

    if (bool write_success = write_xml ? write_to_xml(path_out, &tlk) : write_to_tlk(path_out, &tlk); !write_success)
    {
        std::printf("Failed to write.\n");
        return false;
    }

    return true;
}

}

int main(int argc, char** argv)
{
    return !convert_file(argv[2], argv[1]);
}
