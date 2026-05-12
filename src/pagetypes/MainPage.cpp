#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "elements/TextBox.h"
#include "pagetypes/MainPage.h"
#include "syshelpers/Utilities.h"

using namespace PageConstants;

void MainPage::Draw()
{
    DecorateCorners();
    DrawLayer1();
    DrawLayer2();
}

void MainPage::Fill()
{
    Utilities::LogInfo(" - Add general info");
    AddName();
    AddRace();
    AddBackground();
    AddStats();
    AddHitDice();

    Utilities::LogInfo(" - Add proficiency boxes");
    AddProficienciesBox();
    AddProficiencyBonus();
    AddSkillProfs();
    AddSavingThrows();
}

void MainPage::DrawLayer1()
{
    Utilities::LogInfo(" - Draw layer 1");
    doc.AddImage("assets/label_race.png", MIDDLE_LINE / 2 - 72, 78, 147, 16);
    doc.AddImage("assets/label_background.png", MIDDLE_LINE / 2 - 84, 108, 161, 16);

    doc.AddImage("assets/statvine_left.png", MARGIN, 16, 164, 168, 364);
    doc.AddImage("assets/statvine_right.png", MIDDLE_LINE - 170, 9, 167, 148, 349);

    doc.AddImage("assets/hp_col.png", MIDDLE_LINE / 2 - 51, 176, 128, 208);
}

void MainPage::DrawLayer2()
{
    Utilities::LogInfo(" - Draw layer 2");
    doc.AddImage("assets/nametag.png", HALF_PAGE_WIDTH / 2 - 95, 8, 200, 70);

    doc.AddImage("assets/statorb_str.png", MIDDLE_LINE / 2 - 208, 191, 86, 77, 15);
    doc.AddImage("assets/statorb_dex.png", MIDDLE_LINE / 2 - 154, 147, 85, 75, 24);
    doc.AddImage("assets/statorb_con.png", MIDDLE_LINE / 2 - 83, 120, 93, 75, 8);

    doc.AddImage("assets/statorb_int.png", MIDDLE_LINE / 2 + 8, 122, 91, 75, 0);
    doc.AddImage("assets/statorb_wis.png", MIDDLE_LINE / 2 + 69, 162, 87, 72, 352);
    doc.AddImage("assets/statorb_cha.png", MIDDLE_LINE / 2 + 123, 196, 89, 77, 356);

    doc.AddImage("assets/orb_gold.png", MIDDLE_LINE / 2 - 27, 367, 64, 84);
    doc.AddImage("assets/orb_silver.png", MIDDLE_LINE / 2 - 23, 441, 63, 89);
    doc.AddImage("assets/orb_copper.png", MIDDLE_LINE / 2 - 27, 516, 66, 84);

    doc.AddImage("assets/Insp_bubble.png", MIDDLE_LINE / 2 - 98, 209, 58, 51);

    doc.AddImage("assets/profi_box.png", MARGIN, 244, 177, 344);
    doc.AddImage("assets/box-profi.png", MIDDLE_LINE - 172, 252, 183, 338);
}

void MainPage::AddName()
{
    doc.AddText(config.getString("name"),
                MIDDLE_LINE / 2 - 72,
                14,
                UtilType::TextOptions(FontType::Seagram, 34, 0, config.getDouble("nameLetterSpacing")));
}

void MainPage::AddRace()
{
    doc.AddText(
        config.getString("race"), MIDDLE_LINE / 2 - 7, 73, UtilType::TextOptions(FontType::Seagram, 12));
}

void MainPage::AddBackground()
{
    doc.AddText(config.getString("background"),
                MIDDLE_LINE / 2 - 7,
                103,
                UtilType::TextOptions(FontType::Seagram, 12));
}

void MainPage::AddStats()
{
    Utilities::LogInfo(" - Add base stats");
    AddStat("strength", 0);
    AddStat("dexterity", 1);
    AddStat("constitution", 2);
    AddStat("intelligence", 3);
    AddStat("wisdom", 4);
    AddStat("charisma", 5);

    Utilities::LogInfo(" - Add derived stats");
    AddAc();
    AddSpeed();
    AddInitiative();
    AddPassivePerception();
    AddMaxHp();
}

void MainPage::AddStat(const std::string& statName, unsigned int statIndex)
{
    const int statVal = config.getObject("stats").getInt(statName);
    const int statMod = Utilities::CalcModFromStatVal(statVal);
    const std::string statModStr = Utilities::FormatSignedInt(statMod);

    doc.AddText(statModStr,
                STAT_MOD_COORDS[statIndex].x,
                STAT_MOD_COORDS[statIndex].y,
                UtilType::TextOptions(FontType::Seagram, 18));
    doc.AddText(std::to_string(statVal),
                STAT_VAL_COORDS[statIndex].x,
                STAT_VAL_COORDS[statIndex].y,
                UtilType::TextOptions(FontType::Seagram, 14));
}

void MainPage::AddAc()
{
    doc.AddText(std::to_string(config.getObject("stats").getInt("ac")),
                MIDDLE_LINE - 53,
                101,
                UtilType::TextOptions(FontType::Seagram, 18));
}

void MainPage::AddSpeed()
{
    doc.AddText(std::to_string(config.getObject("stats").getInt("speed")),
                53,
                55,
                UtilType::TextOptions(FontType::Seagram, 18));
}

void MainPage::AddInitiative()
{
    const int initiativeBonus = config.getObject("stats").getInt("initiativeBonus");
    const int initiative =
        initiativeBonus + Utilities::CalcModFromStatVal(config.getObject("stats").getInt("dexterity"));

    doc.AddText(Utilities::FormatSignedInt(initiative),
                MIDDLE_LINE - 78,
                51,
                UtilType::TextOptions(FontType::Seagram, 18));
}

void MainPage::AddPassivePerception()
{
    const Config stats = config.getObject("stats");
    const int pPercBonus = stats.hasKey("pPercBonus") ? stats.getInt("pPercBonus") : 0;
    const int passivePerception = 10 + CalcModFromStatName("wisdom") + pPercBonus;
    doc.AddText(std::to_string(passivePerception), 30, 111, UtilType::TextOptions(FontType::Seagram, 18));
}

void MainPage::AddMaxHp()
{
    const int maxHp = config.getObject("stats").getInt("maxHp");
    doc.AddText(
        std::to_string(maxHp), MIDDLE_LINE / 2 - 1, 271, UtilType::TextOptions(FontType::Seagram, 15));
}

void MainPage::AddProficiencyBonus()
{
    const int bonus = config.getObject("proficiencies").getInt("bonus");
    const std::string bonusStr = Utilities::FormatSignedInt(bonus);

    doc.AddText(bonusStr, MARGIN + 86, 255, UtilType::TextOptions(FontType::Seagram, 15, 34, 0));
}

void MainPage::AddHitDice()
{
    static const double BASE_Y = 196;
    static const double CENTER_X = MIDDLE_LINE / 2 + 6;
    static const double CURVE_FACTOR = 0.11;
    static const UtilType::TextOptions textOpts(FontType::Seagram, 12);
    static const double separatorWidth = doc.CalculateTextWidth(", ", textOpts);
    static const size_t maxHitDiceDisplayCount = 3;

    const auto hitDiceMap = GetHitDiceMap();
    const auto lastIt = std::prev(hitDiceMap.end());

    if (hitDiceMap.empty())
    {
        Utilities::LogError("Warning: no classes with hit dice found in config");
        return;
    }

    std::string hitDiceStr;
    for (auto it = hitDiceMap.begin(); it != hitDiceMap.end(); ++it)
    {
        hitDiceStr += std::to_string(it->second) + it->first;
        if (it != lastIt) { hitDiceStr += ", "; }
    }

    const double textWidth = doc.CalculateTextWidth(hitDiceStr, textOpts);
    const double xPos = CENTER_X - (textWidth / 2);

    if (hitDiceMap.size() == maxHitDiceDisplayCount)
    {
        double currentX = xPos;
        const auto diceStrComponents = Utilities::Split(hitDiceStr, ", ");
        for (size_t i = 0; i < diceStrComponents.size(); ++i)
        {
            const std::string diceStr = diceStrComponents[i];
            const double componentWidth = doc.CalculateTextWidth(diceStr, textOpts);
            const double componentCenterX = currentX + (componentWidth / 2);
            const double distanceOfCurrent = std::abs(componentCenterX - CENTER_X);
            const bool isLast = (i == diceStrComponents.size() - 1);
            const bool isFirstOrLast = (i == 0 || isLast);
            const double yAdjust = 1 + (isFirstOrLast ? distanceOfCurrent * CURVE_FACTOR : 0);
            const double yPos = BASE_Y + yAdjust;
            const double rotationAngle = !isFirstOrLast ? 0 : (isLast ? 350 : 15);

            UtilType::TextOptions textOptsWithRotation(
                textOpts.fontType, textOpts.fontSize, rotationAngle, textOpts.letterSpacing);
            doc.AddText(diceStr, currentX, yPos, textOptsWithRotation);

            currentX += componentWidth;
            if (!isLast) { currentX += separatorWidth; }
        }
    }
    else
    {
        const double distance = std::abs(xPos - CENTER_X);
        const double yAdjust = distance * CURVE_FACTOR;
        const double yPos = BASE_Y + yAdjust;
        doc.AddText(hitDiceStr, xPos, yPos, textOpts);
    }
}

std::pair<std::string, int> MainPage::FindSkill(const std::string& skillKey) const
{
    const auto skills = config.getObject("proficiencies").getObject("skills");
    const std::vector<std::string> stats = {
        "strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"};

    for (const auto& statName : stats)
    {
        if (skills.hasKey(statName))
        {
            const auto statSkills = skills.getObject(statName);
            if (statSkills.hasKey(skillKey)) { return {statName, statSkills.getInt(skillKey)}; }
        }
    }

    return {"", 0};
}

void MainPage::RenderSkillProf(const std::string& statName, int skillValue, int profBonus, double yPos)
{
    static const double START_X = 57;

    const int statMod = CalcModFromStatName(statName);
    const int totalBonus = statMod + (profBonus * skillValue);
    const std::string bonusStr = Utilities::FormatSignedInt(totalBonus);

    doc.AddText(bonusStr, START_X + 6, yPos, UtilType::TextOptions(FontType::Seagram, 10));

    if (skillValue != 0)
    {
        doc.DrawFilledCircle(START_X, yPos + 8.5, 2);

        if (skillValue > 1) { doc.DrawCircle(START_X, yPos + 8.6, 3.5); }
    }
}

void MainPage::AddSkillProfs()
{
    static const double START_Y = 302;
    static const double Y_SPACING = 10.68;
    static const std::vector<std::string> SKILLS = {"acrobatics",
                                                    "animalHandling",
                                                    "arcana",
                                                    "athletics",
                                                    "deception",
                                                    "history",
                                                    "insight",
                                                    "intimidation",
                                                    "investigation",
                                                    "medicine",
                                                    "nature",
                                                    "perception",
                                                    "performance",
                                                    "persuasion",
                                                    "religion",
                                                    "sleightOfHand",
                                                    "stealth",
                                                    "survival"};

    const int profBonus = config.getObject("proficiencies").getInt("bonus");

    for (size_t i = 0; i < SKILLS.size(); ++i)
    {
        const auto [statName, skillValue] = FindSkill(SKILLS[i]);

        if (!statName.empty())
        {
            const double yPos = START_Y + (i * Y_SPACING);
            RenderSkillProf(statName, skillValue, profBonus, yPos);
        }
        else
        {
            Utilities::LogError(std::string("Warning: skill '") + SKILLS[i] + "' not found in config");
        }
    }
}

void MainPage::RenderSavingThrow(const std::string& statName, int profValue, int profBonus, double yPos)
{
    static const double START_X = 57.5;

    const int statMod = CalcModFromStatName(statName);
    const int totalBonus = statMod + (profBonus * profValue);
    const std::string bonusStr = Utilities::FormatSignedInt(totalBonus);

    doc.AddText(bonusStr, START_X + 6, yPos, UtilType::TextOptions(FontType::Seagram, 10));

    if (profValue != 0) { doc.DrawFilledCircle(START_X, yPos + 8.8, 2); }
}

void MainPage::AddSavingThrows()
{
    static const double START_Y = 511;
    static const double Y_SPACING = 10.6;
    static const std::vector<std::string> STATS = {
        "strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"};

    const int profBonus = config.getObject("proficiencies").getInt("bonus");
    const auto savingThrows = config.getObject("proficiencies").getObject("savingThrows");

    for (size_t i = 0; i < STATS.size(); ++i)
    {
        if (savingThrows.hasKey(STATS[i]))
        {
            const int profValue = savingThrows.getInt(STATS[i]);
            const double yPos = START_Y + (i * Y_SPACING);
            RenderSavingThrow(STATS[i], profValue, profBonus, yPos);
        }
        else
        {
            Utilities::LogError(std::string("Warning: saving throw '") + STATS[i] + "' not found in config");
        }
    }
}

std::map<std::string, int> MainPage::GetHitDiceMap() const
{
    std::map<std::string, int> hitDiceMap;

    if (!config.hasKey("classes")) { return hitDiceMap; }

    const auto classes = config.getObject("classes");

    for (const auto& className : classes.getKeys())
    {
        const auto classData = classes.getObject(className);
        if (classData.hasKey("hitDice") && classData.hasKey("level"))
        {
            const std::string hitDice = classData.getString("hitDice");
            const int level = classData.getInt("level");
            hitDiceMap[hitDice] += level;
        }
    }

    return hitDiceMap;
}

std::string MainPage::GetProficiencyString(const std::string& arrayKey) const
{
    constexpr const char* emptyPlaceholder = " - ";
    const auto proficiencies = config.getObject("proficiencies");
    if (!proficiencies.hasKey(arrayKey))
    {
        Utilities::LogError(std::string("Warning: proficiency key '") + arrayKey + "' not found in config");
        return emptyPlaceholder;
    }

    const auto& items = proficiencies.getStringArray(arrayKey);
    if (items.empty()) { return emptyPlaceholder; }

    std::string result;
    for (const auto& item : items)
    {
        result += item + ", ";
    }
    if (!result.empty()) { result = result.substr(0, result.size() - 2); }
    return result.empty() ? emptyPlaceholder : result;
}

void MainPage::AddProficienciesBox()
{
    TextBox box = TextBox::CreateStandard(
        *this, MIDDLE_LINE - 160, 292, MAIN_PAGE_PROFICIENCIES_FONT_SIZE, {74, 88, 101, 106, 106, 129, 132});

    const std::vector<UtilType::LabeledTextBlock> blocks = {
        UtilType::LabeledTextBlock("Languages", GetProficiencyString("languages")),
        UtilType::LabeledTextBlock("Tools", GetProficiencyString("tools")),
        UtilType::LabeledTextBlock("Armors", GetProficiencyString("armors")),
        UtilType::LabeledTextBlock("Simple Weapons", GetProficiencyString("simpleWeapons")),
        UtilType::LabeledTextBlock("Martial Weapons", GetProficiencyString("martialWeapons")),
    };

    box.RenderBlocks(blocks);
}
