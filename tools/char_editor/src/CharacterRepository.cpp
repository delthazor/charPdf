#include "CharacterRepository.h"

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

CharacterRepository::CharacterRepository(std::string rootDir) : rootDir(std::move(rootDir)) {}

std::vector<CharacterFile> CharacterRepository::ListCharacterFiles() const
{
    std::vector<CharacterFile> out;
    if (!fs::exists(rootDir)) { return out; }

    for (const auto& entry : fs::directory_iterator(rootDir))
    {
        if (!entry.is_regular_file()) { continue; }
        const std::string filename = entry.path().filename().string();
        if (!IsCharJsonFilename(filename)) { continue; }
        out.push_back({filename, entry.path().string()});
    }

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
    const fs::path tmp = target.parent_path() / (target.filename().string() + ".tmp");

    {
        std::ofstream out(tmp);
        if (!out) { throw std::runtime_error("cannot write temp file: " + tmp.string()); }
        out << doc.dump(4) << "\n";
        out.flush();
        if (!out) { throw std::runtime_error("failed writing temp file: " + tmp.string()); }
    }

    std::error_code ec;
    fs::rename(tmp, target, ec);
    if (ec)
    {
        fs::remove(target, ec);
        ec.clear();
        fs::rename(tmp, target, ec);
        if (ec) { throw std::runtime_error("cannot replace target file: " + target.string()); }
    }
}

}

