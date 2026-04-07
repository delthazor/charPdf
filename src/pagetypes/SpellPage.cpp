#include <cctype>
#include <string_view>

#include "elements/TextBox.h"
#include "pagetypes/SpellPage.h"
#include "syshelpers/PageConstants.h"

using namespace PageConstants;

namespace
{
constexpr std::string_view UPGRADES_LABEL_TEXT = "At Higher Levels:";
constexpr std::string_view CONCENTRATION_STRING = "Requires Concentration!";
constexpr size_t CONCENTRATION_STRING_LEN = CONCENTRATION_STRING.size();
}

SpellPage::SpellPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params)
    : DescriptionPage(doc, config, side, params, "spellPageText")
{
}

void SpellPage::RenderDescriptionContent(TextBox& box)
{
    std::vector<UtilType::FormattedLabeledBlock> blocks;
    const std::string blockSep("\n\n");

    std::string remainingText = descChunk;
    size_t blockEnd = remainingText.find(blockSep);
    while (blockEnd != std::string::npos)
    {
        std::string block = remainingText.substr(0, blockEnd);
        blocks.push_back(CreateFormattedBlock(parseSpellBlock(block)));
        remainingText = remainingText.substr(blockEnd + blockSep.size());
        blockEnd = remainingText.find(blockSep);
    }
    if (!remainingText.empty()) { blocks.push_back(CreateFormattedBlock(parseSpellBlock(remainingText))); }

    box.RenderFormattedBlocks(blocks);
}

SpellPage::SpellBlock SpellPage::parseSpellBlock(const std::string& block)
{
    SpellBlock spellBlock;
    const size_t openParen = block.find('(');
    const size_t closeParen = block.find(')');

    if (openParen != std::string::npos && closeParen != std::string::npos && closeParen > openParen)
    {
        spellBlock.name = block.substr(0, openParen);
        while (!spellBlock.name.empty() && spellBlock.name.back() == ' ')
        {
            spellBlock.name.pop_back();
        }
        spellBlock.props = block.substr(openParen, closeParen - openParen + 1);
    }
    else
    {
        spellBlock.name = block;
        spellBlock.props.clear();
    }

    size_t descriptionStart = 0;
    const size_t concentrationPos = block.find(CONCENTRATION_STRING);
    if (concentrationPos != std::string::npos)
    {
        spellBlock.concentration = block.substr(concentrationPos, CONCENTRATION_STRING_LEN);
        descriptionStart = concentrationPos + CONCENTRATION_STRING_LEN;
        if (descriptionStart < block.size() && block[descriptionStart] == ' ') { descriptionStart++; }
    }
    else if (closeParen != std::string::npos && closeParen + 1 < block.size())
    {
        descriptionStart = closeParen + 1;
        while (descriptionStart < block.size() &&
               std::isspace(static_cast<unsigned char>(block[descriptionStart])))
        {
            descriptionStart++;
        }
        if (descriptionStart < block.size() && block[descriptionStart] == ':')
        {
            descriptionStart++;
            while (descriptionStart < block.size() && block[descriptionStart] == ' ')
            {
                descriptionStart++;
            }
        }
    }

    const size_t upgradesPos = block.find(UPGRADES_LABEL_TEXT, descriptionStart);
    const size_t descriptionEndPos = (upgradesPos == std::string::npos) ? block.length() : upgradesPos;
    spellBlock.description = block.substr(descriptionStart, descriptionEndPos - descriptionStart);

    spellBlock.upgrades.clear();
    if (upgradesPos != std::string::npos && upgradesPos + UPGRADES_LABEL_TEXT.size() <= block.size())
    {
        spellBlock.upgrades = block.substr(upgradesPos + UPGRADES_LABEL_TEXT.size());
    }
    return spellBlock;
}

UtilType::FormattedLabeledBlock SpellPage::CreateFormattedBlock(const SpellBlock& spellBlock)
{
    UtilType::FormattedLabeledBlock block;
    block.labelParts.push_back({spellBlock.name, UtilType::TextOptions(FontType::ArialBold, 10), false});
    block.textSpans.push_back({spellBlock.props, UtilType::TextOptions(FontType::ArialItalic, 10), false});
    block.textSpans.push_back({":", UtilType::TextOptions(FontType::Arial, 10), false});
    block.textSpans.push_back(
        {spellBlock.concentration, UtilType::TextOptions(FontType::ArialBold, 10), false});
    block.textSpans.push_back({spellBlock.description, UtilType::TextOptions(FontType::Arial, 10), false});
    if (!spellBlock.upgrades.empty())
    {
        block.textSpans.push_back(
            {std::string(UPGRADES_LABEL_TEXT), UtilType::TextOptions(FontType::Arial, 10), true, true});
        block.textSpans.push_back({spellBlock.upgrades, UtilType::TextOptions(FontType::Arial, 10), false});
    }
    return block;
}
