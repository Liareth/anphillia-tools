#include "FileFormats/Erf.hpp"
#include "Utility/Assert.hpp"

#include <cstring>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

template <typename ... Args>
void log_msg(const char* fmt, Args&& ... args)
{
    std::printf(fmt, std::move(args) ...);
    fflush(stdout); // Better log output when interweaving system();
}

using namespace FileFormats::Erf;

struct AssetCacheInfo
{
    bool needs_rebuild;
    std::filesystem::path original_path;
    std::filesystem::path resolved_path_in_cache;
    std::uint32_t original_checksum;
};

class AssetCache
{
public:
    AssetCache(std::filesystem::path cache_root)
        : m_cache_root(std::move(cache_root)),
          m_cache_root_file(m_cache_root / "CACHE_ROOT")
    {
        std::filesystem::create_directory(m_cache_root);

        if (FILE* cached = std::fopen(m_cache_root_file.string().c_str(), "r"); cached)
        {
            char buf[256];

            while (std::fgets(buf, 256, cached))
            {
                char original_name[256] = { '\0' };
                char new_name[256] = { '\0' };
                std::uint32_t hash;
                std::sscanf(buf, "%s %s %u", original_name, new_name, &hash);
                m_cache[original_name] = std::make_pair(new_name, hash);
            }

            log_msg("Read %zu entries from %s.\n", m_cache.size(), m_cache_root_file.string().c_str());
            std::fclose(cached);
        }
    }

    AssetCacheInfo asset_needs_rebuild(std::filesystem::path asset)
    {
        std::uintmax_t len = std::filesystem::file_size(asset);
        std::vector<std::byte> data;

        if (FILE* f = std::fopen(asset.string().c_str(), "rb"); f)
        {
            data.resize(len);
            std::fread(data.data(), len, 1, f);
            std::fclose(f);
        }

        std::string old_file_name = asset.filename().string();
        std::uint32_t hash = calculate_hash(data.data(), data.size());
        auto iter = m_cache.find(old_file_name);
        bool rebuild = iter == std::end(m_cache) || iter->second.second != hash;
        std::string resolved_file_name = iter == std::end(m_cache) ? old_file_name : iter->second.first;
        std::filesystem::path cache_path = m_cache_root / resolved_file_name;

        if (!rebuild)
        {
            log_msg("Found match for %s -> %s in cache.\n",
                asset.string().c_str(), cache_path.string().c_str());
        }

        return { rebuild, asset, cache_path, hash };
    }

    void commit_rebuilt_asset(AssetCacheInfo&& info)
    {
        ASSERT(info.needs_rebuild);
        std::string old_name = info.original_path.filename().string();
        std::string new_name = info.resolved_path_in_cache.filename().string();
        log_msg("Asset %s committed to cache as %s.\n", old_name.c_str(), new_name.c_str());
        m_cache[std::move(old_name)] = std::make_pair(std::move(new_name), info.original_checksum);
    }

    void save()
    {
        if (FILE* cached = std::fopen(m_cache_root_file.string().c_str(), "w"); cached)
        {
            for (const auto& kvp : m_cache)
            {
                std::fprintf(cached, "%s %s %u\n", kvp.first.c_str(), kvp.second.first.c_str(), kvp.second.second);
            }

            log_msg("Wrote %zu entries to %s.\n", m_cache.size(), m_cache_root_file.string().c_str());
            std::fclose(cached);
        }
    }

private:
    std::uint32_t calculate_hash(std::byte* data, size_t len)
    {
        constexpr std::uint32_t VERSION = 0;
        std::uint32_t hash = VERSION;

        while (len >= 4)
        {
            std::uint32_t a = (std::uint32_t)data[0];
            std::uint32_t b = (std::uint32_t)data[1];
            std::uint32_t c = (std::uint32_t)data[2];
            std::uint32_t d = (std::uint32_t)data[3];

            hash ^= a;
            hash ^= b << 8;
            hash ^= c << 16;
            hash ^= d << 24;

            data += 4;
            len -= 4;
        }

        while (len--)
        {
            hash ^= (std::uint32_t)data[0];
            ++data;
        }

        return hash;
    }

    std::unordered_map<std::string, std::pair<std::string, std::uint32_t>> m_cache; // Original file name -> { new file name, hash }
    std::filesystem::path m_cache_root;
    std::filesystem::path m_cache_root_file;
};

std::unique_ptr<OwningDataBlock> build_asset(AssetCache& cache, std::filesystem::path* path_asset,
    std::filesystem::path path_tex_packer, std::filesystem::path path_model_compiler, std::filesystem::path path_user_dir)
{
    std::unordered_set<std::string> texture_types =
    {
        ".bmp", ".png", ".tga", ".dds", ".ktx", ".crn"
    };

    std::filesystem::path path_asset_ext = path_asset->extension();

    if (texture_types.find(path_asset_ext.string()) != std::end(texture_types))
    {
        AssetCacheInfo info = cache.asset_needs_rebuild(*path_asset);
        bool rebuilt = false;

        if (info.needs_rebuild)
        {
            std::filesystem::path tex_packer_in = path_tex_packer / "in";
            std::filesystem::path tex_packer_in_file = tex_packer_in / info.original_path.filename();
            std::filesystem::path tex_packer_out = path_tex_packer / "out";
            std::filesystem::path tex_packer_script = path_tex_packer / "convert_nwn";

#if defined(_WIN32)
            tex_packer_script += ".bat";
#else
            tex_packer_script += ".sh";
#endif

            std::filesystem::copy_file(info.original_path, tex_packer_in_file);
            std::filesystem::permissions(tex_packer_in_file, std::filesystem::perms::owner_write, std::filesystem::perm_options::add); // perforce is a plague of mankind

            char cmd[512];
            std::sprintf(cmd, "cd %s && %s", path_tex_packer.string().c_str(), tex_packer_script.string().c_str());
            log_msg("Invoking: '%s'\n", cmd);
            system(cmd);

            std::filesystem::remove(tex_packer_in_file);

            for (const auto& file : std::filesystem::directory_iterator(tex_packer_out))
            {
                if (file.path().stem() == tex_packer_in_file.stem())
                {
                    info.resolved_path_in_cache.replace_filename(file.path().filename());
                    std::filesystem::rename(file, info.resolved_path_in_cache);
                    rebuilt = true;
                    break;
                }
            }
        }

        *path_asset = info.resolved_path_in_cache;

        if (rebuilt)
        {
            cache.commit_rebuilt_asset(std::move(info));
        }
    }
    else if (path_asset_ext == ".mdl")
    {
        AssetCacheInfo info = cache.asset_needs_rebuild(*path_asset);
        bool rebuilt = false;

        if (info.needs_rebuild)
        {
            std::filesystem::path asset_path_in_user_dir = path_user_dir / "override" / info.original_path.filename();
            std::filesystem::copy_file(info.original_path, asset_path_in_user_dir);
            std::filesystem::permissions(asset_path_in_user_dir, std::filesystem::perms::owner_write, std::filesystem::perm_options::add);

            char cmd[512];
            std::sprintf(cmd, "cd %s && %s compilemodel %s",
                path_model_compiler.parent_path().string().c_str(),
                path_model_compiler.string().c_str(),
                info.original_path.stem().string().c_str());
            log_msg("Invoking: '%s'\n", cmd);
            system(cmd);

            std::filesystem::path asset_path_in_mc_output = path_user_dir / "modelcompiler" / info.original_path.filename();
            std::filesystem::rename(asset_path_in_mc_output, info.resolved_path_in_cache);
            rebuilt = true;
        }

        *path_asset = info.resolved_path_in_cache;

        if (rebuilt)
        {
            cache.commit_rebuilt_asset(std::move(info));
        }
    }

    std::unique_ptr<OwningDataBlock> db = std::make_unique<OwningDataBlock>();

    if (FILE* f = std::fopen(path_asset->string().c_str(), "rb"); f)
    {
        std::uintmax_t len = std::filesystem::file_size(*path_asset);
        db->m_Data.resize(len);
        std::fread(db->m_Data.data(), len, 1, f);
        std::fclose(f);
    }

    return db;
}

int main(int argc, char** argv)
{
    std::filesystem::path path_out = argv[1];
    std::filesystem::path path_in = argv[2];
    std::filesystem::path path_asset_cache = argv[3];
    std::filesystem::path path_tex_packer = argv[4];
    std::filesystem::path path_model_compiler = argv[5];
    std::filesystem::path path_user_dir = argv[6];

    using namespace FileFormats::Erf;

    std::vector<Friendly::Erf> erfs;

    static auto build_new_erf = []() -> Friendly::Erf
    {
        Friendly::Erf erf;
        Raw::ErfLocalisedString desc;
        desc.m_LanguageId = 0;
        desc.m_String = "Anphillia\nhttp://www.anphilliarise.com\nA new epic take on the classic module of Anphillia.";
        erf.GetDescriptions().emplace_back(std::move(desc));
        return erf;
    };

    erfs.emplace_back(build_new_erf());

    AssetCache cache(path_asset_cache);

    for (const auto& file : std::filesystem::recursive_directory_iterator(path_in))
    {
        if (!file.is_regular_file()) continue;

        Friendly::Erf& erf = erfs[erfs.size() - 1];

        std::filesystem::path file_path = file.path();

        if (std::unique_ptr<OwningDataBlock> db = build_asset(cache, &file_path,
            path_tex_packer, path_model_compiler, path_user_dir); db->GetDataLength())
        {
            log_msg("Packing %s [%zu].\n", file_path.string().c_str(), db->GetDataLength());
            Friendly::ErfResource res;
            res.m_ResRef = file_path.stem().string();
            res.m_ResType = FileFormats::Resource::ResourceTypeFromString(file_path.extension().string().substr(1).c_str());
            res.m_DataBlock = std::move(db);
            erf.GetResources().emplace_back(std::move(res));

            if (erf.GetResources().size() == 16000)
            {
                // I think we can go up to 16392 but this is safer.
                erfs.emplace_back(build_new_erf());
            }
        }
        else
        {
            log_msg("Failed to pack %s!\n", file_path.string().c_str());
        }
    }

    cache.save();

    bool any_failures = false;

    for (int i = 0; i < erfs.size(); ++i)
    {
        std::filesystem::path hak_path = path_out.parent_path();
        std::filesystem::path hak_file_name = path_out.stem();

        if (erfs.size() > 1)
        {
            hak_file_name += std::to_string(i);
        }

        const char* mod_ext = "HAK ";
        std::memcpy(erfs[i].GetFileType(), mod_ext, 4);

        std::filesystem::path end_path = hak_path;
        end_path /= hak_file_name;
        end_path += ".hak";

        log_msg("Writing HAK %s.\n", end_path.string().c_str());
        any_failures |= !erfs[i].WriteToFile(end_path.string().c_str());
    }

    return !!any_failures;
}
