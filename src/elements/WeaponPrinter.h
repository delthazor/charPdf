#pragma once

#include "syshelpers/Config.h"
#include "syshelpers/UtilTypes.h"

class WeaponPrinter
{
  public:
    WeaponPrinter(const Config& rawWeaponConfig,
                  int strengthModParam,
                  int dexterityModParam,
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
    void RenderExtraText();

    void AppendTotalLine(const std::string& extraBeforeColon,
                         int abilityMod,
                         const UtilType::DamageConfig& damage);
    void RenderVersatileTotals(const std::string& prefix, const int statModParam);

    std::string JoinProps() const;
    std::string BuildWeaponTotalString(int abilityMod, const UtilType::DamageConfig& damage) const;
    std::string BuildDamageSumDisplay(int abilityMod, const UtilType::DamageConfig& damage) const;
    std::string BuildExtraDamages() const;

    const UtilType::WeaponConfig weaponCfg;
    const int strengthMod;
    const int dexterityMod;
    const int proficiencyBonus;
    const bool isRanged;

    UtilType::FormattedLabeledBlock block;
};
