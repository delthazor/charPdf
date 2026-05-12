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

bool LooksLikeCastingParams(std::string_view inner)
{
    while (!inner.empty() && std::isspace(static_cast<unsigned char>(inner.front())))
    {
        inner.remove_prefix(1);
    }
    if (inner.empty()) { return false; }
    if (inner.find(',') != std::string_view::npos) { return true; }
    if (inner.find("range") != std::string_view::npos) { return true; }
    if (inner.find("components") != std::string_view::npos) { return true; }
    if (inner.find("duration") != std::string_view::npos) { return true; }
    return std::isdigit(static_cast<unsigned char>(inner.front())) != 0;
}

void SplitSpellLabelIntoNameAndProps(const std::string& label, std::string& outName, std::string& outProps)
{
    outProps.clear();
    const size_t spacePos = label.rfind(" (");
    if (spacePos == std::string::npos || spacePos + 1 >= label.size() || label[spacePos + 1] != '(')
    {
        outName = label;
        return;
    }
    const size_t openParenAt = spacePos + 1;
    size_t depth = 1;
    size_t j = openParenAt + 1;
    for (; j < label.size(); ++j)
    {
        if (label[j] == '(') { ++depth; }
        else if (label[j] == ')')
        {
            --depth;
            if (depth == 0) { break; }
        }
    }
    if (depth != 0 || j != label.size() - 1)
    {
        outName = label;
        return;
    }
    const size_t innerStart = openParenAt + 1;
    const std::string_view inner(label.data() + innerStart, j - innerStart);
    if (!LooksLikeCastingParams(inner))
    {
        outName = label;
        return;
    }
    outName = label.substr(0, spacePos);
    Utilities::TrimTrailingAsciiSpaces(outName);
    outProps = label.substr(openParenAt, j - openParenAt + 1);
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

    const size_t splitColon = Utilities::FindLastColonSpaceAtParenDepthZero(block);
    if (splitColon == std::string::npos) { return spellPartsToBlock(ParseSimpleSpellBlock(block)); }
    return spellPartsToBlock(ParseStructuredSpellBlock(block, splitColon));
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
