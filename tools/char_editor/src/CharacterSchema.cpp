#include "CharacterSchema.h"

#include "CharacterJsonValidator.h"

#include <cmath>
#include <string>
#include <vector>

namespace CharEditor
{

namespace
{

void EnsureString(nlohmann::ordered_json& obj, const char* key, const std::string& def)
{
    if (!obj.contains(key) || !obj[key].is_string()) { obj[key] = def; }
}

void EnsureNonEmptyString(nlohmann::ordered_json& obj, const char* key, const std::string& def)
{
    EnsureString(obj, key, def);
    if (obj.contains(key) && obj[key].is_string() && obj[key].get<std::string>().empty()) { obj[key] = def; }
}

void EnsureInt(nlohmann::ordered_json& obj, const char* key, int def)
{
    if (!obj.contains(key) || !obj[key].is_number_integer()) { obj[key] = def; }
}

void EnsureNameLetterSpacing(nlohmann::ordered_json& doc)
{
    constexpr double kDef = 1.0;
    constexpr double kMin = -50.0;
    constexpr double kMax = 50.0;
    if (!doc.contains("nameLetterSpacing") || !doc["nameLetterSpacing"].is_number())
    {
        doc["nameLetterSpacing"] = kDef;
        return;
    }
    double v = doc["nameLetterSpacing"].is_number_integer()
                   ? static_cast<double>(doc["nameLetterSpacing"].get<int>())
                   : doc["nameLetterSpacing"].get<double>();
    if (!std::isfinite(v)) { v = kDef; }
    if (v < kMin) { v = kMin; }
    if (v > kMax) { v = kMax; }
    v = std::round(v * 100.0) / 100.0;
    doc["nameLetterSpacing"] = v;
}

void EnsureNonNegInt(nlohmann::ordered_json& obj, const char* key, int def)
{
    EnsureInt(obj, key, def);
    if (obj.contains(key) && obj[key].is_number_integer() && obj[key].get<int>() < 0) { obj[key] = def; }
}

void EnsureNonZeroNonNegInt(nlohmann::ordered_json& obj, const char* key, int def)
{
    EnsureNonNegInt(obj, key, def);
    if (obj.contains(key) && obj[key].is_number_integer() && obj[key].get<int>() == 0) { obj[key] = def; }
}

void EnsureObject(nlohmann::ordered_json& obj, const char* key)
{
    if (!obj.contains(key) || !obj[key].is_object()) { obj[key] = nlohmann::ordered_json::object(); }
}

void EnsureArray(nlohmann::ordered_json& obj, const char* key)
{
    if (!obj.contains(key) || !obj[key].is_array()) { obj[key] = nlohmann::ordered_json::array(); }
}

void EnsureStatsNonNegInt(nlohmann::ordered_json& stats, const char* key, int def)
{
    if (!stats.contains(key) || !stats[key].is_number_integer())
    {
        stats[key] = def;
        return;
    }
    if (stats[key].get<int>() < 0) { stats[key] = def; }
}

void EnsureStringArrayElementsNonEmpty(nlohmann::ordered_json& arr)
{
    if (!arr.is_array()) { arr = nlohmann::ordered_json::array(); }
    nlohmann::ordered_json filtered = nlohmann::ordered_json::array();
    for (auto& el : arr)
    {
        if (!el.is_string()) { el = el.dump(); }
        if (!el.is_string()) { continue; }
        const std::string s = el.get<std::string>();
        if (s.empty()) { continue; }
        filtered.push_back(s);
    }
    arr = filtered;
}

void EnsureProficiencyStringArray(nlohmann::ordered_json& prof, const char* key)
{
    EnsureArray(prof, key);
    EnsureStringArrayElementsNonEmpty(prof[key]);
}

// After Ensure* calls, put top-level keys back to the order they had at snapshot time,
// then append any keys that were added (e.g. first-time normalization).
void RestoreTopLevelKeyOrder(nlohmann::ordered_json& obj, const std::vector<std::string>& keyOrder)
{
    if (!obj.is_object() || keyOrder.empty()) { return; }
    nlohmann::ordered_json rebuilt = nlohmann::ordered_json::object();
    for (const auto& k : keyOrder)
    {
        if (obj.contains(k)) { rebuilt[k] = obj.at(k); }
    }
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        if (!rebuilt.contains(it.key())) { rebuilt[it.key()] = it.value(); }
    }
    obj.swap(rebuilt);
}

// Matches hand-edited character JSON (e.g. assets/cfg/chars/bnd/char_Celdrick.json).
void RestoreClassObjectKeyOrder(nlohmann::ordered_json& classObj)
{
    static const char* kOrder[] = {
        "level", "hitDice", "resourcePoints", "castStat", "subclass", "spellslots", "spells"};
    if (!classObj.is_object()) { return; }
    nlohmann::ordered_json rebuilt = nlohmann::ordered_json::object();
    for (const char* k : kOrder)
    {
        if (classObj.contains(k)) { rebuilt[k] = classObj.at(k); }
    }
    for (auto it = classObj.begin(); it != classObj.end(); ++it)
    {
        if (!rebuilt.contains(it.key())) { rebuilt[it.key()] = it.value(); }
    }
    classObj.swap(rebuilt);
}

void EnsureEquipmentBucket(nlohmann::ordered_json& doc, const char* bucketKey)
{
    EnsureObject(doc, "equipment");
    auto& eq = doc["equipment"];
    if (!eq.contains(bucketKey) || !eq[bucketKey].is_object()) { eq[bucketKey] = nlohmann::ordered_json::object(); }
    auto& bucket = eq[bucketKey];
    if (!bucket.contains("weapons") || !bucket["weapons"].is_array()) { bucket["weapons"] = nlohmann::ordered_json::array(); }
    if (!bucket.contains("armors") || !bucket["armors"].is_array()) { bucket["armors"] = nlohmann::ordered_json::array(); }
}

void ReorderWeaponKeysRangeBeforeExtraText(nlohmann::ordered_json& w)
{
    if (!w.is_object()) { return; }
    if (!w.contains("range") || !w.contains("extratext")) { return; }

    nlohmann::ordered_json reordered = nlohmann::ordered_json::object();
    for (const char* k : {"name", "type", "props", "damage", "range", "extratext"})
    {
        if (w.contains(k)) { reordered[k] = w.at(k); }
    }
    for (auto it = w.begin(); it != w.end(); ++it)
    {
        if (reordered.contains(it.key())) { continue; }
        reordered[it.key()] = it.value();
    }
    w = reordered;
}

}

nlohmann::ordered_json CharacterSchema::MakeDefaultCharacter()
{
    nlohmann::ordered_json doc;
    doc["name"] = "New Character";
    doc["nameLetterSpacing"] = 1.0;
    doc["background"] = "";
    doc["race"] = "Human";

    doc["stats"] = nlohmann::ordered_json::object();
    auto& stats = doc["stats"];
    stats["strength"] = 0;
    stats["dexterity"] = 0;
    stats["constitution"] = 0;
    stats["intelligence"] = 0;
    stats["wisdom"] = 0;
    stats["charisma"] = 0;
    stats["speed"] = 30;
    stats["maxHp"] = 10;
    stats["ac"] = 14;
    stats["initiativeBonus"] = 0;
    stats["pPercBonus"] = 0;

    doc["proficiencies"] = nlohmann::ordered_json::object();
    auto& prof = doc["proficiencies"];
    prof["bonus"] = 2;

    prof["skills"] = nlohmann::ordered_json::object();
    auto& skills = prof["skills"];
    skills["strength"] = nlohmann::ordered_json::object({{"athletics", 0}});
    skills["dexterity"] =
        nlohmann::ordered_json::object({{"acrobatics", 0}, {"sleightOfHand", 0}, {"stealth", 0}});
    skills["intelligence"] = nlohmann::ordered_json::object(
        {{"arcana", 0}, {"history", 0}, {"investigation", 0}, {"nature", 0}, {"religion", 0}});
    skills["wisdom"] = nlohmann::ordered_json::object(
        {{"animalHandling", 0}, {"insight", 0}, {"medicine", 0}, {"perception", 0}, {"survival", 0}});
    skills["charisma"] = nlohmann::ordered_json::object(
        {{"deception", 0}, {"intimidation", 0}, {"performance", 0}, {"persuasion", 0}});

    prof["savingThrows"] = nlohmann::ordered_json::object({{"strength", 0},
                                                           {"dexterity", 0},
                                                           {"constitution", 0},
                                                           {"intelligence", 0},
                                                           {"wisdom", 0},
                                                           {"charisma", 0}});

    prof["tools"] = nlohmann::ordered_json::array();
    prof["languages"] = nlohmann::ordered_json::array();
    prof["armors"] = nlohmann::ordered_json::array();
    prof["simpleWeapons"] = nlohmann::ordered_json::array();
    prof["martialWeapons"] = nlohmann::ordered_json::array();

    doc["traits"] = nlohmann::ordered_json::array();
    doc["classes"] = nlohmann::ordered_json::object();
    {
        auto& cls = doc["classes"]["cleric"];
        cls = nlohmann::ordered_json::object();
        cls["level"] = 1;
        cls["hitDice"] = "d8";
        cls["resourcePoints"] = 0;
        cls["castStat"] = "";
        cls["subclass"] = "";
        cls["spellslots"] = nlohmann::ordered_json::object(
            {{"1", 0}, {"2", 0}, {"3", 0}, {"4", 0}, {"5", 0}, {"6", 0}, {"7", 0}, {"8", 0}, {"9", 0}});
        cls["spells"] = nlohmann::ordered_json::object({{"0", nlohmann::ordered_json::array()},
                                                        {"0_extra", nlohmann::ordered_json::array()},
                                                        {"1", nlohmann::ordered_json::array()},
                                                        {"2", nlohmann::ordered_json::array()},
                                                        {"3", nlohmann::ordered_json::array()},
                                                        {"4", nlohmann::ordered_json::array()},
                                                        {"5", nlohmann::ordered_json::array()},
                                                        {"6", nlohmann::ordered_json::array()},
                                                        {"7", nlohmann::ordered_json::array()},
                                                        {"8", nlohmann::ordered_json::array()},
                                                        {"9", nlohmann::ordered_json::array()}});
    }
    doc["equipment"] = nlohmann::ordered_json::object(
        {{"used",
          nlohmann::ordered_json::object(
              {{"weapons", nlohmann::ordered_json::array()}, {"armors", nlohmann::ordered_json::array()}})},
         {"stashed",
          nlohmann::ordered_json::object(
              {{"weapons", nlohmann::ordered_json::array()}, {"armors", nlohmann::ordered_json::array()}})}});
    doc["backpack"] = nlohmann::ordered_json::object();
    auto& backpack = doc["backpack"];
    backpack["accessories"] = nlohmann::ordered_json::array();
    backpack["consumables"] = nlohmann::ordered_json::array();
    backpack["kits & tools"] = nlohmann::ordered_json::array();
    backpack["general"] = nlohmann::ordered_json::array();

    return doc;
}

void CharacterSchema::NormalizeInPlace(nlohmann::ordered_json& doc)
{
    if (!doc.is_object()) { doc = nlohmann::ordered_json::object(); }

    EnsureNonEmptyString(doc, "name", "New Character");
    EnsureNameLetterSpacing(doc);
    EnsureString(doc, "background", "");
    EnsureNonEmptyString(doc, "race", "Human");

    if (!doc.contains("stats") || !doc["stats"].is_object())
    {
        doc["stats"] = nlohmann::ordered_json::object();
    }
    auto& stats = doc["stats"];
    EnsureStatsNonNegInt(stats, "strength", 0);
    EnsureStatsNonNegInt(stats, "dexterity", 0);
    EnsureStatsNonNegInt(stats, "constitution", 0);
    EnsureStatsNonNegInt(stats, "intelligence", 0);
    EnsureStatsNonNegInt(stats, "wisdom", 0);
    EnsureStatsNonNegInt(stats, "charisma", 0);
    EnsureStatsNonNegInt(stats, "speed", 30);
    if (stats.contains("speed") && stats["speed"].is_number_integer() && stats["speed"].get<int>() == 0) { stats["speed"] = 30; }
    EnsureStatsNonNegInt(stats, "maxHp", 10);
    if (stats.contains("maxHp") && stats["maxHp"].is_number_integer() && stats["maxHp"].get<int>() == 0) { stats["maxHp"] = 10; }
    EnsureStatsNonNegInt(stats, "ac", 14);
    if (stats.contains("ac") && stats["ac"].is_number_integer() && stats["ac"].get<int>() == 0) { stats["ac"] = 14; }
    EnsureStatsNonNegInt(stats, "initiativeBonus", 0);
    EnsureStatsNonNegInt(stats, "pPercBonus", 0);

    if (!doc.contains("proficiencies") || !doc["proficiencies"].is_object())
    {
        doc["proficiencies"] = nlohmann::ordered_json::object();
    }
    auto& prof = doc["proficiencies"];
    std::vector<std::string> profKeyOrder;
    profKeyOrder.reserve(prof.size());
    for (auto it = prof.begin(); it != prof.end(); ++it) { profKeyOrder.emplace_back(it.key()); }

    EnsureNonZeroNonNegInt(prof, "bonus", 2);

    if (!prof.contains("skills") || !prof["skills"].is_object())
    {
        prof["skills"] = nlohmann::ordered_json::object();
    }
    {
        auto& skills = prof["skills"];
        if (!skills.contains("strength") || !skills["strength"].is_object()) { skills["strength"] = nlohmann::ordered_json::object(); }
        if (!skills.contains("dexterity") || !skills["dexterity"].is_object()) { skills["dexterity"] = nlohmann::ordered_json::object(); }
        if (!skills.contains("intelligence") || !skills["intelligence"].is_object()) { skills["intelligence"] = nlohmann::ordered_json::object(); }
        if (!skills.contains("wisdom") || !skills["wisdom"].is_object()) { skills["wisdom"] = nlohmann::ordered_json::object(); }
        if (!skills.contains("charisma") || !skills["charisma"].is_object()) { skills["charisma"] = nlohmann::ordered_json::object(); }

        EnsureNonNegInt(skills["strength"], "athletics", 0);
        for (const char* k : {"acrobatics", "sleightOfHand", "stealth"}) { EnsureNonNegInt(skills["dexterity"], k, 0); }
        for (const char* k : {"arcana", "history", "investigation", "nature", "religion"})
        {
            EnsureNonNegInt(skills["intelligence"], k, 0);
        }
        for (const char* k : {"animalHandling", "insight", "medicine", "perception", "survival"})
        {
            EnsureNonNegInt(skills["wisdom"], k, 0);
        }
        for (const char* k : {"deception", "intimidation", "performance", "persuasion"})
        {
            EnsureNonNegInt(skills["charisma"], k, 0);
        }
    }
    if (!prof.contains("savingThrows") || !prof["savingThrows"].is_object())
    {
        prof["savingThrows"] = nlohmann::ordered_json::object();
    }
    for (const char* stat : {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"})
    {
        EnsureNonNegInt(prof["savingThrows"], stat, 0);
    }

    for (const char* key : {"tools", "languages", "armors", "simpleWeapons", "martialWeapons"})
    {
        EnsureProficiencyStringArray(prof, key);
    }

    RestoreTopLevelKeyOrder(prof, profKeyOrder);

    if (!doc.contains("traits") || !doc["traits"].is_array())
    {
        doc["traits"] = nlohmann::ordered_json::array();
    }
    EnsureStringArrayElementsNonEmpty(doc["traits"]);
    if (!doc.contains("classes") || !doc["classes"].is_object())
    {
        doc["classes"] = nlohmann::ordered_json::object();
    }
    if (doc["classes"].empty())
    {
        doc["classes"]["cleric"] = nlohmann::ordered_json::object();
    }
    if (!doc.contains("equipment") || !doc["equipment"].is_object())
    {
        doc["equipment"] = nlohmann::ordered_json::object();
    }
    if (!doc.contains("backpack") || !doc["backpack"].is_object())
    {
        doc["backpack"] = nlohmann::ordered_json::object();
    }

    // Equipment: ensure expected containers exist.
    EnsureEquipmentBucket(doc, "used");
    EnsureEquipmentBucket(doc, "stashed");

    // Classes: ensure required fields, spellslots, and spells structure.
    for (auto it = doc["classes"].begin(); it != doc["classes"].end(); ++it)
    {
        if (!it.value().is_object())
        {
            it.value() = nlohmann::ordered_json::object();
        }
        auto& classObj = it.value();

        EnsureNonZeroNonNegInt(classObj, "level", 1);
        EnsureNonEmptyString(classObj, "hitDice", "d8");
        EnsureNonNegInt(classObj, "resourcePoints", 0);
        EnsureString(classObj, "subclass", "");
        EnsureString(classObj, "castStat", "");

        if (!classObj.contains("spellslots") || !classObj["spellslots"].is_object())
        {
            classObj["spellslots"] = nlohmann::ordered_json::object();
        }
        for (int lvl = 1; lvl <= 9; ++lvl)
        {
            const std::string k = std::to_string(lvl);
            EnsureNonNegInt(classObj["spellslots"], k.c_str(), 0);
        }

        if (!classObj.contains("spells") || !classObj["spells"].is_object())
        {
            classObj["spells"] = nlohmann::ordered_json::object();
        }
        auto& spells = classObj["spells"];
        for (const char* k : {"0", "0_extra", "1", "2", "3", "4", "5", "6", "7", "8", "9"})
        {
            if (!spells.contains(k) || !spells[k].is_array()) { spells[k] = nlohmann::ordered_json::array(); }
            EnsureStringArrayElementsNonEmpty(spells[k]);
        }

        RestoreClassObjectKeyOrder(classObj);
    }

    // Equipment item normalization.
    for (const char* bucketKey : {"used", "stashed"})
    {
        auto& bucket = doc["equipment"][bucketKey];
        if (!bucket.is_object()) { bucket = nlohmann::ordered_json::object(); }
        if (!bucket.contains("weapons") || !bucket["weapons"].is_array()) { bucket["weapons"] = nlohmann::ordered_json::array(); }
        if (!bucket.contains("armors") || !bucket["armors"].is_array()) { bucket["armors"] = nlohmann::ordered_json::array(); }

        for (auto& w : bucket["weapons"])
        {
            if (!w.is_object()) { w = nlohmann::ordered_json::object(); }
            EnsureNonEmptyString(w, "name", "Dagger");
            EnsureNonEmptyString(w, "type", "Dagger");
            EnsureString(w, "extratext", "");
            if (w.contains("range"))
            {
                EnsureNonEmptyString(w, "range", "80/320");
            }
            EnsureArray(w, "props");
            EnsureStringArrayElementsNonEmpty(w["props"]);

            if (!w.contains("damage") || !w["damage"].is_object()) { w["damage"] = nlohmann::ordered_json::object(); }
            auto& dmg = w["damage"];
            if (!dmg.contains("base") || !dmg["base"].is_object()) { dmg["base"] = nlohmann::ordered_json::object(); }
            auto& base = dmg["base"];
            EnsureNonEmptyString(base, "dice", "d4");
            EnsureNonNegInt(base, "bonus", 0);
            EnsureNonEmptyString(base, "type", "piercing");
            if (!dmg.contains("alt") || !dmg["alt"].is_object()) { dmg["alt"] = nlohmann::ordered_json::object(); }
            EnsureArray(dmg, "extra");
            EnsureStringArrayElementsNonEmpty(dmg["extra"]);

            // If present, keep range before extratext in output.
            ReorderWeaponKeysRangeBeforeExtraText(w);
        }

        for (auto& a : bucket["armors"])
        {
            if (!a.is_object()) { a = nlohmann::ordered_json::object(); }
            EnsureNonEmptyString(a, "name", "Breastplate");
            EnsureNonEmptyString(a, "type", "Medium Armor");
            EnsureString(a, "extratext", "");
            if (!a.contains("ac") || !a["ac"].is_object()) { a["ac"] = nlohmann::ordered_json::object(); }
            auto& ac = a["ac"];

            const bool hasFix = ac.contains("fixmod");
            const bool hasBase = ac.contains("base");
            if (!hasFix && !hasBase)
            {
                ac["base"] = 14;
                ac["modstat"] = "dexterity";
                ac["modcap"] = 2;
            }
            if (ac.contains("base"))
            {
                EnsureNonZeroNonNegInt(ac, "base", 14);
                EnsureNonEmptyString(ac, "modstat", "dexterity");
                EnsureNonNegInt(ac, "modcap", 2);
                const int baseVal = ac["base"].get<int>();
                const std::string modstatVal = ac["modstat"].get<std::string>();
                const int modcapVal = ac["modcap"].get<int>();
                ac = nlohmann::ordered_json(
                    {{"base", baseVal}, {"modstat", modstatVal}, {"modcap", modcapVal}});
            }
            else
            {
                EnsureNonZeroNonNegInt(ac, "fixmod", 2);
                if (ac.contains("modstat")) { ac.erase("modstat"); }
                if (ac.contains("modcap")) { ac.erase("modcap"); }
                if (ac.contains("base")) { ac.erase("base"); }
                const int fixVal = ac["fixmod"].get<int>();
                ac = nlohmann::ordered_json({{"fixmod", fixVal}});
            }
        }
    }

    // Backpack required sections.
    EnsureArray(doc["backpack"], "accessories");
    EnsureArray(doc["backpack"], "consumables");
    EnsureArray(doc["backpack"], "kits & tools");
    EnsureArray(doc["backpack"], "general");
    EnsureStringArrayElementsNonEmpty(doc["backpack"]["accessories"]);
    EnsureStringArrayElementsNonEmpty(doc["backpack"]["consumables"]);
    EnsureStringArrayElementsNonEmpty(doc["backpack"]["kits & tools"]);
    EnsureStringArrayElementsNonEmpty(doc["backpack"]["general"]);
}

std::vector<ValidationIssue> CharacterSchema::Validate(const nlohmann::ordered_json& doc)
{
    std::vector<ValidationIssue> issues;
    for (const auto& i : CharacterJson::Validate(doc))
    {
        issues.push_back({i.severity == CharacterJson::Issue::Severity::Error ? ValidationIssue::Severity::Error
                                                                               : ValidationIssue::Severity::Warning,
                          i.jsonPath,
                          i.message});
    }
    return issues;
}

}
