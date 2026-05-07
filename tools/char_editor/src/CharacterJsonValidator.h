#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace CharacterJson
{

struct Issue
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

std::vector<Issue> Validate(const nlohmann::ordered_json& doc);

}
