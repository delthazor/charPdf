#pragma once

#include "pagetypes/PageBase.h"

class TextBox;

class DescriptionPage : public PageBase
{
  public:
    DescriptionPage(PdfDoc& doc,
                    const Config& config,
                    PageSide side,
                    const PageParams& params,
                    const std::string& descriptionParamKey);

    void Draw() override;
    void Fill() override;

  protected:
    virtual bool HasDescriptionContent() const;
    virtual void RenderDescriptionContent(TextBox& box) = 0;

    const std::string& descChunk;
    std::string title;
};
