#include "pagetypes/DescriptionPage.h"
#include "elements/TextBox.h"
#include "syshelpers/Utilities.h"

using namespace PageConstants;

DescriptionPage::DescriptionPage(PdfDoc& doc,
                                 const Config& config,
                                 PageSide side,
                                 const PageParams& params,
                                 const std::string& descriptionParamKey)
    : PageBase(doc, config, side, params), descChunk(params.at(descriptionParamKey))
{
    title = Utilities::CapitalizeFirst(descriptionParamKey.substr(0, descriptionParamKey.find('P'))) + "s";
}

bool DescriptionPage::HasDescriptionContent() const { return !descChunk.empty(); }

void DescriptionPage::Draw()
{
    DecorateCorners();
    doc.AddImage("assets/boxribbon.png", LEFT_EDGE_REF + 90, -4, 242, 40);
}

void DescriptionPage::Fill()
{
    doc.AddTextCurved(
        title, Coords(LEFT_EDGE_REF + 178, 3.5), 8, UtilType::TextOptions(FontType::Seagram, 14, 0, 8));

    if (!HasDescriptionContent()) { return; }

    const double startX = LEFT_EDGE_REF + DESCRIPTION_PAGE_BOX_INSET_LEFT;
    const double startY = DESCRIPTION_PAGE_BOX_INSET_TOP;
    doc.DrawRectangleWithFillAndBorder(startX - 2 * MARGIN,
                                       startY - 2 * MARGIN,
                                       DESCRIPTION_PAGE_BOX_WIDTH + 4 * MARGIN,
                                       DESCRIPTION_PAGE_BOX_HEIGHT + 4 * MARGIN,
                                       0xFFFFFF,
                                       0x808080,
                                       1.5);
    TextBox box = TextBox::CreateStandard(*this,
                                          startX + DESCRIPTION_PAGE_CONTENT_MARGIN,
                                          startY + DESCRIPTION_PAGE_CONTENT_MARGIN,
                                          DESCRIPTION_PAGE_FONT_SIZE,
                                          {DESCRIPTION_PAGE_TEXT_WIDTH});
    RenderDescriptionContent(box);
}
