#include "pagetypes/InventoryPage.h"

#include <algorithm>
#include <string>

#include "elements/TextBox.h"
#include "syshelpers/PageConstants.h"
#include "syshelpers/Utilities.h"

using namespace PageConstants;

namespace InventoryPageTextStyle
{
inline constexpr double FONTSIZE = 10.0;
inline const UtilType::TextOptions NORMAL_TEXT{FontType::Arial, FONTSIZE};
inline const UtilType::TextOptions BOLD_TEXT{FontType::ArialBold, FONTSIZE};
} // namespace InventoryPageTextStyle

namespace
{

static const std::vector<std::string> RANGED_WEAPON_TYPES{};

bool IsRangedWeaponType(const std::string& type)
{
    return std::find(RANGED_WEAPON_TYPES.begin(), RANGED_WEAPON_TYPES.end(), type) !=
           RANGED_WEAPON_TYPES.end();
}

bool WeaponTypeInProficiencyArray(const Config& proficiencies,
                                  const char* arrayKey,
                                  const std::string& weaponType)
{
    if (!proficiencies.hasKey(arrayKey)) { return false; }
    for (const std::string& entry : proficiencies.getStringArray(arrayKey))
    {
        if (entry == weaponType) { return true; }
    }
    return false;
}

bool IsWeaponTypeProficient(const Config& config, const std::string& weaponType)
{
    if (!config.hasKey("proficiencies")) { return false; }
    const Config proficiencies = config.getObject("proficiencies");
    return WeaponTypeInProficiencyArray(proficiencies, "simpleWeapons", weaponType) ||
           WeaponTypeInProficiencyArray(proficiencies, "martialWeapons", weaponType);
}

UtilType::FormattedLabeledBlock MakeStashedHeadingBlock()
{
    UtilType::FormattedLabeledBlock block;
    block.textSpans.push_back({"Stashed:", InventoryPageTextStyle::BOLD_TEXT, false, true});
    return block;
}

std::string JoinProps(const std::vector<std::string>& props)
{
    std::string propsStr;
    for (size_t i = 0; i < props.size(); ++i)
    {
        propsStr += props[i];
        if (i < props.size() - 1) { propsStr += ", "; }
    }
    return propsStr;
}

std::string BuildDamageSumDisplay(const std::string& dice, int baseBonus, bool proficient, int statMod)
{
    std::string out = dice;
    if (baseBonus != 0) { out += Utilities::FormatSignedInt(baseBonus); }
    if (proficient && statMod != 0) { out += Utilities::FormatSignedInt(statMod); }
    return out;
}

std::string ArmorTextAfterColon(const Config& armorCfg)
{
    const Config ac = armorCfg.getObject("ac");
    if (ac.hasKey("fixmod")) { return std::string(" AC ") + Utilities::FormatSignedInt(ac.getInt("fixmod")); }

    const int base = ac.hasKey("base") ? ac.getInt("base") : 0;
    if (ac.hasKey("modstat"))
    {
        const std::string modstat = ac.getString("modstat");
        std::string tail = std::string(" AC ") + std::to_string(base) + " + " + modstat + " modifier";
        if (ac.hasKey("modcap") && ac.getInt("modcap") != 0)
        {
            tail += " (max " + std::to_string(ac.getInt("modcap")) + ")";
        }
        return tail;
    }

    return std::string(" AC ") + std::to_string(base);
}

UtilType::FormattedLabeledBlock BuildArmorBlock(const Config& armorCfg)
{
    const std::string name = armorCfg.getString("name");
    const std::string type = armorCfg.getString("type");

    UtilType::FormattedLabeledBlock block;
    block.labelParts.push_back({name, InventoryPageTextStyle::NORMAL_TEXT, true});
    if (name != type)
    {
        block.labelParts.push_back(
            {std::string(" (") + type + ")", InventoryPageTextStyle::NORMAL_TEXT, false});
    }
    block.textSpans.push_back({":", InventoryPageTextStyle::NORMAL_TEXT, false, false});
    block.textSpans.push_back(
        {ArmorTextAfterColon(armorCfg), InventoryPageTextStyle::NORMAL_TEXT, false, false});
    return block;
}

} // namespace

InventoryPage::InventoryPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params)
    : PageBase(doc, config, side, params), proficiencyBonus(0)
{
}

void InventoryPage::appendWeaponBlocks(const std::vector<Config>& weapons,
                                       std::vector<UtilType::FormattedLabeledBlock>& outBlocks)
{
    for (const Config& w : weapons)
    {
        const std::string weaponName = w.getString("name");
        const std::string weaponType = w.getString("type");
        const bool proficient = IsWeaponTypeProficient(config, weaponType);

        const bool ranged = IsRangedWeaponType(weaponType);
        const std::string statName = ranged ? "dexterity" : "strength";
        const int statMod = CalcModFromStatName(statName);

        const Config dmgBase = w.getObject("damage").getObject("base");
        const std::string dice = dmgBase.getString("dice");
        const int baseBonus = dmgBase.getInt("bonus");
        const std::string dmgType = dmgBase.getString("type");

        std::vector<std::string> props;
        if (w.hasKey("props")) { props = w.getStringArray("props"); }

        std::string damageChunk = dice;
        if (baseBonus != 0) { damageChunk += Utilities::FormatSignedInt(baseBonus); }
        damageChunk += " ";
        damageChunk += dmgType;

        const int hitTotal = statMod + (proficient ? proficiencyBonus : 0) + baseBonus;
        const std::string dmgDisplay = BuildDamageSumDisplay(dice, baseBonus, proficient, statMod);

        UtilType::FormattedLabeledBlock block;
        block.labelParts.push_back({weaponName, InventoryPageTextStyle::NORMAL_TEXT, true});
        if (weaponName != weaponType)
        {
            block.labelParts.push_back(
                {std::string(" (") + weaponType + ")", InventoryPageTextStyle::NORMAL_TEXT, false});
        }
        block.textSpans.push_back({":", InventoryPageTextStyle::NORMAL_TEXT, false, false});
        block.textSpans.push_back({std::string(ranged ? " Ranged (DEX)" : " Melee (STR)"),
                                   InventoryPageTextStyle::NORMAL_TEXT,
                                   false,
                                   false});
        block.textSpans.push_back({" " + damageChunk, InventoryPageTextStyle::NORMAL_TEXT, false, false});
        if (!props.empty())
        {
            block.textSpans.push_back(
                {", " + JoinProps(props), InventoryPageTextStyle::NORMAL_TEXT, false, false});
        }
        if (proficient)
        {
            block.textSpans.push_back({", proficient", InventoryPageTextStyle::NORMAL_TEXT, false, false});
        }

        block.textSpans.push_back({" - TOTAL:", InventoryPageTextStyle::BOLD_TEXT, false, true});
        block.textSpans.push_back(
            {std::string(" Hit Chance: ") + Utilities::FormatSignedInt(hitTotal) + ", Dmg: " + dmgDisplay,
             InventoryPageTextStyle::NORMAL_TEXT,
             false,
             false});

        outBlocks.push_back(std::move(block));
    }
}

void InventoryPage::appendArmorBlocks(const std::vector<Config>& armors,
                                      std::vector<UtilType::FormattedLabeledBlock>& outBlocks)
{
    for (const Config& a : armors)
    {
        outBlocks.push_back(BuildArmorBlock(a));
    }
}

void InventoryPage::Draw()
{
    DecorateCorners();
    doc.AddImage("assets/box-wide.png", LEFT_EDGE_REF + 18, MARGIN, 388, 281);
    doc.AddImage("assets/box-wide-bottomribbon.png", LEFT_EDGE_REF + MARGIN, 281 + MARGIN, 422, 316);
}

void InventoryPage::Fill()
{
    doc.AddTextCurved("Backpack",
                      Coords(LEFT_EDGE_REF + MARGIN + 112, 502),
                      99,
                      UtilType::TextOptions(FontType::Seagram, 28, 0, 11));

    if (!config.hasKey("equipment"))
    {
        Utilities::LogError("No equipment found");
        return;
    }

    const Config equipment = config.getObject("equipment");
    std::vector<UtilType::FormattedLabeledBlock> blocks;

    proficiencyBonus = 0;
    if (config.hasKey("proficiencies") && config.getObject("proficiencies").hasKey("bonus"))
    {
        proficiencyBonus = config.getObject("proficiencies").getInt("bonus");
    }
    else
    {
        Utilities::LogError("No proficiency bonus found, using 0 instead.");
    }

    if (equipment.hasKey("used"))
    {
        const Config used = equipment.getObject("used");
        if (used.hasKey("weapons")) { appendWeaponBlocks(used.getObjectArray("weapons"), blocks); }
        if (used.hasKey("armors")) { appendArmorBlocks(used.getObjectArray("armors"), blocks); }
    }

    blocks.push_back(MakeStashedHeadingBlock());

    if (equipment.hasKey("stashed"))
    {
        const Config stashed = equipment.getObject("stashed");
        if (stashed.hasKey("weapons")) { appendWeaponBlocks(stashed.getObjectArray("weapons"), blocks); }
        if (stashed.hasKey("armors")) { appendArmorBlocks(stashed.getObjectArray("armors"), blocks); }
    }

    if (blocks.empty()) { return; }

    TextBox box = TextBox::CreateStandard(
        *this, LEFT_EDGE_REF + 20, 3 * MARGIN, InventoryPageTextStyle::FONTSIZE, {368});
    box.RenderFormattedBlocks(blocks);
}
