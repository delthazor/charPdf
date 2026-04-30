#include "pagetypes/InventoryPage.h"

#include <algorithm>
#include <string>

#include "elements/TextBox.h"
#include "elements/WeaponPrinter.h"
#include "syshelpers/PageConstants.h"
#include "syshelpers/Utilities.h"

namespace
{

static const std::vector<std::string> RANGED_WEAPON_TYPES{};

constexpr double EQUIPMENT_X_OFFSET = 18.0;
constexpr double EQUIPMENT_WIDTH = 388.0;
constexpr double EQUIPMENT_HEIGHT = 281.0;
constexpr double EQUIPMENT_TEXT_X_OFFSET = 30.0;
constexpr double EQUIPMENT_TEXT_Y_OFFSET = 18.0;

constexpr double BACKPACK_WIDTH = 422.0;
constexpr double BACKPACK_HEIGHT = 316.0;

constexpr double BACKPACK_COLUMN_WIDTH = 110.0;
constexpr double BACKPACK_COLUMN_GAP = 5.0;
constexpr double BACKPACK_COLUMN_START_X = 30.0;
constexpr double BACKPACK_COLUMN_START_Y = 299.0;

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
    block.textSpans.push_back({"Stashed:", UtilType::BOLD_TEXT, false, true});
    return block;
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
    block.labelParts.push_back({name, UtilType::NORMAL_TEXT, true});
    if (name != type)
    {
        block.labelParts.push_back({std::string(" (") + type + ")", UtilType::NORMAL_TEXT, false});
    }
    block.textSpans.push_back({":", UtilType::NORMAL_TEXT, false, false});
    block.textSpans.push_back({ArmorTextAfterColon(armorCfg), UtilType::NORMAL_TEXT, false, false});
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
    if (weapons.empty()) { return; }

    Utilities::LogInfo("   - Appending weapon blocks");
    for (const Config& w : weapons)
    {
        const bool isRanged = IsRangedWeaponType(w.getString("type"));
        const bool isProficient = IsWeaponTypeProficient(config, w.getString("type"));

        UtilType::FormattedLabeledBlock block =
            WeaponPrinter(w,
                          CalcModFromStatName(isRanged ? "dexterity" : "strength"),
                          isProficient ? proficiencyBonus : 0,
                          isRanged)
                .Render();

        outBlocks.push_back(std::move(block));
    }
}

void InventoryPage::appendArmorBlocks(const std::vector<Config>& armors,
                                      std::vector<UtilType::FormattedLabeledBlock>& outBlocks)
{
    if (armors.empty()) { return; }

    Utilities::LogInfo("   - Appending armor blocks");
    for (const Config& a : armors)
    {
        outBlocks.push_back(BuildArmorBlock(a));
    }
}

void InventoryPage::Draw()
{
    DecorateCorners();
    doc.AddImage("assets/box-wide.png",
                 LEFT_EDGE_REF + EQUIPMENT_X_OFFSET,
                 PageConstants::MARGIN,
                 EQUIPMENT_WIDTH,
                 EQUIPMENT_HEIGHT);
    doc.AddImage("assets/box-wide-bottomribbon.png",
                 LEFT_EDGE_REF + PageConstants::MARGIN,
                 EQUIPMENT_HEIGHT + PageConstants::MARGIN,
                 BACKPACK_WIDTH,
                 BACKPACK_HEIGHT);
}

void InventoryPage::loadProficiencyBonusFromConfig()
{
    proficiencyBonus = 0;
    if (config.hasKey("proficiencies") && config.getObject("proficiencies").hasKey("bonus"))
    {
        proficiencyBonus = config.getObject("proficiencies").getInt("bonus");
    }
    else
    {
        Utilities::LogError("No proficiency bonus found, using 0 instead.");
    }
}

void InventoryPage::buildAndRenderEquipmentBlocks()
{
    if (!config.hasKey("equipment"))
    {
        Utilities::LogError("No equipment section found");
        return;
    }

    const Config& equipment = config.getObject("equipment");
    std::vector<UtilType::FormattedLabeledBlock> blocks;

    if (equipment.hasKey("used"))
    {
        Utilities::LogInfo(" - Building used equipment blocks");
        const Config used = equipment.getObject("used");
        if (used.hasKey("weapons")) { appendWeaponBlocks(used.getObjectArray("weapons"), blocks); }
        if (used.hasKey("armors")) { appendArmorBlocks(used.getObjectArray("armors"), blocks); }
    }

    blocks.push_back(MakeStashedHeadingBlock());

    if (equipment.hasKey("stashed"))
    {
        Utilities::LogInfo(" - Building stashed equipment blocks");
        const Config stashed = equipment.getObject("stashed");
        if (stashed.hasKey("weapons")) { appendWeaponBlocks(stashed.getObjectArray("weapons"), blocks); }
        if (stashed.hasKey("armors")) { appendArmorBlocks(stashed.getObjectArray("armors"), blocks); }
    }

    if (blocks.empty()) { return; }

    TextBox box = TextBox::CreateStandard(*this,
                                          LEFT_EDGE_REF + EQUIPMENT_TEXT_X_OFFSET,
                                          EQUIPMENT_TEXT_Y_OFFSET,
                                          PageConstants::EQUIPMENT_FONTSIZE,
                                          {EQUIPMENT_WIDTH - EQUIPMENT_X_OFFSET});
    box.RenderFormattedBlocks(blocks);
}

void InventoryPage::buildAndRenderBackpack()
{
    if (!config.hasKey("backpack"))
    {
        Utilities::LogError("No backpack section found");
        return;
    }

    const Config& backpack = config.getObject("backpack");
    size_t columnIndex = 0;

    for (const std::string& key : backpack.getKeys())
    {
        if (!backpack.isStringArray(key))
        {
            Utilities::LogError(std::string("backpack.") + key + " must be a string array; skipping");
        }
        else
        {
            const double columnX =
                LEFT_EDGE_REF + BACKPACK_COLUMN_START_X +
                static_cast<double>(columnIndex) * (BACKPACK_COLUMN_WIDTH + BACKPACK_COLUMN_GAP);
            TextBox box = TextBox::CreateStandard(*this,
                                                  columnX,
                                                  BACKPACK_COLUMN_START_Y,
                                                  PageConstants::EQUIPMENT_FONTSIZE,
                                                  {BACKPACK_COLUMN_WIDTH});
            box.RenderPlainTextLines(backpack.getStringArray(key));
            ++columnIndex;
        }
    }
}

void InventoryPage::Fill()
{
    doc.AddTextCurved("Backpack",
                      Coords(LEFT_EDGE_REF + PageConstants::MARGIN + 112, 502),
                      99,
                      UtilType::TextOptions(PageConstants::FontType::Seagram, 28, 0, 11));

    loadProficiencyBonusFromConfig();
    buildAndRenderEquipmentBlocks();
    buildAndRenderBackpack();
}
