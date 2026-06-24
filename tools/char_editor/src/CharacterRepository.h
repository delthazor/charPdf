#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace CharEditor
{

struct CharacterFile
{
    std::string campaign;
    std::string filename;
    std::string relativePath;
    std::string fullPath;
};

class CharacterRepository
{
  public:
    explicit CharacterRepository(std::string rootDir);

    std::vector<CharacterFile> ListCharacterFiles() const;
    std::vector<std::string> ListCampaigns() const;

    nlohmann::ordered_json Load(const std::string& fullPath) const;
    void SaveAtomic(const std::string& fullPath, const nlohmann::ordered_json& doc) const;

    const std::string& RootDir() const { return rootDir; }
    std::string CharsDir() const;

  private:
    std::string rootDir;
};

}
