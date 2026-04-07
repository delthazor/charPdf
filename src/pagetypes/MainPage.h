#pragma once

#include <map>
#include <string>

#include "pagetypes/PageBase.h"

class MainPage : public PageBase
{
  public:
    using PageBase::PageBase;

    void Draw() override;
    void Fill() override;

  private:
    void DrawLayer1();
    void DrawLayer2();

    void AddName();
    void AddRace();
    void AddBackground();
    void AddStats();
    void AddStat(const std::string& statName, unsigned int statIndex);
    void AddSpeed();
    void AddAc();
    void AddInitiative();
    void AddPassivePerception();
    void AddMaxHp();

    void AddProficienciesBox();
    void AddProficiencyBonus();
    void AddHitDice();
    void AddSkillProfs();
    void AddSavingThrows();

    std::map<std::string, int> GetHitDiceMap() const;
    std::pair<std::string, int> FindSkill(const std::string& skillKey) const;
    void RenderSkillProf(const std::string& statName, int skillValue, int profBonus, double yPos);
    void RenderSavingThrow(const std::string& statName, int profValue, int profBonus, double yPos);
    std::string GetProficiencyString(const std::string& arrayKey) const;
};
