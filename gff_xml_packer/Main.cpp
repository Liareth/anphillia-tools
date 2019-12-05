#include "FileFormats/Gff.hpp"
#include "gff_xml_core/GffXml.hpp"

#include <cstdio>
#include <string>
#include <unordered_set>

using namespace FileFormats::Gff;

int main(int argc, char** argv)
{
    std::filesystem::path path_out = argv[1];
    std::filesystem::path path_in = argv[2];

    std::filesystem::create_directory(path_out);

    bool any_failure = false;
    std::vector<std::string> unprocessed_paths;

    if (std::filesystem::exists(path_in / "REPO_ROOT"))
    {
        std::printf("Batch mode: xml -> gff.\n");

        for (const auto& file : std::filesystem::recursive_directory_iterator(path_in))
        {
            if (!file.is_regular_file()) continue;

            std::filesystem::path file_path = file.path();
            std::string ext = file_path.has_extension() ? file_path.extension().string().substr(1) : "";

            if (ext != "xml")
            {
                unprocessed_paths.emplace_back(file_path.string());
                continue;
            }

            std::filesystem::path new_file_path = path_out;
            new_file_path /= file_path.filename();
            new_file_path.replace_extension("?");
            any_failure |= !convert_file(file_path, new_file_path);
        }
    }
    else
    {
        std::printf("Batch mode: gff -> xml.\n");

        static std::unordered_set<std::string> permitted_gff_types =
        {
            {"are"}, {"dlg"}, {"fac"}, {"gic"}, {"git"}, {"ifo"}, {"itp"}, {"mod"},
            {"utc"}, {"utd"}, {"ute"}, {"uti"}, {"utm"}, {"utp"}, {"uts"}, {"utt"}, {"utw"},
        };

        for (const auto& file : std::filesystem::directory_iterator(path_in))
        {
            if (!file.is_regular_file()) continue;

            std::filesystem::path file_path = file.path();
            std::string ext = file_path.has_extension() ? file_path.extension().string().substr(1) : "";

            if (permitted_gff_types.find(ext) == std::end(permitted_gff_types))
            {
                unprocessed_paths.emplace_back(file_path.string());
                continue;
            }

            std::filesystem::path new_file_path = path_out;
            new_file_path /= ext;

            std::filesystem::create_directory(new_file_path);

            new_file_path /= file_path.filename();
            new_file_path.replace_extension("xml");
            any_failure |= !convert_file(file_path, new_file_path);
        }

        std::filesystem::path repo_root = path_out;
        repo_root /= "REPO_ROOT";

        if (FILE* file = std::fopen(repo_root.string().c_str(), "w"); file)
        {
            std::fclose(file);
        }
    }

    if (!unprocessed_paths.empty())
    {
        std::filesystem::path unproc_file_path = path_out / "unprocessed.txt";

        if (FILE* file = std::fopen(unproc_file_path.string().c_str(), "w"); file)
        {
            for (const std::string& line : unprocessed_paths)
            {
                std::fprintf(file, "%s\n", line.c_str());
            }

            std::fclose(file);
        }
    }

    return !!any_failure;
}
