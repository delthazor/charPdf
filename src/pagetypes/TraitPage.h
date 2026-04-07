#pragma once

#include "pagetypes/DescriptionPage.h"

class TraitPage : public DescriptionPage
{
  public:
    TraitPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params);

  protected:
    void RenderDescriptionContent(TextBox& box) override;
};
