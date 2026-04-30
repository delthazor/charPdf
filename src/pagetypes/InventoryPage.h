#pragma once

#include "pagetypes/PageBase.h"

class InventoryPage : public PageBase
{
  public:
    InventoryPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params = {});

    void Draw() override;
    void Fill() override;

  private:
    void loadProficiencyBonusFromConfig();

    void appendWeaponBlocks(const std::vector<Config>& weapons,
                            std::vector<UtilType::FormattedLabeledBlock>& outBlocks);
    void appendArmorBlocks(const std::vector<Config>& armors,
                           std::vector<UtilType::FormattedLabeledBlock>& outBlocks);

    void buildAndRenderEquipmentBlocks();
    void buildAndRenderBackpack();

    int proficiencyBonus;
};
