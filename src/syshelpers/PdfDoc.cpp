#include <algorithm>
#include <cmath>
#include <vector>

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

PdfDoc::CurvedRunLayout PdfDoc::buildCurvedRunLayout(const std::string& text,
                                                     const UtilType::TextOptions& textOptions)
{
    static const double CURVED_TEXT_LETTER_SPACING = 0.5;
    static const double FIRST_CHAR_LETTER_SPACING = 0.15;
    static const double FIRST_CHAR_T_SPACING = -1.1;

    CurvedRunLayout layout;
    const size_t n = text.size();
    layout.leftEdge.reserve(n);
    layout.advance.reserve(n);

    double x = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const char c = text[i];
        const std::string charStr(1, c);
        const double advance = CalculateTextWidth(charStr, textOptions);
        layout.leftEdge.push_back(x);
        layout.advance.push_back(advance);
        x += advance;
        if (i + 1 < n)
        {
            if (std::abs(textOptions.letterSpacing) > 0.001) { x += textOptions.letterSpacing; }
            double spacing = (i == 0) ? FIRST_CHAR_LETTER_SPACING : CURVED_TEXT_LETTER_SPACING;
            if (i == 0 && c == 'T') { spacing = FIRST_CHAR_T_SPACING; }
            x += spacing;
        }
    }
    layout.runWidth = x;
    return layout;
}

namespace
{
double CurveTermFromSymmetricProgress(const double progress)
{
    const double oneMinusProgress = 1.0 - progress;
    if (oneMinusProgress * oneMinusProgress <= 1.0)
    {
        return 1.0 - std::sqrt(1.0 - oneMinusProgress * oneMinusProgress);
    }
    return 1.0;
}
} // namespace

void PdfDoc::AddTextCurved(const std::string& text,
                           const Coords& position,
                           double curvature,
                           const UtilType::TextOptions& textOptions)
{
    AddTextCurvedLinear(text, position, curvature, textOptions);
}

void PdfDoc::AddTextCurvedLinear(const std::string& text,
                                 const Coords& position,
                                 double curvature,
                                 const UtilType::TextOptions& textOptions)
{
    static const double CURVATURE_ZERO_THRESHOLD = 1e-6;
    static const double MIN_TEXT_WIDTH_FOR_CURVE = 0.5;
    static const int MIN_TEXT_LENGTH_FOR_CURVE = 5;
    static const double RAD_TO_DEG = 180.0 / M_PI;
    static const double DISPLACEMENT_DAMPING = 0.24;
    static const double ROTATION_DAMPING = 0.35;
    // Same caller `curvature` as before refactor; internal boost restores perceived bend after
    // distance-based progress + slope rotation (replaced index progress + atan(.../totalWidth)).
    static const double LINEAR_VISUAL_CURVATURE_GAIN = 2.15;
    // Symmetric bump uses a point along each glyph toward the left (1 = glyph center). Same idea
    // as circular chord sampling: uneven first-gap / spacing shifts optical mass vs runWidth/2.
    static const double LINEAR_CURVE_PROGRESS_BLEND = 0.86;

    if (!isOpen) { return; }
    if (text.empty()) { return; }
    if (std::abs(curvature) < CURVATURE_ZERO_THRESHOLD)
    {
        AddText(text, position.x, position.y, textOptions);
        return;
    }

    const CurvedRunLayout layout = buildCurvedRunLayout(text, textOptions);
    const double runWidth = layout.runWidth;
    if (runWidth < MIN_TEXT_WIDTH_FOR_CURVE || text.size() < MIN_TEXT_LENGTH_FOR_CURVE)
    {
        AddText(text, position.x, position.y, textOptions);
        return;
    }

    const double effectiveCurvature = curvature * LINEAR_VISUAL_CURVATURE_GAIN;

    const size_t n = text.size();
    std::vector<double> yOffset(n);
    std::vector<double> lineTopX(n);
    std::vector<double> lineTopY(n);

    for (size_t i = 0; i < n; ++i)
    {
        double progress = 0.0;
        if (runWidth > 1e-12)
        {
            const double sampleAlong =
                layout.leftEdge[i] + LINEAR_CURVE_PROGRESS_BLEND * (layout.advance[i] * 0.5);
            const double halfSpan = runWidth * 0.5;
            progress = std::min(sampleAlong, runWidth - sampleAlong) / halfSpan;
            progress = std::clamp(progress, 0.0, 1.0);
        }
        const double curveTerm = CurveTermFromSymmetricProgress(progress);
        yOffset[i] = effectiveCurvature * curveTerm * DISPLACEMENT_DAMPING;
        lineTopX[i] = position.x + layout.leftEdge[i];
        lineTopY[i] = position.y + yOffset[i];
    }

    for (size_t i = 0; i < n; ++i)
    {
        const std::string charStr(1, text[i]);

        double dx = 1.0;
        double dy = 0.0;
        if (n == 1)
        {
            dx = 1.0;
            dy = 0.0;
        }
        else if (i == 0)
        {
            dx = lineTopX[1] - lineTopX[0];
            dy = lineTopY[1] - lineTopY[0];
        }
        else if (i + 1 == n)
        {
            dx = lineTopX[i] - lineTopX[i - 1];
            dy = lineTopY[i] - lineTopY[i - 1];
        }
        else
        {
            dx = lineTopX[i + 1] - lineTopX[i - 1];
            dy = lineTopY[i + 1] - lineTopY[i - 1];
        }

        const double rotationDeg = -std::atan2(dy, dx) * RAD_TO_DEG * ROTATION_DAMPING;

        const Coords charPos(lineTopX[i], lineTopY[i]);
        UtilType::TextOptions perCharOptions(
            textOptions.fontType, textOptions.fontSize, rotationDeg, textOptions.letterSpacing);
        AddText(charStr, charPos.x, charPos.y, perCharOptions);
    }
}

void PdfDoc::AddTextCurvedCircular(const std::string& text,
                                   const Coords& position,
                                   double curvature,
                                   const UtilType::TextOptions& textOptions)
{
    static const double CURVATURE_ZERO_THRESHOLD = 1e-6;
    static const double MIN_TEXT_WIDTH_FOR_CURVE = 0.5;
    static const int MIN_TEXT_LENGTH_FOR_CURVE = 5;
    static const double RAD_TO_DEG = 180.0 / M_PI;
    static const double MAX_PHI = 0.95 * M_PI;
    // Larger |curvature| => smaller radius (tighter bend), aligned with linear "more curvature" feel.
    static const double R_SCALE = 2200.0;
    static const double ROTATION_DAMPING = 1.0;
    // (px,py) is baseline-left on the arc. Extra down in font units: between tight (≈0.38) and
    // previous heavy nudge (1.75) so line top sits between “too high” and “too low”.
    static const double CIRCULAR_LINE_TOP_EXTRA_DOWN = 1.12;

    if (!isOpen) { return; }
    if (text.empty()) { return; }
    if (std::abs(curvature) < CURVATURE_ZERO_THRESHOLD)
    {
        AddText(text, position.x, position.y, textOptions);
        return;
    }

    const CurvedRunLayout layout = buildCurvedRunLayout(text, textOptions);
    const double L = layout.runWidth;
    if (L < MIN_TEXT_WIDTH_FOR_CURVE || text.size() < MIN_TEXT_LENGTH_FOR_CURVE)
    {
        AddText(text, position.x, position.y, textOptions);
        return;
    }

    const size_t n = text.size();
    // Horizontal span of the laid-out run (same as linear); chord subtends phi on the circle.
    const double chord = std::max(L, 1e-9);
    const double halfChord = chord * 0.5;
    double R = R_SCALE / std::max(std::abs(curvature), CURVATURE_ZERO_THRESHOLD);
    // Curvature was tuned for linear y/rotation, not circle geometry. When R would fall near
    // halfChord, phi -> pi (semicircle) and the middle spikes. Keep R at least ~1.32 * halfChord
    // so phi stays bounded (~1.65 rad max) for wide labels (e.g. Backpack curvature 99, fs 28).
    static const double CIRCULAR_MIN_R_OVER_HALF_CHORD = 1.32;
    R = std::max(R, halfChord * CIRCULAR_MIN_R_OVER_HALF_CHORD);

    double phi = 2.0 * std::asin(std::min(halfChord / R, 1.0));
    if (phi > MAX_PHI)
    {
        R = halfChord / std::sin(MAX_PHI * 0.5);
        phi = MAX_PHI;
    }

    const double My = position.y;
    const double bumpSign = (curvature > 0.0) ? 1.0 : -1.0;
    const double dCenter = std::sqrt(std::max(R * R - halfChord * halfChord, 0.0));
    const double Cx = position.x + halfChord;
    const double Cy = My + bumpSign * dCenter;

    const double xLeft = position.x;
    const double xRight = position.x + chord;
    const double thetaLeft = std::atan2(My - Cy, xLeft - Cx);
    // Central angle for chord length is phi; pick sweep so the arc ends at xRight (minor-arc branch).
    double sweep = ((curvature > 0.0) ? 1.0 : -1.0) * phi;
    auto endXForSweep = [&](const double sw) { return Cx + R * std::cos(thetaLeft + sw); };
    if (std::fabs(endXForSweep(sweep) - xRight) > std::fabs(endXForSweep(-sweep) - xRight)) { sweep = -sweep; }
    const double deltaTheta = sweep;

    const double fontSize = textOptions.fontSize;

    for (size_t i = 0; i < n; ++i)
    {
        const std::string charStr(1, text[i]);
        const double adv = layout.advance[i];
        // Chord fraction at glyph center (symmetric on arc; avoids left-edge lean / crowding).
        const double sCenter = layout.leftEdge[i] + adv * 0.5;
        const double t = (chord > 1e-12) ? std::clamp(sCenter / chord, 0.0, 1.0) : 0.0;
        const double theta = thetaLeft + t * deltaTheta;

        const double pxC = Cx + R * std::cos(theta);
        const double pyC = Cy + R * std::sin(theta);
        // Unit tangent along reading direction (matches sign of deltaTheta).
        const double tDir = (deltaTheta >= 0.0) ? 1.0 : -1.0;
        const double tx = -tDir * std::sin(theta);
        const double ty = tDir * std::cos(theta);
        const double px = pxC - 0.5 * adv * tx;
        const double py = pyC - 0.5 * adv * ty;

        const double lineTopX = px;
        const double lineTopY = py - fontSize + CIRCULAR_LINE_TOP_EXTRA_DOWN * fontSize;

        const double rotationDeg = -std::atan2(ty, tx) * RAD_TO_DEG * ROTATION_DAMPING;

        UtilType::TextOptions perCharOptions(
            textOptions.fontType, textOptions.fontSize, rotationDeg, textOptions.letterSpacing);
        AddText(charStr, lineTopX, lineTopY, perCharOptions);
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