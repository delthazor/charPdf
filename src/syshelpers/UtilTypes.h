#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "syshelpers/Config.h"
#include "syshelpers/PageConstants.h"

namespace UtilType
{

struct TextOptions
{
    const PageConstants::FontType fontType = PageConstants::FontType::Seagram;
    const double fontSize = 12;
    const double rotationAngle = 0;
    const double letterSpacing = 0;

    constexpr TextOptions() = default;
    constexpr TextOptions(const PageConstants::FontType fontType) : fontType(fontType) {}
    constexpr TextOptions(const double fontSize) : fontSize(fontSize) {}
    constexpr TextOptions(const PageConstants::FontType fontType, const double fontSize)
        : fontType(fontType), fontSize(fontSize)
    {
    }
    constexpr TextOptions(const PageConstants::FontType fontType,
                          const double fontSize,
                          const double rotationAngle,
                          const double letterSpacing)
        : fontType(fontType), fontSize(fontSize), rotationAngle(rotationAngle), letterSpacing(letterSpacing)
    {
    }
};

constexpr TextOptions NORMAL_TEXT(PageConstants::FontType::Arial, PageConstants::EQUIPMENT_FONTSIZE);
constexpr TextOptions BOLD_TEXT(PageConstants::FontType::ArialBold, PageConstants::EQUIPMENT_FONTSIZE);

using MeasureFn = std::function<double(const std::string&, const TextOptions&)>;

struct LabeledTextBlock
{
    std::string label;
    std::string text;
    LabeledTextBlock(const std::string& label, const std::string& text) : label(label), text(text) {}
};

struct FormattedLabelPart
{
    std::string text;
    TextOptions textOptions;
    bool underline = false;
};

struct FormattedTextSpan
{
    std::string text;
    TextOptions textOptions;
    bool underline = false;
    bool forceLineBreakBefore = false;
};

struct FormattedLabeledBlock
{
    std::vector<FormattedLabelPart> labelParts;
    std::vector<FormattedTextSpan> textSpans;
};

struct StyledWord
{
    std::string text;
    TextOptions textOptions;
    bool underline = false;
    bool forceLineBreakBefore = false;
};

struct TraitsCatalog
{
    nlohmann::ordered_json raw;
    std::unordered_map<std::string, nlohmann::ordered_json> byName;
};

struct SpellsCatalog
{
    nlohmann::ordered_json raw;
    std::unordered_map<std::string, nlohmann::ordered_json> byName;
};

struct DamageConfig
{
    const std::string dice;
    const int bonus;
    const std::string type;
    const std::string damageChunkBare;
    const std::string damageChunkTyped;

    DamageConfig(Config dmgCfg)
        : dice(dmgCfg.getString("dice")),
          bonus(dmgCfg.getInt("bonus")),
          type(dmgCfg.getString("type")),
          damageChunkBare(DamageChunkBare()),
          damageChunkTyped(DamageChunkTyped())
    {
    }

  private:
    std::string DamageChunkBare() const
    {
        std::string damageChunk = dice;
        if (bonus != 0) { damageChunk += (bonus > 0 ? "+" : "") + std::to_string(bonus); }
        return damageChunk;
    }

    std::string DamageChunkTyped() const { return DamageChunkBare() + " " + type; }
};

inline bool DamageAltIsPopulated(const Config& altObj)
{
    if (!altObj.hasKey("dice") || !altObj.hasKey("type")) { return false; }
    return !altObj.getString("dice").empty();
}

inline std::optional<DamageConfig> ParseWeaponDamageAlt(const Config& damageSection)
{
    if (!damageSection.hasKey("alt")) { return std::nullopt; }
    const Config alt = damageSection.getObject("alt");
    if (!DamageAltIsPopulated(alt)) { return std::nullopt; }
    return DamageConfig(alt);
}

struct WeaponConfig
{
    const std::string name;
    const std::string type;
    const DamageConfig damageBase;
    const std::optional<DamageConfig> damageAlt;
    const std::vector<std::string> damageExtra;
    const std::vector<std::string> props;
    const bool proficient;
    const bool ranged;
    const std::string range;
    const std::string extratext;

    WeaponConfig(Config weaponCfg, bool isRanged, bool isProficient)
        : name(weaponCfg.getString("name")),
          type(weaponCfg.getString("type")),
          damageBase(weaponCfg.getObject("damage").getObject("base")),
          damageAlt(ParseWeaponDamageAlt(weaponCfg.getObject("damage"))),
          damageExtra(weaponCfg.getObject("damage").getStringArray("extra")),
          props(weaponCfg.getStringArray("props")),
          proficient(isProficient),
          ranged(isRanged),
          range(weaponCfg.hasKey("range") ? weaponCfg.getString("range") : ""),
          extratext(weaponCfg.hasKey("extratext") ? weaponCfg.getString("extratext") : "")
    {
    }
};

}
