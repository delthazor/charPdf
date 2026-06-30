#include "syshelpers/AcCalculation.h"

#include <algorithm>
#include <string>

#include "syshelpers/Utilities.h"

namespace AcCalculation
{

namespace
{

int StatModFromConfig(const Config& stats, const std::string& statName)
{
    if (!stats.hasKey(statName)) { return 0; }
    return Utilities::CalcModFromStatVal(stats.getInt(statName));
}

bool IsBaseTypeArmor(const Config& armor)
{
    if (!armor.hasKey("ac")) { return false; }
    const Config ac = armor.getObject("ac");
    return ac.hasKey("base");
}

bool IsFixmodTypeArmor(const Config& armor)
{
    if (!armor.hasKey("ac")) { return false; }
    const Config ac = armor.getObject("ac");
    return ac.hasKey("fixmod");
}

int SumFixmodBonuses(const std::vector<Config>& armors)
{
    int total = 0;
    for (const Config& armor : armors)
    {
        if (!IsFixmodTypeArmor(armor)) { continue; }
        total += armor.getObject("ac").getInt("fixmod");
    }
    return total;
}

}

int ArmorStatContribution(const Config& armorAc, int statMod)
{
    if (!armorAc.hasKey("modcap")) { return statMod; }
    return std::min(statMod, armorAc.getInt("modcap"));
}

int CalcAc(const Config& characterConfig)
{
    const Config stats = characterConfig.hasKey("stats") ? characterConfig.getObject("stats") : Config();
    const int acBonus = stats.hasKey("acBonus") ? stats.getInt("acBonus") : 0;

    std::vector<Config> usedArmors;
    if (characterConfig.hasKey("equipment"))
    {
        const Config equipment = characterConfig.getObject("equipment");
        if (equipment.hasKey("used"))
        {
            const Config used = equipment.getObject("used");
            if (used.hasKey("armors")) { usedArmors = used.getObjectArray("armors"); }
        }
    }

    std::vector<Config> baseTypeArmors;
    for (const Config& armor : usedArmors)
    {
        if (IsBaseTypeArmor(armor)) { baseTypeArmors.push_back(armor); }
    }

    int bodyAc = 0;
    if (baseTypeArmors.empty())
    {
        bodyAc = 10 + StatModFromConfig(stats, "dexterity") + acBonus;
    }
    else
    {
        if (baseTypeArmors.size() > 1)
        {
            std::string charName = "character";
            if (characterConfig.hasKey("name")) { charName = characterConfig.getString("name"); }
            Utilities::LogError("Warning: multiple base-type armors for " + charName + ", using last");
        }

        const Config& armor = baseTypeArmors.back();
        const Config ac = armor.getObject("ac");
        const int base = ac.getInt("base");
        const std::string modstat = ac.getString("modstat");
        const int statMod = StatModFromConfig(stats, modstat);
        bodyAc = base + ArmorStatContribution(ac, statMod) + acBonus;
    }

    return bodyAc + SumFixmodBonuses(usedArmors);
}

}
