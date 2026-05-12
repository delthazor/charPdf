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

size_t FindLastColonSpaceAtParenDepthZero(const std::string& block)
{
    int depth = 0;
    size_t last = std::string::npos;
    for (size_t i = 0; i + 1 < block.size(); ++i)
    {
        const char c = block[i];
        if (c == '(')
        {
            ++depth;
        }
        else if (c == ')' && depth > 0)
        {
            --depth;
        }
        else if (c == ':' && block[i + 1] == ' ' && depth == 0)
        {
            last = i;
        }
    }
    return last;
}

void TrimTrailingAsciiSpaces(std::string& s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    {
        s.pop_back();
    }
}

bool LooksLikeCastingParams(std::string_view inner)
{
    while (!inner.empty() && std::isspace(static_cast<unsigned char>(inner.front())))
    {
        inner.remove_prefix(1);
    }
    if (inner.empty())
    {
        return false;
    }
    if (inner.find(',') != std::string_view::npos)
    {
        return true;
    }
    if (inner.find("range") != std::string_view::npos)
    {
        return true;
    }
    if (inner.find("components") != std::string_view::npos)
    {
        return true;
    }
    if (inner.find("duration") != std::string_view::npos)
    {
        return true;
    }
    return std::isdigit(static_cast<unsigned char>(inner.front())) != 0;
}

/// `label` is the substring before `": "` from `BuildFullSpellsText`. When the spell has JSON
/// `params`, that line ends with a single trailing ` (...)` casting block; spell `name` may also
/// end with ` (...)` (e.g. "(in-training)"), so only the last suffix is treated as casting params
/// when its inner text looks like a casting line.
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
        if (label[j] == '(')
        {
            ++depth;
        }
        else if (label[j] == ')')
        {
            --depth;
            if (depth == 0)
            {
                break;
            }
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
    TrimTrailingAsciiSpaces(outName);
    outProps = label.substr(openParenAt, j - openParenAt + 1);
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
    const size_t splitColon = FindLastColonSpaceAtParenDepthZero(block);
    if (splitColon == std::string::npos)
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

    SpellBlock spellBlock;
    std::string label = block.substr(0, splitColon);
    TrimTrailingAsciiSpaces(label);
    SplitSpellLabelIntoNameAndProps(label, spellBlock.name, spellBlock.props);

    size_t afterColon = splitColon + 2;
    while (afterColon < block.size() && std::isspace(static_cast<unsigned char>(block[afterColon])))
    {
        ++afterColon;
    }
    const std::string rest = block.substr(afterColon);

    size_t descStartInRest = 0;
    const size_t concentrationPos = rest.find(CONCENTRATION_STRING);
    if (concentrationPos != std::string::npos)
    {
        spellBlock.concentration = rest.substr(concentrationPos, CONCENTRATION_STRING_LEN);
        descStartInRest = concentrationPos + CONCENTRATION_STRING_LEN;
        if (descStartInRest < rest.size() && rest[descStartInRest] == ' ') { ++descStartInRest; }
    }

    const size_t upgradesPos = rest.find(UPGRADES_LABEL_TEXT, descStartInRest);
    const size_t descriptionEndPos = (upgradesPos == std::string::npos) ? rest.length() : upgradesPos;
    spellBlock.description = rest.substr(descStartInRest, descriptionEndPos - descStartInRest);

    spellBlock.upgrades.clear();
    if (upgradesPos != std::string::npos && upgradesPos + UPGRADES_LABEL_TEXT.size() <= rest.size())
    {
        spellBlock.upgrades = rest.substr(upgradesPos + UPGRADES_LABEL_TEXT.size());
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
