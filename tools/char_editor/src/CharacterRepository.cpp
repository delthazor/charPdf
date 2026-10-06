#include "CharacterRepository.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace CharEditor
{

namespace fs = std::filesystem;

namespace
{

bool IsCharJsonFilename(const std::string& name)
{
    if (name.size() < 10) { return false; }
    if (name.rfind("char_", 0) != 0) { return false; }
    if (name.size() < 5 || name.substr(name.size() - 5) != ".json") { return false; }
    return true;
}

}

CharacterRepository::CharacterRepository(std::string rootDirParam) : rootDir(std::move(rootDirParam)) {}

std::string CharacterRepository::CharsDir() const { return rootDir + "/chars"; }

std::vector<std::string> CharacterRepository::ListCampaigns() const
{
    std::vector<std::string> campaigns;
    const fs::path charsRoot(CharsDir());
    if (!fs::exists(charsRoot) || !fs::is_directory(charsRoot)) { return campaigns; }

    for (const auto& entry : fs::directory_iterator(charsRoot))
    {
        if (entry.is_directory()) { campaigns.push_back(entry.path().filename().string()); }
    }

    std::sort(campaigns.begin(), campaigns.end());
    return campaigns;
}

std::vector<CharacterFile> CharacterRepository::ListCharacterFiles() const
{
    std::vector<CharacterFile> out;
    const fs::path charsRoot(CharsDir());
    if (!fs::exists(charsRoot) || !fs::is_directory(charsRoot)) { return out; }

    for (const auto& campaignEntry : fs::directory_iterator(charsRoot))
    {
        if (!campaignEntry.is_directory()) { continue; }

        const std::string campaign = campaignEntry.path().filename().string();
        for (const auto& fileEntry : fs::directory_iterator(campaignEntry.path()))
        {
            if (!fileEntry.is_regular_file()) { continue; }

            const std::string filename = fileEntry.path().filename().string();
            if (!IsCharJsonFilename(filename)) { continue; }

            const std::string relativePath = std::string("chars/") + campaign + "/" + filename;
            out.push_back({campaign, filename, relativePath, fileEntry.path().string()});
        }
    }

    std::sort(out.begin(), out.end(), [](const CharacterFile& a, const CharacterFile& b) {
        return a.relativePath < b.relativePath;
    });

    return out;
}

nlohmann::ordered_json CharacterRepository::Load(const std::string& fullPath) const
{
    std::ifstream file(fullPath);
    if (!file) { throw std::runtime_error("cannot open JSON file: " + fullPath); }
    return nlohmann::ordered_json::parse(file);
}

void CharacterRepository::SaveAtomic(const std::string& fullPath, const nlohmann::ordered_json& doc) const
{
    const fs::path target(fullPath);
    std::error_code ec;
    fs::create_directories(target.parent_path(), ec);
    if (ec) { throw std::runtime_error("cannot create directory: " + target.parent_path().string()); }

    const fs::path tmp = target.parent_path() / (target.filename().string() + ".tmp");

    {
        std::ofstream out(tmp);
        if (!out) { throw std::runtime_error("cannot write temp file: " + tmp.string()); }
        out << doc.dump(4) << "\n";
        out.flush();
        if (!out) { throw std::runtime_error("failed writing temp file: " + tmp.string()); }
    }

    ec.clear();
    fs::rename(tmp, target, ec);
    if (ec)
    {
        fs::remove(target, ec);
        ec.clear();
        fs::rename(tmp, target, ec);
        if (ec) { throw std::runtime_error("cannot replace target file: " + target.string()); }
    }
}

std::string CharacterRepository::MoveToCampaignArchive(const std::string& fullPath) const
{
    const fs::path source(fullPath);
    std::error_code ec;
    if (!fs::is_regular_file(source, ec))
    {
        throw std::runtime_error("character file not found: " + fullPath);
    }

    const fs::path campaignDir = source.parent_path();
    ec.clear();
    const fs::path charsRoot = fs::weakly_canonical(fs::path(CharsDir()), ec);
    if (ec) { throw std::runtime_error("character directory not found: " + CharsDir()); }

    ec.clear();
    const fs::path campaignCanon = fs::weakly_canonical(campaignDir, ec);
    if (ec || campaignCanon.parent_path() != charsRoot)
    {
        throw std::runtime_error("character file is not directly in a campaign folder: " + fullPath);
    }

    const fs::path destDir = campaignDir / "archived";
    const fs::path dest = destDir / source.filename();
    if (fs::exists(dest))
    {
        throw std::runtime_error("archived character already exists: " + dest.string());
    }

    ec.clear();
    fs::create_directories(destDir, ec);
    if (ec) { throw std::runtime_error("cannot create directory: " + destDir.string()); }

    ec.clear();
    fs::rename(source, dest, ec);
    if (ec) { throw std::runtime_error("cannot move character file: " + source.string()); }

    return dest.string();
}

}
