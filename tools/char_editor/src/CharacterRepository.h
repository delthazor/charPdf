#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace CharEditor
{

struct CharacterFile
{
    std::string filename;
    std::string fullPath;
};

class CharacterRepository
{
  public:
    explicit CharacterRepository(std::string rootDir);

    std::vector<CharacterFile> ListCharacterFiles() const;

    nlohmann::ordered_json Load(const std::string& fullPath) const;
    void SaveAtomic(const std::string& fullPath, const nlohmann::ordered_json& doc) const;

    const std::string& RootDir() const { return rootDir; }

  private:
    std::string rootDir;
};

}

