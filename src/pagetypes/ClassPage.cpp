#include "pagetypes/ClassPage.h"
#include "elements/TextBox.h"
#include "syshelpers/Utilities.h"

using namespace PageConstants;

namespace
{

std::vector<std::vector<Coords>> MakeSpellSlotMarkers(double leftEdgeRef)
{
    std::vector<std::vector<Coords>> out;
    out.push_back({Coords(leftEdgeRef + 60.1, 208),
                   Coords(leftEdgeRef + 69.3, 211),
                   Coords(leftEdgeRef + 78.5, 214),
                   Coords(leftEdgeRef + 88.0, 217),
                   Coords(leftEdgeRef + 98.1, 218),
                   Coords(leftEdgeRef + 108.2, 217),
                   Coords(leftEdgeRef + 118.4, 215),
                   Coords(leftEdgeRef + 127.6, 212),
                   Coords(leftEdgeRef + 136.8, 208)});
    out.push_back({Coords(leftEdgeRef + 178.1, 208),
                   Coords(leftEdgeRef + 188.1, 206),
                   Coords(leftEdgeRef + 198.6, 205),
                   Coords(leftEdgeRef + 209.1, 204),
                   Coords(leftEdgeRef + 219.5, 203.5),
                   Coords(leftEdgeRef + 229.6, 204),
                   Coords(leftEdgeRef + 240.1, 205),
                   Coords(leftEdgeRef + 250.6, 206),
                   Coords(leftEdgeRef + 260.1, 208)});
    out.push_back({Coords(leftEdgeRef + 298.1, 201.1),
                   Coords(leftEdgeRef + 307.6, 202),
                   Coords(leftEdgeRef + 317.4, 202.8),
                   Coords(leftEdgeRef + 327.3, 204.1),
                   Coords(leftEdgeRef + 336.9, 205.9),
                   Coords(leftEdgeRef + 346.2, 208.6),
                   Coords(leftEdgeRef + 355.8, 206.9),
                   Coords(leftEdgeRef + 364.8, 203.4),
                   Coords(leftEdgeRef + 373.6, 201.1)});
    out.push_back({Coords(leftEdgeRef + 58.4, 341),
                   Coords(leftEdgeRef + 68.3, 338),
                   Coords(leftEdgeRef + 77.9, 335),
                   Coords(leftEdgeRef + 87.7, 334),
                   Coords(leftEdgeRef + 98.4, 333),
                   Coords(leftEdgeRef + 108.6, 334),
                   Coords(leftEdgeRef + 119.1, 335),
                   Coords(leftEdgeRef + 129.1, 338),
                   Coords(leftEdgeRef + 139.2, 341)});
    out.push_back({Coords(leftEdgeRef + 174.3, 334),
                   Coords(leftEdgeRef + 184.5, 336.5),
                   Coords(leftEdgeRef + 194.9, 339),
                   Coords(leftEdgeRef + 205.3, 341),
                   Coords(leftEdgeRef + 215.8, 341.5),
                   Coords(leftEdgeRef + 226.6, 341),
                   Coords(leftEdgeRef + 237.4, 339.5),
                   Coords(leftEdgeRef + 247.8, 338),
                   Coords(leftEdgeRef + 258.2, 336)});
    out.push_back({Coords(leftEdgeRef + 295.8, 340),
                   Coords(leftEdgeRef + 305.1, 336),
                   Coords(leftEdgeRef + 314.9, 333),
                   Coords(leftEdgeRef + 325.2, 331),
                   Coords(leftEdgeRef + 335.5, 332),
                   Coords(leftEdgeRef + 345.4, 334),
                   Coords(leftEdgeRef + 355.2, 337),
                   Coords(leftEdgeRef + 365.1, 340),
                   Coords(leftEdgeRef + 374.8, 343)});
    out.push_back({Coords(leftEdgeRef + 58.8, 460),
                   Coords(leftEdgeRef + 68.4, 463),
                   Coords(leftEdgeRef + 78.1, 465),
                   Coords(leftEdgeRef + 88.0, 467),
                   Coords(leftEdgeRef + 98.1, 468),
                   Coords(leftEdgeRef + 108.2, 467),
                   Coords(leftEdgeRef + 118.4, 465),
                   Coords(leftEdgeRef + 127.9, 462),
                   Coords(leftEdgeRef + 137.4, 458)});
    out.push_back({Coords(leftEdgeRef + 173.1, 458),
                   Coords(leftEdgeRef + 183.1, 456),
                   Coords(leftEdgeRef + 193.6, 455),
                   Coords(leftEdgeRef + 204.1, 454),
                   Coords(leftEdgeRef + 214.5, 453.5),
                   Coords(leftEdgeRef + 224.7, 454),
                   Coords(leftEdgeRef + 234.8, 455),
                   Coords(leftEdgeRef + 245.2, 456),
                   Coords(leftEdgeRef + 255.6, 458)});
    out.push_back({Coords(leftEdgeRef + 295.3, 462),
                   Coords(leftEdgeRef + 305.4, 459),
                   Coords(leftEdgeRef + 316.2, 457),
                   Coords(leftEdgeRef + 326.9, 456),
                   Coords(leftEdgeRef + 337.2, 457),
                   Coords(leftEdgeRef + 348.1, 459),
                   Coords(leftEdgeRef + 358.1, 462),
                   Coords(leftEdgeRef + 368.2, 465)});

    return out;
}

std::vector<Coords> MakeSpellBoxMatrix(double leftEdgeRef)
{
    std::vector<Coords> out;
    out.push_back(Coords(leftEdgeRef + 35, 84));

    out.push_back(Coords(leftEdgeRef + 46, 220));
    out.push_back(Coords(leftEdgeRef + 160, 219));
    out.push_back(Coords(leftEdgeRef + 279, 225));
    out.push_back(Coords(leftEdgeRef + 45, 360));
    out.push_back(Coords(leftEdgeRef + 159, 352));
    out.push_back(Coords(leftEdgeRef + 280, 354));
    out.push_back(Coords(leftEdgeRef + 43, 485));
    out.push_back(Coords(leftEdgeRef + 161, 480));
    out.push_back(Coords(leftEdgeRef + 278, 472));

    // 0_extra
    out.push_back(Coords(leftEdgeRef + 89, 63));

    return out;
}

}

ClassPage::ClassPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params)
    : PageBase(doc, config, side, params),
      classData(config.getObject("classes").getObject(params.at("classname"))),
      spellSlotMarker(MakeSpellSlotMarkers(LEFT_EDGE_REF)),
      spellBoxMatrix(MakeSpellBoxMatrix(LEFT_EDGE_REF))
{
}

void ClassPage::Draw()
{
    DecorateCorners();
    doc.AddImage("assets/spellgrid_lvled.png",
                 LEFT_EDGE_REF + 1,
                 MARGIN,
                 HALF_PAGE_WIDTH - 2,
                 A4_LANDSCAPE_HEIGHT - MARGIN);

    doc.AddImage("assets/ribbon_class.png", LEFT_EDGE_REF + 9, 9, 172, 56);
    doc.AddImage("assets/ribbon_subclass.png", LEFT_EDGE_REF + 238, 9, 172, 56);
}

void ClassPage::drawMarker(int level, int position)
{
    if (level < 0 || static_cast<size_t>(level) >= spellSlotMarker.size()) { return; }

    const auto& row = spellSlotMarker[level];
    if (position < 0 || static_cast<size_t>(position) >= row.size()) { return; }

    Coords marker = row[position];
    doc.DrawCircle(marker.x, marker.y, 4);
}

void ClassPage::Fill()
{
    AddClass();

    if (classData.isNull())
    {
        Utilities::LogError("Class data is null for class: " + pageParams.at("classname"));
        return;
    }

    AddSubclass();
    AddLevel();
    AddResourcePoints();
    AddCastInfo();
    AddSpellSlots();
    AddSpellStats();
    AddSpellNames();
}

void ClassPage::AddClass()
{
    Utilities::LogInfo(" - Adding class");
    const std::string& classId = pageParams.at("classname");
    const std::string displayName = Utilities::CapitalizeFirst(classId);

    const double spacing = Utilities::CalcSpacingForClassname(classId);

    if (displayName.size() > 7)
    {
        doc.AddTextCurved(displayName,
                          Coords(LEFT_EDGE_REF + 58, 15),
                          28,
                          UtilType::TextOptions(FontType::Seagram, 20, 0, spacing));
    }
    else if (displayName.size() < 5)
    {
        doc.AddText(
            displayName, LEFT_EDGE_REF + 58, 16, UtilType::TextOptions(FontType::Seagram, 20, 3, spacing));
    }
    else
    {
        doc.AddTextCurved(displayName,
                          Coords(LEFT_EDGE_REF + 58, 14),
                          14,
                          UtilType::TextOptions(FontType::Seagram, 20, 0, spacing));
    }
}

void ClassPage::AddSubclass()
{
    if (!classData.hasKey("subclass")) { return; }

    Utilities::LogInfo(" - Adding subclass");
    const std::string subclass = classData.getString("subclass");
    doc.AddTextCurved(
        subclass, Coords(LEFT_EDGE_REF + 302, 22), 14, UtilType::TextOptions(FontType::Seagram, 12));
}

void ClassPage::AddLevel()
{
    if (!classData.hasKey("level"))
    {
        Utilities::LogError("ERROR:Level data is missing!");
        return;
    }

    Utilities::LogInfo(" - Adding level");
    const int levelVal = classData.getInt("level");
    doc.AddText(
        std::to_string(levelVal), LEFT_EDGE_REF + 200, 20, UtilType::TextOptions(FontType::Seagram, 36));
}

void ClassPage::AddResourcePoints()
{
    if (!classData.hasKey("resourcePoints")) { return; }

    Utilities::LogInfo(" - Adding resource points");
    const int resourcePoints = classData.getInt("resourcePoints");
    doc.AddText(std::to_string(resourcePoints),
                LEFT_EDGE_REF + 202,
                130,
                UtilType::TextOptions(FontType::Seagram, 26));
}

void ClassPage::AddCastInfo()
{
    if (!classData.hasKey("castStat"))
    {
        Utilities::LogError("ERROR: Cast stat data is missing!");
        return;
    }

    const UtilType::TextOptions headOpts(FontType::Seagram, 16);
    const UtilType::TextOptions dataOpts(FontType::Seagram, 18);
    const UtilType::TextOptions labelOpts(FontType::Seagram, 10);
    const UtilType::TextOptions infoOpts(FontType::Arial, 6);

    doc.AddText("Spellcasting Ability", LEFT_EDGE_REF + 289, 88, labelOpts);
    doc.AddText("Spell Save DC:", LEFT_EDGE_REF + 280, 111, labelOpts);
    doc.AddText("Spell Attack Bonus:", LEFT_EDGE_REF + 276, 144, labelOpts);

    doc.AddText("8+ spellcasting ability mod+ prof.bonus", LEFT_EDGE_REF + 277, 124, infoOpts);
    doc.AddText("spellcasting ability mod+ prof.bonus", LEFT_EDGE_REF + 279, 158, infoOpts);

    const std::string castStatRaw = classData.getString("castStat");
    if (castStatRaw.empty()) { return; }

    Utilities::LogInfo(" - Adding cast info");

    const std::string castStat = Utilities::CapitalizeFirst(castStatRaw);
    const int castStatMod = CalcModFromStatName(castStatRaw);
    const std::string castStatModStr = castStat + " (" + Utilities::FormatSignedInt(castStatMod) + ")";
    doc.AddText(castStatModStr, LEFT_EDGE_REF + 274, 69, headOpts);

    const int spellSaveDc = 8 + castStatMod + config.getObject("proficiencies").getInt("bonus");
    doc.AddText(std::to_string(spellSaveDc), LEFT_EDGE_REF + 348, 103, dataOpts);

    const int spellAttackBonus = castStatMod + config.getObject("proficiencies").getInt("bonus");
    doc.AddText(std::to_string(spellAttackBonus), LEFT_EDGE_REF + 362, 136, dataOpts);
}

void ClassPage::AddSpellSlots()
{
    if (!classData.hasKey("spellslots")) { return; }

    Utilities::LogInfo(" - Adding spell slots");
    const auto spellSlots = classData.getObject("spellslots");
    for (int level = 1; level <= 9; ++level)
    {
        const std::string key = std::to_string(level);
        if (spellSlots.hasKey(key))
        {
            const int count = spellSlots.getInt(key);
            if (count > 0)
            {
                const int levelIndex = level - 1;
                for (int position = 0; position < count; ++position)
                {
                    drawMarker(levelIndex, position);
                }
            }
        }
    }
}

void ClassPage::AddSpellStats()
{
    if (!classData.hasKey("spells")) { return; }
    // Utilities::LogInfo(" - Adding spell stats");
    /// TODO: implement me
}

void ClassPage::AddSpellNames()
{
    if (!classData.hasKey("spells")) { return; }

    Utilities::LogInfo(" - Adding spell names");

    const auto spells = classData.getObject("spells");

    const std::vector<std::string> spell0Names = spells.getStringArray("0");
    std::vector<std::string> spell0NamesWithOffsets;
    for (size_t i = 0; i < spell0Names.size(); i++)
    {
        int offset = (i < 5) ? i : 5;

        std::string spellName = spell0Names[i];
        for (int j = 0; j < offset; j++)
        {
            spellName = std::string("\u00A0") + spellName;
        }
        if (spellName.size() > 30)
        {
            size_t breakPos = spellName.find_last_of(' ', 30);
            std::string spellnameTail = spellName.substr(breakPos + 1);
            for (int j = 0; j < offset; j++)
            {
                spellnameTail = std::string("\u00A0") + spellnameTail;
            }
            spellName = spellName.substr(0, breakPos);
            spell0NamesWithOffsets.push_back(spellName);
            spell0NamesWithOffsets.push_back(spellnameTail);
        }
        else
        {
            spell0NamesWithOffsets.push_back(spellName);
        }
    }
    TextBox::CreateStandard(*this, spellBoxMatrix[0].x, spellBoxMatrix[0].y, 7, {75.0})
        .RenderPlainTextLines(spell0NamesWithOffsets);

    for (int level = 1; level <= 9; ++level)
    {
        const std::string key = std::to_string(level);
        if (spells.hasKey(key))
        {
            const auto spellNames = spells.getStringArray(key);
            TextBox::CreateStandard(*this, spellBoxMatrix[level].x, spellBoxMatrix[level].y, 7, {100.0})
                .RenderPlainTextLines(spellNames);
        }
    }

    const std::vector<std::string> spell0_extra = spells.getStringArray("0_extra");
    TextBox::CreateStandard(
        *this, spellBoxMatrix[10].x, spellBoxMatrix[10].y, 7, {62.0, 71.0, 73.0, 72.0, 62.0})
        .RenderPlainTextLines(spell0_extra);
}
