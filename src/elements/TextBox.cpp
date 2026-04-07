#include "elements/TextBox.h"
#include "syshelpers/Utilities.h"

TextBox::TextBox(PageBase& page,
                 double x,
                 double y,
                 const std::vector<double>& lineWidths,
                 PageConstants::FontType fontType,
                 double fontSize,
                 double blockSpacing)
    : page(page), x(x), fontType(fontType), fontSize(fontSize),
      formatter(lineWidths, y, Utilities::CalculateLineHeight(fontSize), blockSpacing)
{
}

TextBox TextBox::CreateStandard(
    PageBase& page, double x, double y, double fontSize, std::initializer_list<double> lineWidths)
{
    return TextBox(page,
                   x,
                   y,
                   std::vector<double>(lineWidths),
                   PageConstants::FontType::Arial,
                   fontSize,
                   PageConstants::TEXTBOX_BLOCK_SPACING);
}

void TextBox::RenderBlocks(const std::vector<UtilType::LabeledTextBlock>& blocks)
{
    for (size_t i = 0; i < blocks.size(); i++)
    {
        RenderBlock(blocks[i]);

        if (i < blocks.size() - 1) { formatter.AddBlockSpacing(); }
    }
}

void TextBox::RenderBlock(const UtilType::LabeledTextBlock& block)
{
    const std::string formattedLabel = block.label + ":";

    const size_t linesUsed = RenderLabeledText(formattedLabel, block.text);

    formatter.AdvanceLines(linesUsed);
}

void TextBox::RenderFormattedBlocks(const std::vector<UtilType::FormattedLabeledBlock>& blocks)
{
    for (size_t i = 0; i < blocks.size(); i++)
    {
        formatter.AdvanceLines(RenderFormattedLabeledText(blocks[i]));
        if (i < blocks.size() - 1) { formatter.AddBlockSpacing(); }
    }
}

size_t TextBox::RenderFormattedLabeledText(const UtilType::FormattedLabeledBlock& block)
{
    const double lineHeight = Utilities::CalculateLineHeight(fontSize);
    const std::span<const double> remainingWidths = formatter.GetRemainingLineWidthsSpan();
    const double y = formatter.GetCurrentY();

    const double labelWidth = page.DrawFormattedLabel(block.labelParts, x, y, fontSize);

    return page.AddWrappedFormattedText(
        x, y, block.textSpans, remainingWidths, lineHeight, fontSize, labelWidth);
}

size_t TextBox::RenderLabeledText(const std::string& label, const std::string& text)
{
    PageConstants::FontType boldFont;
    switch (fontType)
    {
    case PageConstants::FontType::Arial:
    case PageConstants::FontType::ArialItalic:
        boldFont = PageConstants::FontType::ArialBold;
        break;
    case PageConstants::FontType::TimesNewRoman:
    case PageConstants::FontType::TimesNewRomanItalic:
        boldFont = PageConstants::FontType::TimesNewRomanBold;
        break;
    default:
        boldFont = fontType;
        break;
    }

    const UtilType::TextOptions labelOpts(boldFont, fontSize);
    const UtilType::TextOptions valueOpts(fontType, fontSize);

    const double lineHeight = Utilities::CalculateLineHeight(fontSize);

    const std::span<const double> remainingWidths = formatter.GetRemainingLineWidthsSpan();

    const size_t linesUsed = page.AddWrappedTextWithLabel(
        label, text, x, formatter.GetCurrentY(), remainingWidths, lineHeight, labelOpts, valueOpts);

    return linesUsed;
}

void TextBox::RenderPlainText(const std::string& text)
{
    if (text.empty()) { return; }

    const double lineHeight = Utilities::CalculateLineHeight(fontSize);
    const UtilType::TextOptions opts(fontType, fontSize);
    const size_t linesUsed = page.AddWrappedPlainText(
        text, x, formatter.GetCurrentY(), formatter.GetRemainingLineWidthsSpan(), lineHeight, opts);
    formatter.AdvanceLines(linesUsed);
}

void TextBox::RenderPlainTextLines(const std::vector<std::string>& lines)
{
    const double lineHeight = Utilities::CalculateLineHeight(fontSize);
    const UtilType::TextOptions opts(fontType, fontSize);

    for (const std::string& line : lines)
    {
        if (line.empty())
        {
            formatter.AdvanceLines(1);
            continue;
        }

        const size_t linesUsed = page.AddWrappedPlainText(
            line, x, formatter.GetCurrentY(), formatter.GetRemainingLineWidthsSpan(), lineHeight, opts);
        formatter.AdvanceLines(linesUsed);
    }
}
