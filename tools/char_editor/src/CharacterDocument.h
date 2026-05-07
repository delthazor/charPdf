#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace CharEditor
{

class CharacterDocument
{
  public:
    CharacterDocument();
    explicit CharacterDocument(nlohmann::ordered_json doc);

    const nlohmann::ordered_json& Json() const { return doc; }
    nlohmann::ordered_json& Json() { dirty = true; return doc; }

    bool IsDirty() const { return dirty; }
    void MarkClean() { dirty = false; }

    const std::optional<std::string>& FilePath() const { return filePath; }
    void SetFilePath(std::optional<std::string> p) { filePath = std::move(p); }

  private:
    nlohmann::ordered_json doc;
    bool dirty = false;
    std::optional<std::string> filePath;
};

}

