
#include "elements/WeaponPrinter.h"
#include "syshelpers/UtilTypes.h"
#include "syshelpers/Utilities.h"

WeaponPrinter::WeaponPrinter(const Config& rawWeaponConfig,
                             const int statModParam,
                             const int proficiencyBonusParam,
                             const bool isRangedParam)
    : weaponCfg(rawWeaponConfig, isRangedParam, proficiencyBonusParam > 0),
      statMod(statModParam),
      proficiencyBonus(proficiencyBonusParam),
      isRanged(isRangedParam)

{
    RenderName();
    RenderRangeType();
    RenderRawDamage();
    RenderProps();
    RenderProfLabel();
    RenderTotals();
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
    block.textSpans.push_back(
        {" " + weaponCfg.damageBase.damageChunkTyped, UtilType::NORMAL_TEXT, false, false});
}

void WeaponPrinter::RenderProps()
{
    if (!weaponCfg.props.empty())
    {
        block.textSpans.push_back({", " + JoinProps(), UtilType::NORMAL_TEXT, false, false});
    }
}

void WeaponPrinter::RenderProfLabel()
{
    if (proficiencyBonus > 0)
    {
        block.textSpans.push_back({", proficient", UtilType::NORMAL_TEXT, false, false});
    }
}

void WeaponPrinter::RenderTotals()
{
    block.textSpans.push_back({" - TOTAL:", UtilType::BOLD_TEXT, false, true});
    block.textSpans.push_back({BuildWeaponTotalString(), UtilType::NORMAL_TEXT, false, false});
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

std::string WeaponPrinter::BuildWeaponTotalString() const
{
    const int hitTotal = statMod + (weaponCfg.proficient ? proficiencyBonus : 0) + weaponCfg.damageBase.bonus;
    const std::string dmgDisplay = BuildDamageSumDisplay() + " " + weaponCfg.damageBase.type;

    return std::string(" Hit Chance: ") + Utilities::FormatSignedInt(hitTotal) + ", Dmg: " + dmgDisplay;
}

std::string WeaponPrinter::BuildDamageSumDisplay() const
{
    std::string out = weaponCfg.damageBase.dice;
    int bonus = weaponCfg.damageBase.bonus;
    if (weaponCfg.proficient && statMod != 0) { bonus += statMod; }
    if (bonus != 0) { out += Utilities::FormatSignedInt(bonus); }
    return out;
}

UtilType::FormattedLabeledBlock WeaponPrinter::Render() const { return block; }