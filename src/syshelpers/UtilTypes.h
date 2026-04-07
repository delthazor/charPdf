#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "syshelpers/PageConstants.h"

namespace UtilType
{

struct TextOptions
{
    PageConstants::FontType fontType = PageConstants::FontType::Seagram;
    double fontSize = 12;
    double rotationAngle = 0;
    double letterSpacing = 0;

    TextOptions() = default;
    TextOptions(PageConstants::FontType fontType) : fontType(fontType) {}
    TextOptions(double fontSize) : fontSize(fontSize) {}
    TextOptions(PageConstants::FontType fontType, double fontSize) : fontType(fontType), fontSize(fontSize) {}
    TextOptions(PageConstants::FontType fontType, double fontSize, double rotationAngle, double letterSpacing)
        : fontType(fontType), fontSize(fontSize), rotationAngle(rotationAngle), letterSpacing(letterSpacing)
    {
    }
};

using MeasureFn = std::function<double(const std::string&, const TextOptions&)>;

struct LabeledTextBlock
{
    std::string label;
    std::string text;
    LabeledTextBlock(const std::string& label, const std::string& text) : label(label), text(text) {}
};

struct FormattedLabelPart
{
    std::string text;
    TextOptions textOptions;
    bool underline = false;
};

struct FormattedTextSpan
{
    std::string text;
    TextOptions textOptions;
    bool underline = false;
    bool forceLineBreakBefore = false;
};

struct FormattedLabeledBlock
{
    std::vector<FormattedLabelPart> labelParts;
    std::vector<FormattedTextSpan> textSpans;
};

struct StyledWord
{
    std::string text;
    TextOptions textOptions;
    bool underline = false;
    bool forceLineBreakBefore = false;
};

struct TraitsCatalog
{
    nlohmann::ordered_json raw;
    std::unordered_map<std::string, nlohmann::ordered_json> byName;
};

struct SpellsCatalog
{
    nlohmann::ordered_json raw;
    std::unordered_map<std::string, nlohmann::ordered_json> byName;
};

}
