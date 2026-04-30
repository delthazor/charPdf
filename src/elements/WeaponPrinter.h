#pragma once

#include "syshelpers/Config.h"
#include "syshelpers/UtilTypes.h"

class WeaponPrinter
{
  public:
    WeaponPrinter(const Config& rawWeaponConfig,
                  int statModParam,
                  int proficiencyBonusParam,
                  bool isRangedParam);

    UtilType::FormattedLabeledBlock Render() const;

  private:
    void RenderName();
    void RenderRangeType();
    void RenderRawDamage();
    void RenderProps();
    void RenderProfLabel();
    void RenderTotals();

    std::string JoinProps() const;
    std::string BuildWeaponTotalString() const;
    std::string BuildDamageSumDisplay() const;

    const UtilType::WeaponConfig weaponCfg;
    const int statMod;
    const int proficiencyBonus;
    const bool isRanged;

    UtilType::FormattedLabeledBlock block;
};
