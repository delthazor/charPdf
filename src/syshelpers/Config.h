#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class Config
{
  public:
    Config() = default;
    Config(const nlohmann::ordered_json& j) : data(j) {}

    std::string getString(const std::string& key) const { return data[key].get<std::string>(); }
    int getInt(const std::string& key) const { return data[key].get<int>(); }
    double getDouble(const std::string& key) const { return data[key].get<double>(); }
    bool getBool(const std::string& key) const { return data[key].get<bool>(); }

    Config getObject(const std::string& key) const { return Config(data[key]); }

    std::vector<std::string> getStringArray(const std::string& key) const
    {
        return data[key].get<std::vector<std::string>>();
    }
    std::vector<int> getIntArray(const std::string& key) const { return data[key].get<std::vector<int>>(); }
    std::vector<double> getDoubleArray(const std::string& key) const
    {
        return data[key].get<std::vector<double>>();
    }
    std::vector<bool> getBoolArray(const std::string& key) const
    {
        return data[key].get<std::vector<bool>>();
    }

    std::vector<Config> getObjectArray(const std::string& key) const
    {
        std::vector<Config> result;
        for (const auto& item : data[key])
        {
            result.emplace_back(item);
        }
        return result;
    }

    bool isNull() const { return data.is_null(); }
    bool hasKey(const std::string& key) const { return data.contains(key); }

    bool isStringArray(const std::string& key) const
    {
        if (!hasKey(key)) { return false; }
        const auto& node = data[key];
        if (!node.is_array()) { return false; }
        for (const auto& el : node)
        {
            if (!el.is_string()) { return false; }
        }
        return true;
    }

    std::vector<std::string> getKeys() const
    {
        std::vector<std::string> keys;
        for (auto it = data.begin(); it != data.end(); ++it)
        {
            keys.push_back(it.key());
        }
        return keys;
    }

  private:
    nlohmann::ordered_json data;
};
