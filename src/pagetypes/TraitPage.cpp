#include "pagetypes/TraitPage.h"
#include "elements/TextBox.h"

TraitPage::TraitPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params)
    : DescriptionPage(doc, config, side, params, "traitPageText")
{
}

void TraitPage::RenderDescriptionContent(TextBox& box)
{
    std::vector<UtilType::LabeledTextBlock> blocks;

    const std::string blockSep("\n\n");
    size_t delimiterpos = descChunk.find(':');
    while (delimiterpos != std::string::npos)
    {
        size_t startpos = descChunk.rfind(blockSep, delimiterpos);
        (startpos == std::string::npos) ? startpos = 0 : startpos += blockSep.size();
        const std::string label = descChunk.substr(startpos, delimiterpos - startpos);
        size_t blockEnd = descChunk.find(blockSep, delimiterpos);
        if (blockEnd == std::string::npos) { blockEnd = descChunk.length(); }
        const std::string text = descChunk.substr(delimiterpos + 1, blockEnd - delimiterpos - 1);
        blocks.push_back(UtilType::LabeledTextBlock(label, text));
        delimiterpos = descChunk.find(':', blockEnd);
    }

    box.RenderBlocks(blocks);
}
