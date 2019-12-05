#include "FileFormats/Erf.hpp"
#include "FileFormats/Gff.hpp"

#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_set>

using namespace FileFormats;
using namespace FileFormats::Erf;

int main(int argc, char** argv)
{
    std::filesystem::path path_out = argv[1];
    std::filesystem::path path_in_haks = argv[2];
    std::filesystem::path path_in_content = argv[3];

    using namespace FileFormats::Erf;

    Friendly::Erf erf;

    Raw::ErfLocalisedString desc;
    desc.m_LanguageId = 0;
    desc.m_String = "Anphillia\nhttp://www.anphilliarise.com\nA new epic take on the classic module of Anphillia.";
    erf.GetDescriptions().emplace_back(std::move(desc));

    std::unordered_set<std::string> haks;
    std::unordered_set<std::string> areas;
    std::string custom_tlk;

    for (const auto& file : std::filesystem::directory_iterator(path_in_haks))
    {
        if (!file.is_regular_file()) continue;
        if (file.path().extension() == ".tlk")
        {
            custom_tlk = file.path().stem().string();
            std::printf("Custom TLK: %s\n", custom_tlk.c_str());
            continue;
        }
        if (file.path().extension() != ".hak") continue;
        std::string hak = file.path().stem().string();
        std::printf("Hak: %s\n", hak.c_str());
        haks.emplace(std::move(hak));
    }

    Gff::Friendly::Gff module_ifo;

    for (const auto& file : std::filesystem::recursive_directory_iterator(path_in_content))
    {
        if (!file.is_regular_file()) continue;

        if (file.path().filename() == "module.ifo")
        {
            Gff::Raw::Gff raw_gff;
            bool loaded = Gff::Raw::Gff::ReadFromFile(file.path().string().c_str(), &raw_gff);
            ASSERT(loaded);
            module_ifo = Gff::Friendly::Gff(std::move(raw_gff));
            continue; // We'll add it back later.
        }

        if (file.path().extension() == ".are")
        {
            areas.emplace(file.path().stem().string());
        }

        std::uintmax_t len = std::filesystem::file_size(file);
        std::printf("Packing %s [%zu].\n", file.path().string().c_str(), len);
        std::unique_ptr<OwningDataBlock> db = std::make_unique<OwningDataBlock>();
        db->m_Data.resize(len);

        FILE* f = std::fopen(file.path().string().c_str(), "rb");
        ASSERT(f);

        if (f)
        {
            std::fread(db->m_Data.data(), len, 1, f);
            std::fclose(f);
        }
        else
        {
            continue;
        }

        Friendly::ErfResource res;
        res.m_ResRef = file.path().stem().string();
        res.m_ResType = FileFormats::Resource::ResourceTypeFromString(file.path().extension().string().substr(1).c_str());
        res.m_DataBlock = std::move(db);
        erf.GetResources().emplace_back(std::move(res));
    }

    // For module.ifo, we strip Mod_Area_list, Mod_HakList, and Mod_CustomTlk, then repopulate it from above.

    Gff::Friendly::Type_List gff_areas, gff_haks;

    for (const std::string& area : areas)
    {
        Gff::Friendly::Type_CResRef area_resref;
        std::memset(area_resref.m_String, 0, sizeof(area_resref.m_String));
        area_resref.m_Size = (std::uint8_t)area.size();
        std::memcpy(area_resref.m_String, area.c_str(), area_resref.m_Size);

        Gff::Friendly::Type_Struct struc;
        struc.SetUserDefinedId(6);
        struc.WriteField("Area_Name", std::move(area_resref));

        gff_areas.GetStructs().emplace_back(std::move(struc));
    }

    for (const std::string& hak : haks)
    {
        Gff::Friendly::Type_CExoString gff_hak;
        gff_hak.m_String = hak;

        Gff::Friendly::Type_Struct struc;
        struc.SetUserDefinedId(8);
        struc.WriteField("Mod_Hak", std::move(gff_hak));

        gff_haks.GetStructs().emplace_back(std::move(struc));
    }

    module_ifo.GetTopLevelStruct().WriteField("Mod_Area_list", std::move(gff_areas));
    module_ifo.GetTopLevelStruct().WriteField("Mod_HakList", std::move(gff_haks));

    Gff::Friendly::Type_CExoString gff_customtlk;
    gff_customtlk.m_String = custom_tlk;
    module_ifo.GetTopLevelStruct().WriteField("Mod_CustomTlk", std::move(gff_customtlk));

    const char* ifo_ext = "IFO ";
    std::memcpy(module_ifo.GetFileType(), ifo_ext, 4);

    std::unique_ptr<OwningDataBlock> db = std::make_unique<OwningDataBlock>();

    size_t len_required = module_ifo.WriteToBytes(nullptr, 0);
    db->m_Data.resize(len_required);
    size_t len_written = module_ifo.WriteToBytes(db->m_Data.data(), len_required);
    ASSERT(len_required == len_written);

    Friendly::ErfResource res;
    res.m_ResRef = "module";
    res.m_ResType = FileFormats::Resource::ResourceType::IFO;
    res.m_DataBlock = std::move(db);
    erf.GetResources().emplace_back(std::move(res));

    const char* mod_ext = "MOD ";
    std::memcpy(erf.GetFileType(), mod_ext, 4);

    return !erf.WriteToFile(path_out.string().c_str());
}
