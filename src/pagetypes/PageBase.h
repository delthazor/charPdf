#pragma once

#include <map>
#include <span>
#include <string>
#include <vector>

#include "syshelpers/Config.h"
#include "syshelpers/PageConstants.h"
#include "syshelpers/PdfDoc.h"
#include "syshelpers/UtilTypes.h"

using PageConstants::PageSide;

using PageParams = std::map<std::string, std::string>;

class PageBase
{
  public:
    PageBase(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params = {});
    virtual ~PageBase() = default;

    virtual void Draw() = 0;
    virtual void Fill() = 0;

    size_t AddWrappedTextWithLabel(const std::string& label,
                                   const std::string& value,
                                   double startX,
                                   double startY,
                                   std::span<const double> lineWidths,
                                   double lineHeight,
                                   const UtilType::TextOptions& labelOpts,
                                   const UtilType::TextOptions& valueOpts);

    size_t AddWrappedPlainText(const std::string& text,
                               double startX,
                               double startY,
                               std::span<const double> lineWidths,
                               double lineHeight,
                               const UtilType::TextOptions& textOpts,
                               bool indentWrappedContinuationLines = false);

    size_t AddWrappedFormattedText(double startX,
                                   double startY,
                                   const std::vector<UtilType::FormattedTextSpan>& spans,
                                   std::span<const double> lineWidths,
                                   double lineHeight,
                                   double fontSize,
                                   double labelWidth);

    double DrawFormattedLabel(const std::vector<UtilType::FormattedLabelPart>& parts,
                              double startX,
                              double startY,
                              double fontSize);

  protected:
    PdfDoc& doc;
    const Config& config;
    const PageSide side;
    const double LEFT_EDGE_REF;
    const PageParams pageParams;

    void DecorateCorners();
    int CalcModFromStatName(const std::string& statName) const;

    double CalculateTextWidth(const std::string& text, const UtilType::TextOptions& opts) const;

  private:
    void PrintStyledWordsLine(const UtilType::MeasureFn& measure,
                              const std::vector<UtilType::StyledWord>& lineWords,
                              double lineStartX,
                              double currentY,
                              double fontSize);
};
