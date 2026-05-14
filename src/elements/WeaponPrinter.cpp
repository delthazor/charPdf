
#include "elements/WeaponPrinter.h"
#include "syshelpers/UtilTypes.h"
#include "syshelpers/Utilities.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{

bool HasProp(const std::vector<std::string>& props, const char* key)
{
    return std::find(props.begin(), props.end(), std::string(key)) != props.end();
}

} // namespace

WeaponPrinter::WeaponPrinter(const Config& rawWeaponConfig,
                             const int strengthModParam,
                             const int dexterityModParam,
                             const int proficiencyBonusParam,
                             const bool isRangedParam)
    : weaponCfg(rawWeaponConfig, isRangedParam, proficiencyBonusParam > 0),
      strengthMod(strengthModParam),
      dexterityMod(dexterityModParam),
      proficiencyBonus(proficiencyBonusParam),
      isRanged(isRangedParam)

{
    RenderName();
    RenderRangeType();
    RenderRawDamage();
    RenderProps();
    RenderProfLabel();
    RenderTotals();
    RenderExtraText();
}

void WeaponPrinter::RenderName()
{
    block.labelParts.push_back({weaponCfg.name, UtilType::NORMAL_TEXT, true});
    if (weaponCfg.name != weaponCfg.type)
    {
        block.labelParts.push_back({std::string(" (") + weaponCfg.type + ")", UtilType::NORMAL_TEXT, false});
    }
    block.textSpans.push_back({":", UtilType::NORMAL_TEXT, false, false});
}

void WeaponPrinter::RenderRangeType()
{
    block.textSpans.push_back(
        {std::string(isRanged ? " Ranged (DEX)" : " Melee (STR)"), UtilType::NORMAL_TEXT, false, false});
}

void WeaponPrinter::RenderRawDamage()
{
    block.textSpans.push_back({" " + weaponCfg.damageBase.damageChunkTyped + BuildExtraDamages(),
                               UtilType::NORMAL_TEXT,
                               false,
                               false});
}

void WeaponPrinter::RenderProps()
{
    if (!weaponCfg.props.empty())
    {
        block.textSpans.push_back({", " + JoinProps(), UtilType::NORMAL_TEXT, false, false, true});
    }
}

void WeaponPrinter::RenderProfLabel()
{
    if (proficiencyBonus > 0)
    {
        block.textSpans.push_back({", proficient", UtilType::NORMAL_TEXT, false, false, true});
    }
}

void WeaponPrinter::AppendTotalLine(const std::string& extraBeforeColon,
                                    const int abilityMod,
                                    const UtilType::DamageConfig& damage)
{
    block.textSpans.push_back(
        {std::string(" - TOTAL") + extraBeforeColon + ":", UtilType::BOLD_TEXT, false, true});
    block.textSpans.push_back(
        {BuildWeaponTotalString(abilityMod, damage), UtilType::NORMAL_TEXT, false, false});
}

void WeaponPrinter::RenderVersatileTotals(const std::string& prefix, const int statModParam)
{
    const bool versatile = HasProp(weaponCfg.props, "versatile");

    if (versatile)
    {
        if (!weaponCfg.damageAlt.has_value())
        {
            Utilities::LogError("Weapon has versatile property but damage.alt is missing or empty.");
        }
        else
        {
            AppendTotalLine(prefix + " (OneHand)", strengthMod, weaponCfg.damageBase);
            AppendTotalLine(prefix + " (TwoHand)", strengthMod, weaponCfg.damageAlt.value());
        }
    }
    else
    {
        AppendTotalLine(prefix, statModParam, weaponCfg.damageBase);
    }
}

void WeaponPrinter::RenderTotals()
{
    const bool finesse = HasProp(weaponCfg.props, "finesse");

    if (finesse)
    {
        RenderVersatileTotals(" (STR)", strengthMod);
        RenderVersatileTotals(" (DEX)", dexterityMod);
    }

    else
    {
        const int mod = isRanged ? dexterityMod : strengthMod;
        RenderVersatileTotals("", mod);
    }
}

std::string WeaponPrinter::JoinProps() const
{
    std::string propsStr;
    for (size_t i = 0; i < weaponCfg.props.size(); ++i)
    {
        propsStr += weaponCfg.props[i];
        if (i < weaponCfg.props.size() - 1) { propsStr += ", "; }
    }
    return propsStr;
}

std::string WeaponPrinter::BuildWeaponTotalString(const int abilityMod,
                                                  const UtilType::DamageConfig& damage) const
{
    const int hitTotal = abilityMod + (weaponCfg.proficient ? proficiencyBonus : 0) + damage.bonus;
    const std::string dmgDisplay =
        BuildDamageSumDisplay(abilityMod, damage) + " " + damage.type + BuildExtraDamages();

    return std::string(" Hit Chance: ") + Utilities::FormatSignedInt(hitTotal) + ", Dmg: " + dmgDisplay;
}

std::string WeaponPrinter::BuildDamageSumDisplay(const int abilityMod,
                                                 const UtilType::DamageConfig& damage) const
{
    std::string out = damage.dice;
    int bonus = damage.bonus;
    if (weaponCfg.proficient && abilityMod != 0) { bonus += abilityMod; }
    if (bonus != 0) { out += Utilities::FormatSignedInt(bonus); }
    return out;
}

std::string WeaponPrinter::BuildExtraDamages() const
{
    std::string out;
    if (!weaponCfg.damageExtra.empty())
    {
        for (const std::string& extra : weaponCfg.damageExtra)
        {
            out += " " + extra;
        }
    }
    return out;
}

void WeaponPrinter::RenderExtraText()
{
    if (!weaponCfg.extratext.empty())
    {
        block.textSpans.push_back({weaponCfg.extratext, UtilType::NORMAL_TEXT, false, true});
    }
}

UtilType::FormattedLabeledBlock WeaponPrinter::Render() const { return block; }
