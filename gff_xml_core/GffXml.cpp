#include "GffXml.hpp"
#include "FileFormats/Gff.hpp"
#include "tinyxml2.h"

#include <algorithm>
#include <cstdio>
#include <string>

using namespace FileFormats::Gff;
using namespace tinyxml2;

namespace {

void write_to_xml_r(const Friendly::GffStruct& element, XMLDocument* doc, XMLElement* root, bool top_level = false, const char* element_name = nullptr);
void read_from_xml_r(XMLElement* parent, Friendly::GffStruct* struc);

bool read_from_xml(std::filesystem::path file, Friendly::Gff* out)
{
    XMLDocument doc;
    if (doc.LoadFile(file.string().c_str()) != XML_SUCCESS) return false;
    XMLElement* gff = doc.FirstChildElement("Gff");
    char* file_type = out->GetFileType();
    std::memcpy(file_type, gff->Attribute("Type"), 3);
    file_type[3] = ' ';
    read_from_xml_r(gff, &out->GetTopLevelStruct());
    return true;
}

bool read_from_gff(std::filesystem::path file, Friendly::Gff* out)
{
    Raw::Gff gff_raw;
    if (!Raw::Gff::ReadFromFile(file.string().c_str(), &gff_raw)) return false;
    *out = Friendly::Gff(std::move(gff_raw));
    return true;
}

bool write_to_xml(std::filesystem::path file, const Friendly::Gff* in)
{
    XMLDocument doc;
    XMLElement* root = doc.NewElement("Gff");
    root->SetAttribute("Version", 1);
    root->SetAttribute("Type", std::string(in->GetFileType(), 3).c_str());
    doc.InsertFirstChild(root);
    write_to_xml_r(in->GetTopLevelStruct(), &doc, root, true);
    return doc.SaveFile(file.string().c_str()) == XML_SUCCESS;
}

bool write_to_gff(std::filesystem::path file, const Friendly::Gff* in)
{
    return in->WriteToFile(file.string().c_str());
}

template <typename T>
XMLElement* create_generic_node(const char* type, const char* name, const T& value, XMLDocument* doc)
{
    XMLElement* new_node = doc->NewElement(type);
    new_node->SetText(value);
    new_node->SetAttribute("Name", name);
    return new_node;
}

void write_to_xml_r(const Friendly::GffStruct& element, XMLDocument* doc, XMLElement* root, bool top_level, const char* element_name)
{
    XMLElement* new_root = top_level ? nullptr : doc->NewElement("Struct");

    if (new_root)
    {
        if (element_name)
        {
            new_root->SetAttribute("Name", element_name);
        }

        new_root->SetAttribute("Id", element.GetUserDefinedId());
        std::swap(root, new_root);
    }

    for (auto const& kvp : element.GetFields())
    {
        if (kvp.second.first == Raw::GffField::Type::BYTE)
        {
            Friendly::Type_BYTE value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("Byte", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::CHAR)
        {
            Friendly::Type_CHAR value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("Char", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::WORD)
        {
            Friendly::Type_WORD value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("Word", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::SHORT)
        {
            Friendly::Type_SHORT value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("Short", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::DWORD)
        {
            Friendly::Type_DWORD value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("DWord", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::INT)
        {
            Friendly::Type_INT value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("Int", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::DWORD64)
        {
            Friendly::Type_DWORD64 value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("DWord64", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::INT64)
        {
            Friendly::Type_INT64 value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("Int64", kvp.first.c_str(), value, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::FLOAT)
        {
            Friendly::Type_FLOAT value;
            element.ReadField(kvp, &value);
            char flt[64];
            std::sprintf(flt, "%f", value);
            root->InsertEndChild(create_generic_node("Float", kvp.first.c_str(), flt, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::DOUBLE)
        {
            Friendly::Type_DOUBLE value;
            element.ReadField(kvp, &value);
            char dbl[64];
            std::sprintf(dbl, "%f", value);
            root->InsertEndChild(create_generic_node("Double", kvp.first.c_str(), dbl, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::CExoString)
        {
            Friendly::Type_CExoString value;
            element.ReadField(kvp, &value);
            root->InsertEndChild(create_generic_node("CExoString", kvp.first.c_str(), value.m_String.c_str(), doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::ResRef)
        {
            Friendly::Type_CResRef value;
            element.ReadField(kvp, &value);
            char resref[32];
            std::sprintf(resref, "%.*s", value.m_Size, value.m_String);
            root->InsertEndChild(create_generic_node("ResRef", kvp.first.c_str(), resref, doc));
        }
        else if (kvp.second.first == Raw::GffField::Type::CExoLocString)
        {
            Friendly::Type_CExoLocString value;
            element.ReadField(kvp, &value);

            XMLElement* new_node = doc->NewElement("CExoLocString");
            new_node->SetAttribute("Name", kvp.first.c_str());
            new_node->InsertEndChild(create_generic_node("DWord", "StringRef", value.m_StringRef, doc));

            for (std::size_t i = 0; i < value.m_SubStrings.size(); ++i)
            {
                XMLElement* new_nod_substr = doc->NewElement("SubString");
                new_nod_substr->InsertEndChild(create_generic_node("Int", "StringID", value.m_SubStrings[i].m_StringID, doc));
                new_nod_substr->InsertEndChild(create_generic_node("CExoString", "String", value.m_SubStrings[i].m_String.c_str(), doc));
                new_node->InsertEndChild(new_nod_substr);
            }

            root->InsertEndChild(new_node);
        }
        else if (kvp.second.first == Raw::GffField::Type::VOID)
        {
            std::printf("VOID data detected in %s. Dropping.\n", kvp.first.c_str());
        }
        else if (kvp.second.first == Raw::GffField::Type::Struct)
        {
            Friendly::Type_Struct value;
            element.ReadField(kvp, &value);
            write_to_xml_r(value, doc, root, false, kvp.first.c_str());
        }
        else if (kvp.second.first == Raw::GffField::Type::List)
        {
            Friendly::Type_List value;
            element.ReadField(kvp, &value);

            XMLElement* new_node = doc->NewElement("List");
            new_node->SetAttribute("Name", kvp.first.c_str());

            for (const Friendly::GffStruct& struc : value.GetStructs())
            {
                write_to_xml_r(struc, doc, new_node);
            }

            root->InsertEndChild(new_node);
        }
        else
        {
            ASSERT_FAIL();
        }
    }

    if (new_root)
    {
        new_root->InsertEndChild(root);
    }
}

void read_from_xml_r(XMLElement* parent, Friendly::GffStruct* struc)
{
    XMLElement* element = parent->FirstChildElement();

    while (element)
    {
        const char* name = element->Name();

        if (std::strcmp(name, "Byte") == 0)
        {
            Friendly::Type_BYTE value = (Friendly::Type_BYTE)std::stoul(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "Char") == 0)
        {
            Friendly::Type_CHAR value = (Friendly::Type_CHAR)std::stoi(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "Word") == 0)
        {
            Friendly::Type_WORD value = (Friendly::Type_WORD)std::stoul(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "Short") == 0)
        {
            Friendly::Type_SHORT value = (Friendly::Type_SHORT)std::stoi(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "DWord") == 0)
        {
            Friendly::Type_DWORD value = std::stoul(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "Int") == 0)
        {
            Friendly::Type_INT value = std::stoi(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "DWord64") == 0)
        {
            Friendly::Type_DWORD64 value = std::stoull(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));;
        }
        else if (std::strcmp(name, "Int64") == 0)
        {
            Friendly::Type_INT64 value = std::stoll(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "Float") == 0)
        {
            Friendly::Type_FLOAT value = std::stof(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "Double") == 0)
        {
            Friendly::Type_DOUBLE value = std::stod(element->GetText());
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "CExoString") == 0)
        {
            Friendly::Type_CExoString value;
            value.m_String = element->GetText() ? element->GetText() : "";
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "ResRef") == 0)
        {
            Friendly::Type_CResRef value;

            if (const char* text = element->GetText(); text)
            {
                value.m_Size = (uint8_t)std::strlen(text);
                memcpy(value.m_String, text, value.m_Size);
            }
            else
            {
                value.m_Size = 0;
            }

            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "CExoLocString") == 0)
        {
            Friendly::Type_CExoLocString value;
            value.m_TotalSize = sizeof(value.m_StringRef) + sizeof(std::uint32_t); // string count
            XMLElement* child = element->FirstChildElement();

            while (child)
            {
                if (std::strcmp(child->Name(), "SubString") == 0)
                {
                    Friendly::Type_CExoLocString::SubString ss;
                    ss.m_StringID = std::stol(child->FirstChildElement("Int")->GetText());
                    const char* str = child->FirstChildElement("CExoString")->GetText();
                    ss.m_String = str ? str : "";
                    value.m_TotalSize += sizeof(ss.m_StringID) + sizeof(std::uint32_t) + (std::uint32_t)ss.m_String.size();
                    value.m_SubStrings.emplace_back(std::move(ss));
                }
                else
                {
                    value.m_StringRef = std::stoul(child->GetText());
                }

                child = child->NextSiblingElement();
            }

            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "Struct") == 0)
        {
            Friendly::Type_Struct value;
            read_from_xml_r(element, &value);
            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));
        }
        else if (std::strcmp(name, "List") == 0)
        {
            Friendly::Type_List value;
            XMLElement* child = element->FirstChildElement();

            while (child)
            {
                Friendly::GffStruct child_struc;
                read_from_xml_r(child, &child_struc);
                value.GetStructs().emplace_back(std::move(child_struc));
                child = child->NextSiblingElement();
            }

            struc->WriteField(element->FindAttribute("Name")->Value(), std::move(value));

        }
        else
        {
            ASSERT_FAIL();
        }

        element = element->NextSiblingElement();
    }

    struc->SetUserDefinedId(parent->IntAttribute("Id"));
}

}

bool convert_file(std::filesystem::path path_in, std::filesystem::path path_out)
{
    bool read_xml = path_in.extension() == ".xml";
    bool write_xml = path_out.extension() == ".xml";

    std::printf("Processing %s -> %s.\n", path_in.string().c_str(), path_out.string().c_str());

    Friendly::Gff gff;

    if (bool read_success = read_xml ? read_from_xml(path_in, &gff) : read_from_gff(path_in, &gff); !read_success)
    {
        std::printf("Failed to read.\n");
        return false;
    }

    if (path_out.extension() == ".?")
    {
        std::string new_ext = std::string(gff.GetFileType(), 3);
        std::transform(std::begin(new_ext), std::end(new_ext), std::begin(new_ext), ::tolower);
        path_out.replace_extension(std::move(new_ext));
    }

    if (bool write_success = write_xml ? write_to_xml(path_out, &gff) : write_to_gff(path_out, &gff); !write_success)
    {
        std::printf("Failed to write.\n");
        return false;
    }

    return true;
}
