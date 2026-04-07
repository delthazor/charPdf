#pragma once

#include "pagetypes/DescriptionPage.h"

class SpellPage : public DescriptionPage
{
  public:
    SpellPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params);

  protected:
    void RenderDescriptionContent(TextBox& box) override;

  private:
    struct SpellBlock
    {
        std::string name;
        std::string props;
        std::string concentration;
        std::string description;
        std::string upgrades;
    };

    SpellBlock parseSpellBlock(const std::string& block);
    UtilType::FormattedLabeledBlock CreateFormattedBlock(const SpellBlock& spellBlock);
};
