#include <algorithm>
#include <cmath>

#include "PDFWriter/PDFUsedFont.h"
#include "syshelpers/PdfDoc.h"

static std::string GetFontPathForType(PageConstants::FontType fontType)
{
    switch (fontType)
    {
    case PageConstants::FontType::Seagram:
        return "assets/Seagram tfb.ttf";
    case PageConstants::FontType::Arial:
        return "assets/Arial.ttf";
    case PageConstants::FontType::ArialBold:
        return "assets/Arial Bold.ttf";
    case PageConstants::FontType::ArialItalic:
        return "assets/Arial Italic.ttf";
    case PageConstants::FontType::TimesNewRoman:
        return "assets/TimesNewRoman.ttf";
    case PageConstants::FontType::TimesNewRomanBold:
        return "assets/TimesNewRoman Bold.ttf";
    case PageConstants::FontType::TimesNewRomanItalic:
        return "assets/TimesNewRoman Italic.ttf";
    default:
        return "assets/Arial.ttf";
    }
}

PdfDoc::PdfDoc(const std::string& outputPath, double width, double height)
    : pageWidth(width), pageHeight(height), currentPage(nullptr), currentContext(nullptr), isOpen(true)
{
    pdfWriter.StartPDF(outputPath, ePDFVersion13);

    currentPage = new PDFPage();
    currentPage->SetMediaBox(PDFRectangle(0, 0, pageWidth, pageHeight));
    currentContext = pdfWriter.StartPageContentContext(currentPage);
}

PdfDoc::~PdfDoc() { Finalize(); }

PDFUsedFont* PdfDoc::GetCachedFont(const std::string& fontPath)
{
    const auto it = fontCache.find(fontPath);
    if (it != fontCache.end()) { return it->second; }
    PDFUsedFont* const font = pdfWriter.GetFontForFile(fontPath);
    fontCache[fontPath] = font;
    return font;
}

void PdfDoc::AddImage(
    const std::string& filename, double x, double y, double width, double height, double rotationAngle)
{
    if (!isOpen) { return; }

    PageContentContext* const context = currentContext;
    context->q();

    const double pdfY = pageHeight - y - height;

    if (rotationAngle > 0.001)
    {
        const double angleRad = rotationAngle * M_PI / 180.0;
        const double cosA = cos(angleRad);
        const double sinA = sin(angleRad);

        const double centerX = x + width / 2.0;
        const double centerY = pdfY + height / 2.0;

        context->cm(1, 0, 0, 1, centerX, centerY);
        context->cm(cosA, sinA, -sinA, cosA, 0, 0);
        context->cm(1, 0, 0, 1, -centerX, -centerY);
    }

    context->DrawImage(x, pdfY, filename, CreateImageOptions(width, height));

    context->Q();
}

AbstractContentContext::ImageOptions PdfDoc::CreateImageOptions(double width, double height)
{
    AbstractContentContext::ImageOptions imageOpts;
    imageOpts.transformationMethod = AbstractContentContext::eFit;
    imageOpts.fitProportional = false;
    imageOpts.fitPolicy = AbstractContentContext::eAlways;
    imageOpts.boundingBoxWidth = width;
    imageOpts.boundingBoxHeight = height;
    return imageOpts;
}

void PdfDoc::AddText(const std::string& text, double x, double y, const UtilType::TextOptions& textOptions)
{
    if (!isOpen) { return; }

    const std::string fontPath = GetFontPathForType(textOptions.fontType);
    PDFUsedFont* const font = GetCachedFont(fontPath);
    if (!font) { return; }

    // y is top of line in top-down page coords; text anchor aligns with y + fontSize (see DrawTextUnderline).
    const double pdfY = pageHeight - y - textOptions.fontSize;
    const double textWidth = font->CalculateTextAdvance(text, textOptions.fontSize);
    const double textHeight = textOptions.fontSize;

    PageContentContext* const context = currentContext;
    context->q();

    if (std::abs(textOptions.letterSpacing) > 0.001) { context->Tc(textOptions.letterSpacing); }

    if (std::abs(textOptions.rotationAngle) > 0.001)
    {
        const double angleRad = textOptions.rotationAngle * M_PI / 180.0;
        const double cosA = cos(angleRad);
        const double sinA = sin(angleRad);

        const double centerX = x + textWidth / 2.0;
        const double centerY = pdfY + textHeight / 2.0;

        context->cm(1, 0, 0, 1, centerX, centerY);
        context->cm(cosA, sinA, -sinA, cosA, 0, 0);
        context->cm(1, 0, 0, 1, -centerX, -centerY);
    }

    const AbstractContentContext::TextOptions textOpts(
        font, textOptions.fontSize, AbstractContentContext::eGray, 0);
    context->WriteText(x, pdfY, text, textOpts);

    context->Q();
}

void PdfDoc::AddTextCurved(const std::string& text,
                           const Coords& position,
                           double curvature,
                           const UtilType::TextOptions& textOptions)
{
    static const double CURVATURE_ZERO_THRESHOLD = 1e-6;
    static const double MIN_TEXT_WIDTH_FOR_CURVE = 0.5;
    static const int MIN_TEXT_LENGTH_FOR_CURVE = 5;
    static const double RAD_TO_DEG = 180.0 / M_PI;
    static const double PARABOLA_TANGENT_FACTOR = 4.0;
    static const double DISPLACEMENT_DAMPING = 0.24;
    static const double ROTATION_DAMPING = 0.35;
    static const double CURVED_TEXT_LETTER_SPACING = 0.5;
    static const double FIRST_CHAR_LETTER_SPACING = 0.15;
    static const double FIRST_CHAR_T_SPACING = -1.1;

    if (!isOpen) { return; }
    if (text.empty()) { return; }
    if (std::abs(curvature) < CURVATURE_ZERO_THRESHOLD)
    {
        AddText(text, position.x, position.y, textOptions);
        return;
    }

    const double totalWidth = CalculateTextWidth(text, textOptions);
    if (totalWidth < MIN_TEXT_WIDTH_FOR_CURVE || text.size() < MIN_TEXT_LENGTH_FOR_CURVE)
    {
        AddText(text, position.x, position.y, textOptions);
        return;
    }

    const size_t n = text.size();
    const double halfLen = (n > 1) ? (static_cast<double>(n) - 1.0) * 0.5 : 0.0;

    double currentX = position.x;
    for (size_t i = 0; i < n; ++i)
    {
        const char c = text[i];
        const std::string charStr(1, c);
        const double advance = CalculateTextWidth(charStr, textOptions);

        const double progress =
            (halfLen > 1e-9) ? std::min(static_cast<double>(i), static_cast<double>(n - 1 - i)) / halfLen
                             : 0.0;

        const double oneMinusProgress = 1.0 - progress;
        const double curveTerm = (oneMinusProgress * oneMinusProgress <= 1.0)
                                     ? (1.0 - std::sqrt(1.0 - oneMinusProgress * oneMinusProgress))
                                     : 1.0;
        const double yOffset = curvature * curveTerm * DISPLACEMENT_DAMPING;

        const double rotationMagnitude =
            std::atan(PARABOLA_TANGENT_FACTOR * curvature * (1.0 - progress) / totalWidth) * RAD_TO_DEG *
            ROTATION_DAMPING;
        const double rotationDeg =
            (static_cast<double>(i) <= halfLen) ? rotationMagnitude : -rotationMagnitude;

        const Coords charPos(currentX, position.y + yOffset);

        UtilType::TextOptions perCharOptions = textOptions;
        perCharOptions.rotationAngle = rotationDeg;
        perCharOptions.letterSpacing = 0;

        AddText(charStr, charPos.x, charPos.y, perCharOptions);

        currentX += advance;
        if (i + 1 < n)
        {
            if (std::abs(textOptions.letterSpacing) > 0.001) { currentX += textOptions.letterSpacing; }
            double spacing = (i == 0) ? FIRST_CHAR_LETTER_SPACING : CURVED_TEXT_LETTER_SPACING;
            if (i == 0 && c == 'T') { spacing = FIRST_CHAR_T_SPACING; }
            currentX += spacing;
        }
    }
}

void PdfDoc::DrawCircle(double centerX, double centerY, double radius, double strokeWidth)
{
    if (!isOpen) { return; }

    const double pdfY = pageHeight - centerY;
    AbstractContentContext::GraphicOptions options(AbstractContentContext::eStroke);
    options.strokeWidth = strokeWidth;
    currentContext->DrawCircle(centerX, pdfY, radius, options);
}

void PdfDoc::DrawFilledCircle(double centerX, double centerY, double radius)
{
    if (!isOpen) { return; }

    const double pdfY = pageHeight - centerY;
    AbstractContentContext::GraphicOptions options(AbstractContentContext::eFill);
    currentContext->DrawCircle(centerX, pdfY, radius, options);
}

void PdfDoc::DrawRectangleFill(
    double topLeftX, double topLeftY, double width, double height, unsigned long color)
{
    if (!isOpen) { return; }

    const double pdfBottom = pageHeight - topLeftY - height;
    AbstractContentContext::GraphicOptions options(
        AbstractContentContext::eFill, AbstractContentContext::eRGB, color);
    currentContext->DrawRectangle(topLeftX, pdfBottom, width, height, options);
}

void PdfDoc::DrawRectangleBorder(
    double topLeftX, double topLeftY, double width, double height, double strokeWidth, unsigned long color)
{
    if (!isOpen) { return; }

    const double pdfBottom = pageHeight - topLeftY - height;
    AbstractContentContext::GraphicOptions options(
        AbstractContentContext::eStroke, AbstractContentContext::eRGB, color, strokeWidth);
    currentContext->DrawRectangle(topLeftX, pdfBottom, width, height, options);
}

void PdfDoc::DrawRectangleWithFillAndBorder(double topLeftX,
                                            double topLeftY,
                                            double width,
                                            double height,
                                            unsigned long fillColor,
                                            unsigned long borderColor,
                                            double strokeWidth)
{
    DrawRectangleFill(topLeftX, topLeftY, width, height, fillColor);
    DrawRectangleBorder(topLeftX, topLeftY, width, height, strokeWidth, borderColor);
}

PdfDoc::UnderlineMetrics PdfDoc::GetUnderlineMetrics(PageConstants::FontType fontType, double fontSize)
{
    const double fallbackPosition = fontSize * 0.15;
    const double fallbackThickness = fontSize * 0.08;
    const UnderlineMetrics fallback{fallbackPosition, std::max(0.5, fallbackThickness)};

    const std::string path = GetFontPathForType(fontType);
    PDFUsedFont* const font = GetCachedFont(path);
    if (!font) { return fallback; }

    FreeTypeFaceWrapper* const ft = font->GetFreeTypeFont();
    if (!ft) { return fallback; }

    FT_Face face = *ft;
    if (!face || face->units_per_EM == 0) { return fallback; }

    const double scale = fontSize / face->units_per_EM;
    // FreeType: positive underline_position is below the baseline (user Y increases downward).
    double position = static_cast<double>(face->underline_position) * scale;
    double thickness = face->underline_thickness * scale;

    if (face->underline_position == 0) { position = fallbackPosition; }
    if (face->underline_thickness == 0) { thickness = std::max(0.5, fallbackThickness); }

    return {position, thickness};
}

void PdfDoc::DrawTextUnderline(
    double x, double y, double width, PageConstants::FontType fontType, double fontSize)
{
    if (!isOpen) { return; }
    static constexpr double underline_offset = 2.6;
    const UnderlineMetrics m = GetUnderlineMetrics(fontType, fontSize);
    // y is line top (same as AddText); AddText maps baseline using y + fontSize.
    // Nudge below font metrics so the rule sits clearly under the text.
    const double underlineTopY = y + fontSize + m.position + underline_offset;
    const double inset = 1.0;
    DrawLine(x + inset, underlineTopY, x + width - inset, underlineTopY, m.thickness);
}

void PdfDoc::DrawLine(double x1, double y1, double x2, double y2, double strokeWidth, unsigned long color)
{
    if (!isOpen) return;
    const double pdfY1 = pageHeight - y1;
    const double pdfY2 = pageHeight - y2;
    DoubleAndDoublePairList path = {{x1, pdfY1}, {x2, pdfY2}};
    AbstractContentContext::GraphicOptions opts(
        AbstractContentContext::eStroke, AbstractContentContext::eRGB, color, strokeWidth);
    currentContext->DrawPath(path, opts);
}

void PdfDoc::CreateNewPage()
{
    if (!isOpen) { return; }

    pdfWriter.EndPageContentContext(currentContext);
    pdfWriter.WritePageAndRelease(currentPage);

    currentPage = new PDFPage();
    currentPage->SetMediaBox(PDFRectangle(0, 0, pageWidth, pageHeight));
    currentContext = pdfWriter.StartPageContentContext(currentPage);
}

void PdfDoc::Finalize()
{
    if (isOpen) { Save(); }
}

void PdfDoc::Save()
{
    if (!isOpen || currentPage == nullptr || currentContext == nullptr) { return; }

    isOpen = false;

    pdfWriter.EndPageContentContext(currentContext);
    pdfWriter.WritePageAndRelease(currentPage);
    currentPage = nullptr;
    currentContext = nullptr;

    pdfWriter.EndPDF();
}

double PdfDoc::CalculateTextWidth(const std::string& text, const UtilType::TextOptions& textOptions)
{
    const std::string fontPath = GetFontPathForType(textOptions.fontType);
    PDFUsedFont* const font = GetCachedFont(fontPath);
    if (!font) { return text.length() * textOptions.fontSize * 0.5; }

    double width = font->CalculateTextAdvance(text, textOptions.fontSize);

    if (std::abs(textOptions.letterSpacing) > 0.001 && text.length() > 0)
    {
        width += (text.length() - 1) * textOptions.letterSpacing;
    }

    return width;
}