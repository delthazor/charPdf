#pragma once

#include <string>
#include <unordered_map>

#include "PDFWriter/PDFPage.h"
#include "PDFWriter/PDFWriter.h"
#include "PDFWriter/PageContentContext.h"
#include "syshelpers/PageConstants.h"
#include "syshelpers/UtilTypes.h"

class PDFUsedFont;

class PdfDoc
{
  public:
    PdfDoc(const std::string& outputPath, double width, double height);
    ~PdfDoc();

    void AddImage(const std::string& filename,
                  double x,
                  double y,
                  double width,
                  double height,
                  double rotationAngle = 0);
    void AddText(const std::string& text, double x, double y, const UtilType::TextOptions& textOptions = {});
    void AddTextCurved(const std::string& text,
                       const Coords& position,
                       double curvature,
                       const UtilType::TextOptions& textOptions = {});
    void DrawCircle(double centerX, double centerY, double radius, double strokeWidth = 1.0);
    void DrawFilledCircle(double centerX, double centerY, double radius);
    void DrawRectangleFill(
        double topLeftX, double topLeftY, double width, double height, unsigned long color = 0xFFFFFF);
    void DrawRectangleBorder(double topLeftX,
                             double topLeftY,
                             double width,
                             double height,
                             double strokeWidth = 1.0,
                             unsigned long color = 0x000000);
    void DrawRectangleWithFillAndBorder(double topLeftX,
                                        double topLeftY,
                                        double width,
                                        double height,
                                        unsigned long fillColor = 0xFFFFFF,
                                        unsigned long borderColor = 0x000000,
                                        double strokeWidth = 1.0);
    void DrawLine(
        double x1, double y1, double x2, double y2, double strokeWidth = 0.5, unsigned long color = 0x000000);
    void
    DrawTextUnderline(double x, double y, double width, PageConstants::FontType fontType, double fontSize);

    void CreateNewPage();
    void Finalize();

    double CalculateTextWidth(const std::string& text, const UtilType::TextOptions& textOptions);

  private:
    struct UnderlineMetrics
    {
        double position;
        double thickness;
    };
    UnderlineMetrics GetUnderlineMetrics(PageConstants::FontType fontType, double fontSize);
    PDFUsedFont* GetCachedFont(const std::string& fontPath);

    void Save();
    AbstractContentContext::ImageOptions CreateImageOptions(double width, double height);

    const double pageWidth;
    const double pageHeight;

    PDFWriter pdfWriter;
    std::unordered_map<std::string, PDFUsedFont*> fontCache;
    PDFPage* currentPage;
    PageContentContext* currentContext;
    bool isOpen;
};
