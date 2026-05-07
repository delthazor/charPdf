#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace CharEditor
{

struct ValidationIssue
{
    enum class Severity
    {
        Error,
        Warning,
    };

    Severity severity = Severity::Error;
    std::string jsonPath;
    std::string message;
};

class CharacterSchema
{
  public:
    static nlohmann::ordered_json MakeDefaultCharacter();
    static void NormalizeInPlace(nlohmann::ordered_json& doc);

    static std::vector<ValidationIssue> Validate(const nlohmann::ordered_json& doc);
};

}

