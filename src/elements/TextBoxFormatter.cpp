#include "elements/TextBoxFormatter.h"

TextBoxFormatter::TextBoxFormatter(const std::vector<double>& lineWidths,
                                   double startY,
                                   double lineHeight,
                                   double blockSpacing)
    : lineWidths(lineWidths), currentLineIndex(0), currentY(startY), lineHeight(lineHeight),
      blockSpacing(blockSpacing)
{
}

double TextBoxFormatter::GetCurrentY() const { return currentY; }

void TextBoxFormatter::AdvanceLines(size_t numLines)
{
    currentLineIndex += numLines;
    currentY += numLines * lineHeight;
}

void TextBoxFormatter::AddBlockSpacing() { currentY += blockSpacing; }

std::span<const double> TextBoxFormatter::GetRemainingLineWidthsSpan() const
{
    if (lineWidths.empty()) { return {}; }
    if (currentLineIndex >= lineWidths.size()) { return std::span<const double>(&lineWidths.back(), 1); }
    return std::span<const double>(lineWidths.data() + currentLineIndex,
                                   lineWidths.size() - currentLineIndex);
}

double TextBoxFormatter::GetLineWidthAt(size_t index) const
{
    if (lineWidths.empty()) { return 0.0; }

    if (index >= lineWidths.size()) { return lineWidths.back(); }

    return lineWidths[index];
}
