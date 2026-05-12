#include <algorithm>

#include "pagetypes/PageBase.h"
#include "syshelpers/Utilities.h"

using namespace PageConstants;

PageBase::PageBase(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params)
    : doc(doc),
      config(config),
      side(side),
      LEFT_EDGE_REF((side == PageSide::LEFT_SIDE) ? 0 : MIDDLE_LINE),
      pageParams(params)
{
}

void PageBase::PrintStyledWordsLine(const UtilType::MeasureFn& measure,
                                    const std::vector<UtilType::StyledWord>& lineWords,
                                    double lineStartX,
                                    double currentY,
                                    double fontSize)
{
    const size_t n = lineWords.size();
    std::vector<double> edge(n + 1);
    std::vector<double> runW(n);
    edge[0] = lineStartX;
    for (size_t i = 0; i < n; ++i)
    {
        const UtilType::StyledWord& w = lineWords[i];
        const std::string run =
            ((i > 0 && !lineWords[i].noSpaceBefore) ? " " : "") + w.text;
        runW[i] = measure(run, w.textOptions);
        edge[i + 1] = edge[i] + runW[i];
    }

    auto sameUnderlineStyle = [](const UtilType::StyledWord& a, const UtilType::StyledWord& b)
    {
        return a.textOptions.fontType == b.textOptions.fontType
               && a.textOptions.fontSize == b.textOptions.fontSize;
    };

    for (size_t i = 0; i < n;)
    {
        if (!lineWords[i].underline)
        {
            ++i;
            continue;
        }
        size_t j = i;
        while (j + 1 < n && lineWords[j + 1].underline && sameUnderlineStyle(lineWords[i], lineWords[j + 1]))
        {
            ++j;
        }
        doc.DrawTextUnderline(
            edge[i], currentY, edge[j + 1] - edge[i], lineWords[i].textOptions.fontType, fontSize);
        i = j + 1;
    }

    for (size_t i = 0; i < n; ++i)
    {
        const UtilType::StyledWord& w = lineWords[i];
        const std::string run =
            ((i > 0 && !lineWords[i].noSpaceBefore) ? " " : "") + w.text;
        doc.AddText(run, edge[i], currentY, w.textOptions);
    }
}

void PageBase::DecorateCorners()
{
    doc.AddImage("assets/corner_UpperLeft.png", LEFT_EDGE_REF + MARGIN, MARGIN, CORNER_WIDTH, CORNER_HEIGHT);
    doc.AddImage("assets/corner_UpperRight.png",
                 LEFT_EDGE_REF + HALF_PAGE_WIDTH + MARGIN - CORNER_WIDTH,
                 MARGIN,
                 CORNER_WIDTH,
                 CORNER_HEIGHT);
    doc.AddImage("assets/corner_BottomLeft.png",
                 LEFT_EDGE_REF + MARGIN,
                 A4_LANDSCAPE_HEIGHT - MARGIN - CORNER_HEIGHT,
                 CORNER_WIDTH,
                 CORNER_HEIGHT);

    const double rightOffset = (side == PageSide::LEFT_SIDE) ? -CORNER_SPACING : CORNER_SPACING;
    doc.AddImage("assets/corner_BottomRight.png",
                 LEFT_EDGE_REF + HALF_PAGE_WIDTH + MARGIN + rightOffset - CORNER_WIDTH,
                 A4_LANDSCAPE_HEIGHT - MARGIN - CORNER_HEIGHT,
                 CORNER_WIDTH,
                 CORNER_HEIGHT);
}

int PageBase::CalcModFromStatName(const std::string& statName) const
{
    const int statVal = config.getObject("stats").getInt(statName);
    return Utilities::CalcModFromStatVal(statVal);
}

double PageBase::CalculateTextWidth(const std::string& text, const UtilType::TextOptions& opts) const
{
    return doc.CalculateTextWidth(text, opts);
}

size_t PageBase::AddWrappedTextWithLabel(const std::string& label,
                                         const std::string& value,
                                         double startX,
                                         double startY,
                                         std::span<const double> lineWidths,
                                         double lineHeight,
                                         const UtilType::TextOptions& labelOpts,
                                         const UtilType::TextOptions& valueOpts)
{
    if (lineWidths.empty() || lineHeight <= 0) { return 0; }

    doc.AddText(label, startX, startY, labelOpts);

    const double labelWidth = CalculateTextWidth(label, labelOpts);
    const double firstLineX = startX + labelWidth + 1;
    const double firstLineMaxWidth = std::max(0.0, lineWidths[0] - labelWidth - 1.0);

    const std::vector<std::string> words = Utilities::SplitIntoWords(value);
    double currentY = startY;
    size_t wordIndex = 0;
    size_t totalLines = 0;

    const UtilType::MeasureFn measure = [this](const std::string& t, const UtilType::TextOptions& o)
    { return CalculateTextWidth(t, o); };

    while (wordIndex < words.size())
    {
        Utilities::EnsureLineLimit(totalLines);
        const bool isFirstLine = (totalLines == 0);
        const double lineX = isFirstLine ? firstLineX : startX;
        const double maxWidth =
            isFirstLine ? firstLineMaxWidth : Utilities::RemainingLineWidth(lineWidths, totalLines);

        const std::string line = Utilities::CollectPlainLine(words, wordIndex, maxWidth, valueOpts, measure);
        if (line.empty()) { break; }
        doc.AddText(line, lineX, currentY, valueOpts);
        currentY += lineHeight;
        totalLines++;
    }

    return totalLines;
}

size_t PageBase::AddWrappedPlainText(const std::string& text,
                                     double startX,
                                     double startY,
                                     std::span<const double> lineWidths,
                                     double lineHeight,
                                     const UtilType::TextOptions& textOpts,
                                     bool indentWrappedContinuationLines)
{
    if (lineWidths.empty() || lineHeight <= 0) { return 0; }

    const std::vector<std::string> words = Utilities::SplitIntoWords(text);
    double currentY = startY;
    size_t wordIndex = 0;
    size_t totalLines = 0;

    const UtilType::MeasureFn measure = [this](const std::string& t, const UtilType::TextOptions& o)
    { return CalculateTextWidth(t, o); };

    const double oneSpaceW =
        indentWrappedContinuationLines ? measure(" ", textOpts) : 0.0;

    while (wordIndex < words.size())
    {
        Utilities::EnsureLineLimit(totalLines);
        const bool isContinuation = indentWrappedContinuationLines && totalLines > 0;
        const double fullWidth = Utilities::RemainingLineWidth(lineWidths, totalLines);
        const double maxWidth = isContinuation ? std::max(0.0, fullWidth - oneSpaceW) : fullWidth;
        const double lineX = isContinuation ? startX + oneSpaceW : startX;
        const std::string line = Utilities::CollectPlainLine(words, wordIndex, maxWidth, textOpts, measure);
        if (line.empty()) { break; }
        doc.AddText(line, lineX, currentY, textOpts);
        currentY += lineHeight;
        totalLines++;
    }

    return totalLines;
}

size_t PageBase::AddWrappedFormattedText(double startX,
                                         double startY,
                                         const std::vector<UtilType::FormattedTextSpan>& spans,
                                         std::span<const double> lineWidths,
                                         double lineHeight,
                                         double fontSize,
                                         double labelWidth)
{
    if (lineWidths.empty() || lineHeight <= 0) { return 0; }

    std::vector<UtilType::StyledWord> words;
    for (const auto& span : spans)
    {
        UtilType::TextOptions opts(span.textOptions.fontType,
                                   fontSize,
                                   span.textOptions.rotationAngle,
                                   span.textOptions.letterSpacing);
        bool firstWord = true;
        for (const auto& word : Utilities::SplitIntoWords(span.text))
        {
            words.push_back({word,
                             opts,
                             span.underline,
                             firstWord && span.forceLineBreakBefore,
                             firstWord && span.joinToPrevious});
            firstWord = false;
        }
    }

    const double firstLineX = startX + labelWidth + 1;
    const double firstLineMaxWidth = std::max(0.0, lineWidths[0] - labelWidth - 1.0);

    double currentY = startY;
    size_t wordIndex = 0;
    size_t totalLines = 0;

    const UtilType::MeasureFn measure = [this](const std::string& t, const UtilType::TextOptions& o)
    { return CalculateTextWidth(t, o); };

    while (wordIndex < words.size())
    {
        Utilities::EnsureLineLimit(totalLines);
        const bool isFirstLine = (totalLines == 0);
        const double lineStartX = isFirstLine ? firstLineX : startX;
        const double maxWidth =
            isFirstLine ? firstLineMaxWidth : Utilities::RemainingLineWidth(lineWidths, totalLines);

        const std::vector<UtilType::StyledWord> lineWords =
            Utilities::CollectStyledLine(words, wordIndex, maxWidth, measure);
        if (lineWords.empty()) { break; }
        PrintStyledWordsLine(measure, lineWords, lineStartX, currentY, fontSize);

        currentY += lineHeight;
        totalLines++;
    }

    return totalLines;
}

double PageBase::DrawFormattedLabel(const std::vector<UtilType::FormattedLabelPart>& parts,
                                    double startX,
                                    double startY,
                                    double fontSize)
{
    double x = startX;
    double totalWidth = 0;
    for (const auto& part : parts)
    {
        UtilType::TextOptions opts(part.textOptions.fontType,
                                   fontSize,
                                   part.textOptions.rotationAngle,
                                   part.textOptions.letterSpacing);
        const double w = CalculateTextWidth(part.text, opts);
        if (part.underline) { doc.DrawTextUnderline(x, startY, w, opts.fontType, fontSize); }
        x += w;
        totalWidth += w;
    }
    x = startX;
    for (const auto& part : parts)
    {
        UtilType::TextOptions opts(part.textOptions.fontType,
                                   fontSize,
                                   part.textOptions.rotationAngle,
                                   part.textOptions.letterSpacing);
        doc.AddText(part.text, x, startY, opts);
        x += CalculateTextWidth(part.text, opts);
    }
    return totalWidth;
}
