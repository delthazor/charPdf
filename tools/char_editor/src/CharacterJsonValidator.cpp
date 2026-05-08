#include "CharacterJsonValidator.h"

#include <cmath>
#include <string>

namespace CharacterJson
{

namespace
{

std::string JoinPath(const std::string& base, const std::string& key)
{
    if (base == "/") { return "/" + key; }
    return base + "/" + key;
}

bool IsStringArray(const nlohmann::ordered_json& node)
{
    if (!node.is_array()) { return false; }
    for (const auto& el : node)
    {
        if (!el.is_string()) { return false; }
    }
    return true;
}

bool IsStringArrayNonEmpty(const nlohmann::ordered_json& node)
{
    if (!IsStringArray(node)) { return false; }
    for (const auto& el : node)
    {
        if (el.get<std::string>().empty()) { return false; }
    }
    return true;
}

void RequireType(std::vector<Issue>& out,
                 const nlohmann::ordered_json& node,
                 const std::string& path,
                 nlohmann::ordered_json::value_t type)
{
    if (node.type() != type) { out.push_back({Issue::Severity::Error, path, "wrong type"}); }
}

void RequireKey(std::vector<Issue>& out,
                const nlohmann::ordered_json& obj,
                const std::string& path,
                const char* key)
{
    if (!obj.is_object())
    {
        out.push_back({Issue::Severity::Error, path, "expected object"});
        return;
    }
    if (!obj.contains(key))
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "missing key"});
    }
}

void RequireString(std::vector<Issue>& out,
                   const nlohmann::ordered_json& obj,
                   const std::string& path,
                   const char* key)
{
    RequireKey(out, obj, path, key);
    if (obj.is_object() && obj.contains(key) && !obj.at(key).is_string())
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "expected string"});
    }
}

void RequireNonEmptyString(std::vector<Issue>& out,
                           const nlohmann::ordered_json& obj,
                           const std::string& path,
                           const char* key)
{
    RequireString(out, obj, path, key);
    if (obj.is_object() && obj.contains(key) && obj.at(key).is_string() && obj.at(key).get<std::string>().empty())
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "must not be empty"});
    }
}

void RequireInt(std::vector<Issue>& out,
                const nlohmann::ordered_json& obj,
                const std::string& path,
                const char* key)
{
    RequireKey(out, obj, path, key);
    if (obj.is_object() && obj.contains(key) && !obj.at(key).is_number_integer())
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "expected int"});
    }
}

void RequireNameLetterSpacing(std::vector<Issue>& out,
                              const nlohmann::ordered_json& obj,
                              const std::string& path,
                              const char* key)
{
    RequireKey(out, obj, path, key);
    if (!obj.is_object() || !obj.contains(key)) { return; }
    const auto& n = obj.at(key);
    if (!n.is_number())
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "expected number"});
        return;
    }
    const double v = n.is_number_integer() ? static_cast<double>(n.get<int>()) : n.get<double>();
    if (!std::isfinite(v))
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "must be a finite number"});
        return;
    }
    if (v < -50.0 || v > 50.0)
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "must be between -50 and 50"});
        return;
    }
    const double scaled = std::round(v * 100.0);
    if (std::fabs(v * 100.0 - scaled) > 1e-4)
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "at most 2 decimal places"});
    }
}

void RequireNonNegInt(std::vector<Issue>& out,
                      const nlohmann::ordered_json& obj,
                      const std::string& path,
                      const char* key)
{
    RequireInt(out, obj, path, key);
    if (obj.is_object() && obj.contains(key) && obj.at(key).is_number_integer() && obj.at(key).get<int>() < 0)
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "must be >= 0"});
    }
}

void RequireNonZeroNonNegInt(std::vector<Issue>& out,
                             const nlohmann::ordered_json& obj,
                             const std::string& path,
                             const char* key)
{
    RequireNonNegInt(out, obj, path, key);
    if (obj.is_object() && obj.contains(key) && obj.at(key).is_number_integer() && obj.at(key).get<int>() == 0)
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "must not be 0"});
    }
}

void RequireStringArray(std::vector<Issue>& out,
                        const nlohmann::ordered_json& obj,
                        const std::string& path,
                        const char* key)
{
    RequireKey(out, obj, path, key);
    if (obj.is_object() && obj.contains(key) && !IsStringArrayNonEmpty(obj.at(key)))
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, key), "expected string[] (no empty strings)"});
    }
}

void ValidateDamageBase(std::vector<Issue>& out,
                        const nlohmann::ordered_json& base,
                        const std::string& path)
{
    if (!base.is_object())
    {
        out.push_back({Issue::Severity::Error, path, "expected object"});
        return;
    }
    RequireNonEmptyString(out, base, path, "dice");
    RequireNonNegInt(out, base, path, "bonus");
    RequireNonEmptyString(out, base, path, "type");
}

void ValidateDamageSection(std::vector<Issue>& out,
                           const nlohmann::ordered_json& dmg,
                           const std::string& path)
{
    if (!dmg.is_object())
    {
        out.push_back({Issue::Severity::Error, path, "expected object"});
        return;
    }
    RequireKey(out, dmg, path, "base");
    if (dmg.contains("base")) { ValidateDamageBase(out, dmg.at("base"), JoinPath(path, "base")); }

    if (dmg.contains("alt"))
    {
        const auto& alt = dmg.at("alt");
        if (!alt.is_object())
        {
            out.push_back({Issue::Severity::Error, JoinPath(path, "alt"), "expected object"});
        }
        else if (!alt.empty())
        {
            if (alt.contains("dice") && !alt.at("dice").is_string())
            {
                out.push_back({Issue::Severity::Error, JoinPath(path, "alt") + "/dice", "expected string"});
            }
            if (alt.contains("bonus") && !alt.at("bonus").is_number_integer())
            {
                out.push_back({Issue::Severity::Error, JoinPath(path, "alt") + "/bonus", "expected int"});
            }
            if (alt.contains("type") && !alt.at("type").is_string())
            {
                out.push_back({Issue::Severity::Error, JoinPath(path, "alt") + "/type", "expected string"});
            }
        }
    }

    RequireKey(out, dmg, path, "extra");
    if (dmg.contains("extra") && !IsStringArrayNonEmpty(dmg.at("extra")))
    {
        out.push_back({Issue::Severity::Error, JoinPath(path, "extra"), "expected string[] (no empty strings)"});
    }
}

void ValidateWeapon(std::vector<Issue>& out, const nlohmann::ordered_json& w, const std::string& path)
{
    if (!w.is_object())
    {
        out.push_back({Issue::Severity::Error, path, "expected object"});
        return;
    }
    RequireNonEmptyString(out, w, path, "name");
    RequireNonEmptyString(out, w, path, "type");
    RequireStringArray(out, w, path, "props");
    RequireString(out, w, path, "extratext");

    // Optional range: if present, must be a non-empty string.
    if (w.contains("range"))
    {
        if (!w.at("range").is_string())
        {
            out.push_back({Issue::Severity::Error, JoinPath(path, "range"), "expected string"});
        }
        else if (w.at("range").get<std::string>().empty())
        {
            out.push_back({Issue::Severity::Error, JoinPath(path, "range"), "must not be empty"});
        }
    }

    RequireKey(out, w, path, "damage");
    if (w.contains("damage")) { ValidateDamageSection(out, w.at("damage"), JoinPath(path, "damage")); }
}

void ValidateArmorAc(std::vector<Issue>& out, const nlohmann::ordered_json& ac, const std::string& path)
{
    if (!ac.is_object())
    {
        out.push_back({Issue::Severity::Error, path, "expected object"});
        return;
    }
    const bool hasFix = ac.contains("fixmod");
    const bool hasBase = ac.contains("base");
    if (hasFix && hasBase)
    {
        out.push_back({Issue::Severity::Error, path, "ac: use fixmod or base, not both"});
    }
    else if (hasFix)
    {
        RequireNonZeroNonNegInt(out, ac, path, "fixmod");
    }
    else if (hasBase)
    {
        RequireNonZeroNonNegInt(out, ac, path, "base");
        RequireNonEmptyString(out, ac, path, "modstat");
        RequireNonNegInt(out, ac, path, "modcap");
    }
    else
    {
        out.push_back({Issue::Severity::Error, path, "ac: need fixmod or base"});
    }
}

void ValidateArmor(std::vector<Issue>& out, const nlohmann::ordered_json& a, const std::string& path)
{
    if (!a.is_object())
    {
        out.push_back({Issue::Severity::Error, path, "expected object"});
        return;
    }
    RequireNonEmptyString(out, a, path, "name");
    RequireNonEmptyString(out, a, path, "type");
    RequireString(out, a, path, "extratext");
    RequireKey(out, a, path, "ac");
    if (a.contains("ac")) { ValidateArmorAc(out, a.at("ac"), JoinPath(path, "ac")); }
}

void ValidateEquipmentBucket(std::vector<Issue>& out,
                             const nlohmann::ordered_json& bucket,
                             const std::string& bucketPath)
{
    if (!bucket.is_object())
    {
        out.push_back({Issue::Severity::Error, bucketPath, "expected object"});
        return;
    }
    RequireKey(out, bucket, bucketPath, "weapons");
    RequireKey(out, bucket, bucketPath, "armors");
    if (bucket.contains("weapons"))
    {
        const auto& weapons = bucket.at("weapons");
        if (!weapons.is_array())
        {
            out.push_back({Issue::Severity::Error, JoinPath(bucketPath, "weapons"), "expected array"});
        }
        else
        {
            for (size_t i = 0; i < weapons.size(); ++i)
            {
                ValidateWeapon(out, weapons.at(i), JoinPath(bucketPath, "weapons") + "/" + std::to_string(i));
            }
        }
    }
    if (bucket.contains("armors"))
    {
        const auto& armors = bucket.at("armors");
        if (!armors.is_array())
        {
            out.push_back({Issue::Severity::Error, JoinPath(bucketPath, "armors"), "expected array"});
        }
        else
        {
            for (size_t i = 0; i < armors.size(); ++i)
            {
                ValidateArmor(out, armors.at(i), JoinPath(bucketPath, "armors") + "/" + std::to_string(i));
            }
        }
    }
}

void ValidateProficiencySkills(std::vector<Issue>& out, const nlohmann::ordered_json& skills, const std::string& path)
{
    if (!skills.is_object())
    {
        out.push_back({Issue::Severity::Error, path, "expected object"});
        return;
    }
    // Required structure with required keys.
    struct SkillGroup
    {
        const char* statKey;
        const std::vector<const char*> skills;
    };
    const std::vector<SkillGroup> groups = {
        {"strength", {"athletics"}},
        {"dexterity", {"acrobatics", "sleightOfHand", "stealth"}},
        {"intelligence", {"arcana", "history", "investigation", "nature", "religion"}},
        {"wisdom", {"animalHandling", "insight", "medicine", "perception", "survival"}},
        {"charisma", {"deception", "intimidation", "performance", "persuasion"}},
    };

    for (const auto& g : groups)
    {
        RequireKey(out, skills, path, g.statKey);
        if (!skills.contains(g.statKey)) { continue; }
        const auto& statNode = skills.at(g.statKey);
        const std::string statPath = JoinPath(path, g.statKey);
        RequireType(out, statNode, statPath, nlohmann::ordered_json::value_t::object);
        if (!statNode.is_object()) { continue; }
        for (const char* sk : g.skills)
        {
            RequireNonNegInt(out, statNode, statPath, sk);
        }
    }
}

} // namespace

std::vector<Issue> Validate(const nlohmann::ordered_json& doc)
{
    std::vector<Issue> issues;

    if (!doc.is_object())
    {
        issues.push_back({Issue::Severity::Error, "/", "root must be an object"});
        return issues;
    }

    RequireString(issues, doc, "/", "name");
    RequireNameLetterSpacing(issues, doc, "/", "nameLetterSpacing");
    RequireNonEmptyString(issues, doc, "/", "race");
    RequireString(issues, doc, "/", "background");
    RequireNonEmptyString(issues, doc, "/", "name");

    RequireKey(issues, doc, "/", "stats");
    if (doc.contains("stats"))
    {
        const auto& stats = doc.at("stats");
        RequireType(issues, stats, "/stats", nlohmann::ordered_json::value_t::object);
        if (stats.is_object())
        {
            for (const char* k : {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma", "initiativeBonus"})
            {
                RequireNonNegInt(issues, stats, "/stats", k);
            }
            for (const char* k : {"speed", "maxHp", "ac"})
            {
                RequireNonZeroNonNegInt(issues, stats, "/stats", k);
            }
        }
    }

    RequireKey(issues, doc, "/", "proficiencies");
    if (doc.contains("proficiencies"))
    {
        const auto& prof = doc.at("proficiencies");
        RequireType(issues, prof, "/proficiencies", nlohmann::ordered_json::value_t::object);
        if (prof.is_object())
        {
            RequireNonZeroNonNegInt(issues, prof, "/proficiencies", "bonus");
            RequireKey(issues, prof, "/proficiencies", "skills");
            RequireKey(issues, prof, "/proficiencies", "savingThrows");

            if (prof.contains("skills"))
            {
                ValidateProficiencySkills(issues, prof.at("skills"), "/proficiencies/skills");
            }

            if (prof.contains("savingThrows"))
            {
                const auto& st = prof.at("savingThrows");
                const std::string stPath = "/proficiencies/savingThrows";
                RequireType(issues, st, stPath, nlohmann::ordered_json::value_t::object);
                if (st.is_object())
                {
                    for (const char* stat : {"strength",
                                             "dexterity",
                                             "constitution",
                                             "intelligence",
                                             "wisdom",
                                             "charisma"})
                    {
                        RequireNonNegInt(issues, st, stPath, stat);
                    }
                }
            }

            for (const char* arrKey : {"languages", "tools", "armors", "simpleWeapons", "martialWeapons"})
            {
                RequireStringArray(issues, prof, "/proficiencies", arrKey);
            }
        }
    }

    RequireKey(issues, doc, "/", "traits");
    if (doc.contains("traits"))
    {
        if (!IsStringArrayNonEmpty(doc.at("traits")))
        {
            issues.push_back({Issue::Severity::Error, "/traits", "expected string[] (no empty strings)"});
        }
    }

    RequireKey(issues, doc, "/", "classes");
    if (doc.contains("classes"))
    {
        const auto& classes = doc.at("classes");
        if (!classes.is_object())
        {
            issues.push_back({Issue::Severity::Error, "/classes", "expected object"});
        }
        else
        {
            if (classes.empty())
            {
                issues.push_back({Issue::Severity::Error, "/classes", "must contain at least one class"});
            }
            for (auto it = classes.begin(); it != classes.end(); ++it)
            {
                const std::string classId = it.key();
                const auto& classObj = it.value();
                const std::string basePath = JoinPath("/classes", classId);
                if (!classObj.is_object())
                {
                    issues.push_back({Issue::Severity::Error, basePath, "expected object"});
                    continue;
                }

                RequireNonZeroNonNegInt(issues, classObj, basePath, "level");

                RequireNonEmptyString(issues, classObj, basePath, "hitDice");
                RequireNonNegInt(issues, classObj, basePath, "resourcePoints");
                RequireString(issues, classObj, basePath, "castStat");
                RequireString(issues, classObj, basePath, "subclass");

                RequireKey(issues, classObj, basePath, "spellslots");
                if (classObj.contains("spellslots"))
                {
                    const auto& slots = classObj.at("spellslots");
                    if (!slots.is_object())
                    {
                        issues.push_back({Issue::Severity::Error,
                                          JoinPath(basePath, "spellslots"),
                                          "expected object"});
                    }
                    else
                    {
                        for (int level = 1; level <= 9; ++level)
                        {
                            const std::string k = std::to_string(level);
                            RequireNonNegInt(issues, slots, JoinPath(basePath, "spellslots"), k.c_str());
                        }
                    }
                }

                RequireKey(issues, classObj, basePath, "spells");
                if (classObj.contains("spells"))
                {
                    const auto& spells = classObj.at("spells");
                    const std::string spellsPath = JoinPath(basePath, "spells");
                    if (!spells.is_object())
                    {
                        issues.push_back({Issue::Severity::Error, spellsPath, "expected object"});
                    }
                    else
                    {
                        for (const char* must : {"0", "0_extra", "1", "2", "3", "4", "5", "6", "7", "8", "9"})
                        {
                            RequireKey(issues, spells, spellsPath, must);
                            if (spells.contains(must) && !IsStringArrayNonEmpty(spells.at(must)))
                            {
                                issues.push_back({Issue::Severity::Error,
                                                  JoinPath(spellsPath, must),
                                                  "expected string[] (no empty strings)"});
                            }
                        }
                    }
                }
            }
        }
    }

    RequireKey(issues, doc, "/", "equipment");
    if (doc.contains("equipment"))
    {
        if (!doc.at("equipment").is_object())
        {
            issues.push_back({Issue::Severity::Error, "/equipment", "expected object"});
        }
        else
        {
            const auto& eq = doc.at("equipment");
            for (const char* bucketKey : {"used", "stashed"})
            {
                RequireKey(issues, eq, "/equipment", bucketKey);
                if (eq.contains(bucketKey))
                {
                    ValidateEquipmentBucket(issues, eq.at(bucketKey), std::string("/equipment/") + bucketKey);
                }
            }
        }
    }

    RequireKey(issues, doc, "/", "backpack");
    if (doc.contains("backpack"))
    {
        const auto& backpack = doc.at("backpack");
        if (!backpack.is_object())
        {
            issues.push_back({Issue::Severity::Error, "/backpack", "expected object"});
        }
        else
        {
            for (const char* req : {"accessories", "consumables", "kits & tools", "general"})
            {
                RequireKey(issues, backpack, "/backpack", req);
                if (backpack.contains(req) && !IsStringArrayNonEmpty(backpack.at(req)))
                {
                    issues.push_back({Issue::Severity::Error,
                                      JoinPath("/backpack", req),
                                      "expected string[] (no empty strings)"});
                }
            }
            for (auto it = backpack.begin(); it != backpack.end(); ++it)
            {
                const std::string k = it.key();
                const auto& v = it.value();
                if (!IsStringArrayNonEmpty(v))
                {
                    issues.push_back(
                        {Issue::Severity::Error, JoinPath("/backpack", k), "expected string[] (no empty strings)"});
                }
            }
        }
    }

    return issues;
}

}
