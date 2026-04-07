#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "elements/TextBoxFormatter.h"
#include "pagetypes/PageBase.h"
#include "syshelpers/PageConstants.h"
#include "syshelpers/UtilTypes.h"

class TextBox
{
  public:
    TextBox(PageBase& page,
            double x,
            double y,
            const std::vector<double>& lineWidths,
            PageConstants::FontType fontType,
            double fontSize,
            double blockSpacing = PageConstants::TEXTBOX_BLOCK_SPACING);

    static TextBox CreateStandard(
        PageBase& page, double x, double y, double fontSize, std::initializer_list<double> lineWidths);

    void RenderBlocks(const std::vector<UtilType::LabeledTextBlock>& blocks);
    void RenderBlock(const UtilType::LabeledTextBlock& block);

    void RenderFormattedBlocks(const std::vector<UtilType::FormattedLabeledBlock>& blocks);

    void RenderPlainText(const std::string& text);
    void RenderPlainTextLines(const std::vector<std::string>& lines);

  private:
    size_t RenderLabeledText(const std::string& label, const std::string& text);
    size_t RenderFormattedLabeledText(const UtilType::FormattedLabeledBlock& block);

    PageBase& page;
    double x;
    PageConstants::FontType fontType;
    double fontSize;
    TextBoxFormatter formatter;
};
