#pragma once

#include <nlohmann/json.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "syshelpers/Config.h"
#include "syshelpers/UtilTypes.h"

namespace Utilities
{

void LogInfo(std::string_view message);
void LogError(std::string_view message);

inline constexpr double CalculateLineHeight(double fontSize) { return fontSize * 1.2; }

double RemainingLineWidth(std::span<const double> lineWidthView, size_t lineIdx);

void EnsureLineLimit(size_t lineIndex);

UtilType::TraitsCatalog BuildTraitsCatalog(nlohmann::ordered_json arr);
UtilType::SpellsCatalog BuildSpellsCatalog(nlohmann::ordered_json arr);

std::vector<std::string> Split(const std::string& s, const std::string& delim);
std::vector<std::string> SplitIntoWords(const std::string& text);
std::string CapitalizeFirst(const std::string& s);
std::string SanitizePdfStem(std::string stem);
int CalcModFromStatVal(int statVal);
double CalcSpacingForClassname(const std::string& classname);

nlohmann::ordered_json LoadJsonFromFile(const std::string& path);

std::string BuildFullTraitsText(const Config& characterConfig, const UtilType::TraitsCatalog& traitsCatalog);
std::string BuildFullSpellsText(const Config& characterConfig, const UtilType::SpellsCatalog& spellsCatalog);
std::vector<std::string> SplitDescriptionTextIntoPages(const std::string& fullText);

std::string CollectPlainLine(const std::vector<std::string>& words,
                             size_t& wordIndex,
                             double maxWidth,
                             const UtilType::TextOptions& valueOpts,
                             const UtilType::MeasureFn& measure);

std::vector<UtilType::StyledWord> CollectStyledLine(const std::vector<UtilType::StyledWord>& words,
                                                    size_t& wordIndex,
                                                    double maxWidth,
                                                    const UtilType::MeasureFn& measure);
}
