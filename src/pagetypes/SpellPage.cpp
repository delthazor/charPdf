#include <cctype>
#include <string_view>

#include "elements/TextBox.h"
#include "pagetypes/SpellPage.h"
#include "syshelpers/PageConstants.h"
#include "syshelpers/Utilities.h"

using namespace PageConstants;

namespace
{
constexpr std::string_view UPGRADES_LABEL_TEXT = "At Higher Levels:";
constexpr std::string_view CONCENTRATION_STRING = "Requires Concentration!";
constexpr size_t CONCENTRATION_STRING_LEN = CONCENTRATION_STRING.size();

bool FindOpeningParenMatchingFinalClose(const std::string& label, size_t& outOpenParenIdx)
{
    if (label.empty() || label.back() != ')') { return false; }
    if (label.size() < 2) { return false; }
    int depth = 1;
    for (size_t i = label.size() - 2;; --i)
    {
        const char c = label[i];
        if (c == ')') { ++depth; }
        else if (c == '(')
        {
            --depth;
            if (depth == 0)
            {
                outOpenParenIdx = i;
                return true;
            }
        }
        if (i == 0) { break; }
    }
    return false;
}

void SplitSpellLabelIntoNameAndProps(const std::string& label, std::string& outName, std::string& outProps)
{
    outProps.clear();
    size_t openParenIdx = 0;
    if (!FindOpeningParenMatchingFinalClose(label, openParenIdx))
    {
        outName = label;
        return;
    }
    outProps = label.substr(openParenIdx, label.size() - openParenIdx);
    outName = label.substr(0, openParenIdx);
    Utilities::TrimTrailingAsciiSpaces(outName);
}

void AssignDescriptionAndUpgrades(const std::string& source,
                                  size_t descriptionContentStart,
                                  UtilType::SpellParts& out)
{
    const size_t upgradesPos = source.find(UPGRADES_LABEL_TEXT, descriptionContentStart);
    const size_t descriptionEnd = (upgradesPos == std::string::npos) ? source.length() : upgradesPos;
    out.description = source.substr(descriptionContentStart, descriptionEnd - descriptionContentStart);
    out.upgrades.clear();
    if (upgradesPos != std::string::npos && upgradesPos + UPGRADES_LABEL_TEXT.size() <= source.size())
    {
        out.upgrades = source.substr(upgradesPos + UPGRADES_LABEL_TEXT.size());
    }
}

size_t ConsumeOptionalConcentrationPrefix(const std::string& source,
                                          size_t searchFrom,
                                          UtilType::SpellParts& out)
{
    out.concentration.clear();
    const size_t pos = source.find(CONCENTRATION_STRING, searchFrom);
    if (pos == std::string::npos) { return searchFrom; }
    out.concentration = source.substr(pos, CONCENTRATION_STRING_LEN);
    size_t after = pos + CONCENTRATION_STRING_LEN;
    if (after < source.size() && source[after] == ' ') { ++after; }
    return after;
}

std::string TextAfterLabelColon(const std::string& block, size_t labelEndColon)
{
    size_t i = labelEndColon + 2;
    while (i < block.size() && std::isspace(static_cast<unsigned char>(block[i])))
    {
        ++i;
    }
    return block.substr(i);
}

void SimpleSplitNameAndProps(const std::string& block, UtilType::SpellParts& out)
{
    const size_t openParen = block.find('(');
    const size_t closeParen = block.find(')');
    if (openParen != std::string::npos && closeParen != std::string::npos && closeParen > openParen)
    {
        out.name = block.substr(0, openParen);
        Utilities::TrimTrailingAsciiSpaces(out.name);
        out.props = block.substr(openParen, closeParen - openParen + 1);
    }
    else
    {
        out.name = block;
        out.props.clear();
    }
}

UtilType::SpellParts ParseStructuredSpellBlock(const std::string& block, size_t labelEndColon)
{
    UtilType::SpellParts out;
    std::string label = block.substr(0, labelEndColon);
    Utilities::TrimTrailingAsciiSpaces(label);
    SplitSpellLabelIntoNameAndProps(label, out.name, out.props);

    const std::string rest = TextAfterLabelColon(block, labelEndColon);
    const size_t descStart = ConsumeOptionalConcentrationPrefix(rest, 0, out);
    AssignDescriptionAndUpgrades(rest, descStart, out);
    return out;
}

UtilType::SpellParts ParseSimpleSpellBlock(const std::string& block)
{
    UtilType::SpellParts out;
    SimpleSplitNameAndProps(block, out);

    size_t descriptionStart = ConsumeOptionalConcentrationPrefix(block, 0, out);
    if (out.concentration.empty())
    {
        const size_t closeParen = block.find(')');
        if (closeParen != std::string::npos && closeParen + 1 < block.size())
        {
            descriptionStart = closeParen + 1;
            while (descriptionStart < block.size() &&
                   std::isspace(static_cast<unsigned char>(block[descriptionStart])))
            {
                ++descriptionStart;
            }
            if (descriptionStart < block.size() && block[descriptionStart] == ':')
            {
                ++descriptionStart;
                while (descriptionStart < block.size() && block[descriptionStart] == ' ')
                {
                    ++descriptionStart;
                }
            }
        }
    }

    AssignDescriptionAndUpgrades(block, descriptionStart, out);
    return out;
}

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
    auto spellPartsToBlock = [](UtilType::SpellParts&& p) -> SpellBlock
    {
        SpellBlock b;
        b.name = std::move(p.name);
        b.props = std::move(p.props);
        b.concentration = std::move(p.concentration);
        b.description = std::move(p.description);
        b.upgrades = std::move(p.upgrades);
        return b;
    };

    const size_t splitColon = Utilities::FindFirstColonSpaceAtParenDepthZero(block);
    if (splitColon == std::string::npos) { return spellPartsToBlock(ParseSimpleSpellBlock(block)); }
    return spellPartsToBlock(ParseStructuredSpellBlock(block, splitColon));
}

UtilType::FormattedLabeledBlock SpellPage::CreateFormattedBlock(const SpellBlock& spellBlock)
{
    UtilType::FormattedLabeledBlock block;
    block.labelParts.push_back({spellBlock.name, UtilType::TextOptions(FontType::ArialBold, 10), false});
    block.textSpans.push_back({spellBlock.props, UtilType::TextOptions(FontType::ArialItalic, 10), false});
    block.textSpans.push_back({":", UtilType::TextOptions(FontType::Arial, 10), false, false, true});
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
