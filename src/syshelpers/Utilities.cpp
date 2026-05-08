#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "syshelpers/PageConstants.h"
#include "syshelpers/Utilities.h"

namespace
{

constexpr std::string_view PARAGRAPH_SEPARATOR = "\n\n";

std::vector<std::string> splitParagraphs(const std::string& fullText)
{
    std::vector<std::string> raw = Utilities::Split(fullText, std::string(PARAGRAPH_SEPARATOR));
    std::vector<std::string> out;
    out.reserve(raw.size());
    for (std::string& part : raw)
    {
        if (!part.empty()) { out.push_back(std::move(part)); }
    }
    return out;
}

size_t paginationCharsPerLine()
{
    using namespace PageConstants;
    return static_cast<size_t>(std::floor(DESCRIPTION_PAGE_TEXT_WIDTH / DESCRIPTION_PAGE_EST_CHAR_WIDTH));
}

size_t estimateMaxCharsPerPage()
{
    using namespace PageConstants;
    const size_t charsPerLine = paginationCharsPerLine();
    const double lineHeight = Utilities::CalculateLineHeight(DESCRIPTION_PAGE_FONT_SIZE);
    size_t linesPerPage = static_cast<size_t>(std::floor(DESCRIPTION_PAGE_TEXT_USABLE_HEIGHT / lineHeight));
    const size_t blockLineReserve = static_cast<size_t>(std::ceil(TEXTBOX_BLOCK_SPACING / lineHeight));
    if (linesPerPage > blockLineReserve) { linesPerPage -= blockLineReserve; }
    else
    {
        linesPerPage = 1;
    }
    constexpr size_t SAFETY_LINES = 2;
    if (linesPerPage > SAFETY_LINES) { linesPerPage -= SAFETY_LINES; }
    else
    {
        linesPerPage = 1;
    }
    return charsPerLine * std::max<size_t>(1, linesPerPage);
}

size_t interParagraphSpacingCharPenalty()
{
    const double lineHeight = Utilities::CalculateLineHeight(PageConstants::DESCRIPTION_PAGE_FONT_SIZE);
    const size_t cpl = paginationCharsPerLine();
    const double gapLines = PageConstants::TEXTBOX_BLOCK_SPACING / lineHeight;
    return static_cast<size_t>(std::ceil(gapLines * static_cast<double>(cpl)));
}

bool StripOneTrailingBracketContext(std::string& s)
{
    const size_t pos = s.rfind(" [");
    if (pos == std::string::npos) { return false; }
    if (pos + 2 >= s.size()) { return false; }
    if (s[pos + 1] != '[') { return false; }
    size_t depth = 1;
    size_t j = pos + 2;
    for (; j < s.size(); ++j)
    {
        if (s[j] == '[') { ++depth; }
        else if (s[j] == ']')
        {
            --depth;
            if (depth == 0) { break; }
        }
    }
    if (depth != 0 || j != s.size() - 1) { return false; }
    s.resize(pos);
    while (!s.empty() && s.back() == ' ') { s.pop_back(); }
    return true;
}

bool StripOneTrailingParenContext(std::string& s)
{
    const size_t pos = s.rfind(" (");
    if (pos == std::string::npos) { return false; }
    if (pos + 2 >= s.size()) { return false; }
    if (s[pos + 1] != '(') { return false; }
    size_t depth = 1;
    size_t j = pos + 2;
    for (; j < s.size(); ++j)
    {
        if (s[j] == '(') { ++depth; }
        else if (s[j] == ')')
        {
            --depth;
            if (depth == 0) { break; }
        }
    }
    if (depth != 0 || j != s.size() - 1) { return false; }
    s.resize(pos);
    while (!s.empty() && s.back() == ' ') { s.pop_back(); }
    return true;
}

std::string ToLowerAscii(std::string_view sv)
{
    std::string out;
    out.reserve(sv.size());
    for (unsigned char c : sv)
    {
        out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

void stripLeadingAtHigherLevels(std::string& upgrades)
{
    constexpr std::string_view prefix = "At Higher Levels:";
    size_t i = 0;
    while (i < upgrades.size() && std::isspace(static_cast<unsigned char>(upgrades[i])))
    {
        i++;
    }
    if (upgrades.size() - i < prefix.size()) { return; }
    if (upgrades.compare(i, prefix.size(), prefix) != 0) { return; }
    i += prefix.size();
    while (i < upgrades.size() && std::isspace(static_cast<unsigned char>(upgrades[i])))
    {
        i++;
    }
    upgrades.erase(0, i);
}

}

namespace Utilities
{

void LogInfo(std::string_view message) { std::cout << message << std::endl; }

void LogError(std::string_view message) { std::cerr << message << std::endl; }

std::string SanitizePdfStem(std::string stem)
{
    for (char& c : stem)
    {
        if (c == ' ') { c = '_'; }
    }
    return stem;
}

std::string CollectPlainLine(const std::vector<std::string>& words,
                             size_t& wordIndex,
                             double maxWidth,
                             const UtilType::TextOptions& valueOpts,
                             const UtilType::MeasureFn& measure)
{
    std::string outLine;
    double runW = 0;
    while (wordIndex < words.size())
    {
        const std::string& w = words[wordIndex];
        const bool firstOnLine = outLine.empty();
        const double spaceW = firstOnLine ? 0 : measure(" ", valueOpts);
        const double wordW = measure(w, valueOpts);
        const double withWord = runW + spaceW + wordW;
        if (!firstOnLine && withWord > maxWidth) { return outLine; }
        if (firstOnLine && wordW > maxWidth)
        {
            outLine = w;
            wordIndex++;
            return outLine;
        }
        if (!firstOnLine) { outLine += " "; }
        outLine += w;
        runW = withWord;
        wordIndex++;
    }
    return outLine;
}

std::vector<UtilType::StyledWord> CollectStyledLine(const std::vector<UtilType::StyledWord>& words,
                                                    size_t& wordIndex,
                                                    double maxWidth,
                                                    const UtilType::MeasureFn& measure)
{
    std::vector<UtilType::StyledWord> outLine;
    double runW = 0;
    while (wordIndex < words.size())
    {
        const UtilType::StyledWord& w = words[wordIndex];
        if (w.forceLineBreakBefore && !outLine.empty()) { return outLine; }

        const bool firstOnLine = outLine.empty();
        const double spaceW = firstOnLine ? 0 : measure(" ", w.textOptions);
        const double wordW = measure(w.text, w.textOptions);
        const double withWord = runW + spaceW + wordW;
        if (!firstOnLine && withWord > maxWidth) { return outLine; }
        if (firstOnLine && wordW > maxWidth)
        {
            outLine.push_back(w);
            wordIndex++;
            return outLine;
        }
        outLine.push_back(w);
        runW = withWord;
        wordIndex++;
    }
    return outLine;
}

double RemainingLineWidth(std::span<const double> lineWidthView, size_t lineIdx)
{
    if (lineWidthView.empty()) { return 0; }
    return lineIdx < lineWidthView.size() ? lineWidthView[lineIdx] : lineWidthView.back();
}

void EnsureLineLimit(size_t lineIndex)
{
    if (lineIndex >= PageConstants::TEXTWRAP_LINE_LIMIT)
    {
        throw std::runtime_error("text wrap line limit exceeded");
    }
}

UtilType::TraitsCatalog BuildTraitsCatalog(nlohmann::ordered_json arr)
{
    UtilType::TraitsCatalog out;
    out.raw = std::move(arr);
    if (!out.raw.is_array()) { return out; }
    for (const auto& elem : out.raw)
    {
        const std::string name = elem.value("name", std::string());
        if (!name.empty() && !out.byName.contains(name)) { out.byName.emplace(name, elem); }
    }
    return out;
}

UtilType::SpellsCatalog BuildSpellsCatalog(nlohmann::ordered_json arr)
{
    UtilType::SpellsCatalog out;
    out.raw = std::move(arr);
    if (!out.raw.is_array()) { return out; }
    for (const auto& elem : out.raw)
    {
        const std::string name = elem.value("name", std::string());
        if (!name.empty() && !out.byName.contains(name))
        {
            out.byName.emplace(name, elem);
            const std::string lower = ToLowerAscii(name);
            if (!out.byLowerName.contains(lower)) { out.byLowerName.emplace(lower, name); }
        }
    }
    return out;
}

std::string BuildFullTraitsText(const Config& characterConfig, const UtilType::TraitsCatalog& traitsCatalog)
{
    std::string fullText;

    for (const std::string& name : characterConfig.getStringArray("traits"))
    {
        const auto it = traitsCatalog.byName.find(name);
        if (it != traitsCatalog.byName.end())
        {
            std::string desc = it->second.value("description", std::string());
            fullText += name + ": " + desc + "\n\n";
        }
        else
        {
            Utilities::LogError("ERROR: Trait data is missing for trait: " + name);
        }
    }

    return fullText;
}

std::string SpellNameForCatalogLookup(const std::string& displayName)
{
    std::string s = displayName;
    while (!s.empty() && s.back() == ' ') { s.pop_back(); }
    size_t lead = 0;
    while (lead < s.size() && std::isspace(static_cast<unsigned char>(s[lead]))) { ++lead; }
    if (lead > 0) { s.erase(0, lead); }

    for (;;)
    {
        const bool strippedBracket = StripOneTrailingBracketContext(s);
        const bool strippedParen = StripOneTrailingParenContext(s);
        if (!strippedBracket && !strippedParen) { break; }
    }
    return s;
}

std::string BuildFullSpellsText(const Config& characterConfig, const UtilType::SpellsCatalog& spellsCatalog)
{
    std::string fullText;
    if (!characterConfig.hasKey("classes")) return fullText;

    std::set<std::string> seen;

    const Config classesObj = characterConfig.getObject("classes");
    for (const std::string& classId : classesObj.getKeys())
    {
        const Config classObj = classesObj.getObject(classId);
        if (!classObj.hasKey("spells")) continue;

        const Config spellsObj = classObj.getObject("spells");

        for (size_t i = 0; i < 10; ++i)
        {
            const std::string levelKey = std::to_string(i);
            if (!spellsObj.hasKey(levelKey)) continue;

            for (const std::string& name : spellsObj.getStringArray(levelKey))
            {
                const std::string catalogKey = SpellNameForCatalogLookup(name);
                if (catalogKey.empty())
                {
                    Utilities::LogError("ERROR: Spell name is empty after context suffix strip: " + name);
                    continue;
                }
                auto spellIt = spellsCatalog.byName.find(catalogKey);
                if (spellIt == spellsCatalog.byName.end())
                {
                    const auto lit = spellsCatalog.byLowerName.find(ToLowerAscii(catalogKey));
                    if (lit != spellsCatalog.byLowerName.end())
                    {
                        spellIt = spellsCatalog.byName.find(lit->second);
                    }
                }
                if (spellIt == spellsCatalog.byName.end())
                {
                    Utilities::LogError("ERROR: Spell data is missing for spell: " + name
                                        + " (catalog key: " + catalogKey + ")");
                    continue;
                }
                const std::string& canonicalKey = spellIt->first;
                if (seen.insert(canonicalKey).second)
                {
                    std::string label = spellIt->second.value("name", canonicalKey);
                    if (spellIt->second.contains("params"))
                    {
                        label += " (";
                        label += spellIt->second.value("params", std::string());
                        label += ")";
                    }

                    std::string desc = spellIt->second.value("description", std::string());
                    if (spellIt->second.contains("concentration")
                        && spellIt->second.value("concentration", true))
                    {
                        desc = "Requires Concentration! " + desc;
                    }
                    if (spellIt->second.contains("upgrades"))
                    {
                        std::string up = spellIt->second.value("upgrades", std::string());
                        stripLeadingAtHigherLevels(up);
                        desc += "\nAt Higher Levels: ";
                        desc += up;
                    }
                    fullText += label + ": " + desc + "\n\n";
                }
            }
        }
    }

    return fullText;
}

std::vector<std::string> SplitDescriptionTextIntoPages(const std::string& fullText)
{
    std::vector<std::string> chunks;
    if (fullText.empty()) { return chunks; }

    const std::vector<std::string> paragraphs = splitParagraphs(fullText);
    if (paragraphs.empty()) { return chunks; }

    const size_t maxCharsPerPage = estimateMaxCharsPerPage();
    const size_t spacingPenaltyChars = interParagraphSpacingCharPenalty();

    std::string current;
    size_t chunkParaCount = 0;
    for (const std::string& para : paragraphs)
    {
        if (para.size() > maxCharsPerPage)
        {
            if (!current.empty())
            {
                chunks.push_back(std::move(current));
                current.clear();
                chunkParaCount = 0;
            }
            chunks.push_back(para);
            continue;
        }

        const size_t sepLen = current.empty() ? 0 : PARAGRAPH_SEPARATOR.size();
        const size_t gapCount = chunkParaCount;
        const size_t effectiveLoad = current.size() + sepLen + para.size() + gapCount * spacingPenaltyChars;
        if (effectiveLoad > maxCharsPerPage && !current.empty())
        {
            chunks.push_back(std::move(current));
            current.clear();
            chunkParaCount = 0;
        }

        if (!current.empty()) { current += PARAGRAPH_SEPARATOR; }
        current += para;
        chunkParaCount++;
    }

    if (!current.empty()) { chunks.push_back(std::move(current)); }
    return chunks;
}

nlohmann::ordered_json LoadJsonFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) { throw std::runtime_error("cannot open JSON file: " + path); }
    return nlohmann::ordered_json::parse(file);
}

std::string CapitalizeFirst(const std::string& s)
{
    if (s.empty()) return s;
    std::string out = s;
    out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return out;
}

int CalcModFromStatVal(int statVal) { return (statVal - 10) / 2; }

std::string FormatSignedInt(int value)
{
    if (value >= 0)
    {
        return "+" + std::to_string(value);
    }
    return std::to_string(value);
}

std::vector<std::string> SplitIntoWords(const std::string& text)
{
    std::vector<std::string> words;
    std::string::size_type pos = 0;
    const std::string::size_type n = text.size();
    while (pos < n)
    {
        pos = text.find_first_not_of(" \t\n", pos);
        if (pos == std::string::npos) { break; }
        const std::string::size_type end = text.find_first_of(" \t\n", pos);
        if (end == std::string::npos)
        {
            words.push_back(text.substr(pos));
            break;
        }
        words.push_back(text.substr(pos, end - pos));
        pos = end;
    }
    return words;
}

std::vector<std::string> Split(const std::string& s, const std::string& delim)
{
    std::vector<std::string> out;
    if (delim.empty())
    {
        out.push_back(s);
        return out;
    }
    std::string::size_type pos = 0;
    for (;;)
    {
        const std::string::size_type found = s.find(delim, pos);
        if (found == std::string::npos)
        {
            out.push_back(s.substr(pos));
            break;
        }
        out.push_back(s.substr(pos, found - pos));
        pos = found + delim.size();
    }
    return out;
}

double CalcSpacingForClassname(const std::string& classname)
{
    if (classname == "cleric") return 3.5;
    if (classname == "bard") return 4.7;
    if (classname == "barbarian") return 0.7;
    if (classname == "sorcerer") return 0.9;
    if (classname == "paladin") return 1.1;
    return 1.5;
}

}
