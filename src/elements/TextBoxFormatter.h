#pragma once

#include <cstddef>
#include <span>
#include <vector>

class TextBoxFormatter
{
  public:
    TextBoxFormatter(const std::vector<double>& lineWidths,
                     double startY,
                     double lineHeight,
                     double blockSpacing);

    double GetCurrentY() const;
    void AdvanceLines(size_t numLines);
    void AddBlockSpacing();
    std::span<const double> GetRemainingLineWidthsSpan() const;
    double GetLineWidthAt(size_t index) const;

  private:
    std::vector<double> lineWidths;
    size_t currentLineIndex;
    double currentY;
    const double lineHeight;
    const double blockSpacing;
};
