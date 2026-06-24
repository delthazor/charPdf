#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "CharSheet.h"
#include "syshelpers/Config.h"
#include "syshelpers/Utilities.h"

namespace fs = std::filesystem;

namespace
{

constexpr const char* kCharsRoot = "assets/cfg/chars";

std::string MakeProcessingBanner(std::string_view name)
{
    const size_t dashNum = 40 - name.size();
    return std::string(dashNum, '-') + " " + std::string(name) + " " + std::string(dashNum, '-');
}

bool IsCharJsonFilename(const std::string& name)
{
    if (name.size() < 10) { return false; }
    if (name.rfind("char_", 0) != 0) { return false; }
    if (name.substr(name.size() - 5) != ".json") { return false; }
    return true;
}

struct RunMode
{
    enum class Kind
    {
        All,
        Single
    };

    Kind kind = Kind::All;
    std::string campaign;
};

RunMode ParseRunMode(int argc, char* argv[])
{
    if (argc <= 1) { return {}; }

    if (argc > 2)
    {
        throw std::runtime_error("Usage: pdf_app [all|<campaign>]");
    }

    const std::string arg = argv[1];
    if (arg == "all") { return {}; }

    RunMode mode;
    mode.kind = RunMode::Kind::Single;
    mode.campaign = arg;
    return mode;
}

std::vector<fs::path> CollectCampaignDirs(const RunMode& mode)
{
    const fs::path charsRoot(kCharsRoot);
    if (!fs::exists(charsRoot) || !fs::is_directory(charsRoot))
    {
        throw std::runtime_error(std::string("Character directory not found: ") + kCharsRoot);
    }

    if (mode.kind == RunMode::Kind::Single)
    {
        const fs::path campaignDir = charsRoot / mode.campaign;
        if (!fs::exists(campaignDir) || !fs::is_directory(campaignDir))
        {
            throw std::runtime_error(std::string("Campaign folder not found: ") + campaignDir.string());
        }
        return {campaignDir};
    }

    std::vector<fs::path> campaigns;
    for (const auto& entry : fs::directory_iterator(charsRoot))
    {
        if (entry.is_directory()) { campaigns.push_back(entry.path()); }
    }

    std::sort(campaigns.begin(), campaigns.end());
    return campaigns;
}

void WarnLooseCharFilesInCharsRoot()
{
    const fs::path charsRoot(kCharsRoot);
    for (const auto& entry : fs::directory_iterator(charsRoot))
    {
        if (!entry.is_regular_file()) { continue; }
        const std::string filename = entry.path().filename().string();
        if (IsCharJsonFilename(filename))
        {
            Utilities::LogError(std::string("Ignoring character file not in a campaign folder: ")
                                + entry.path().string());
        }
    }
}

void ProcessCampaignDir(const fs::path& campaignDir,
                        const UtilType::TraitsCatalog& traitsCatalog,
                        const UtilType::SpellsCatalog& spellsCatalog)
{
    const std::string campaign = campaignDir.filename().string();
    bool foundAny = false;

    for (const auto& entry : fs::directory_iterator(campaignDir))
    {
        if (!entry.is_regular_file()) { continue; }

        const std::string filename = entry.path().filename().string();
        if (!IsCharJsonFilename(filename)) { continue; }

        foundAny = true;
        const Config config(Utilities::LoadJsonFromFile(entry.path().string()));

        const std::string name = config.getString("name");
        Utilities::LogInfo(MakeProcessingBanner(name));
        Utilities::LogInfo(std::string("Processing: ") + name);

        const std::string outputPath =
            std::string("chars/") + campaign + "/" + Utilities::SanitizePdfStem(name) + ".pdf";
        const fs::path outputFs(outputPath);
        fs::create_directories(outputFs.parent_path());

        CharSheet sheet(config, traitsCatalog, spellsCatalog, outputPath);

        Utilities::LogInfo(std::string("PDF created: ") + outputPath);
    }

    if (!foundAny)
    {
        Utilities::LogInfo(std::string("No character files in campaign folder: ") + campaign);
    }
}

void WarnLegacyRootCharFiles()
{
    const fs::path cfgRoot("assets/cfg");
    for (const auto& entry : fs::directory_iterator(cfgRoot))
    {
        if (!entry.is_regular_file()) { continue; }
        const std::string filename = entry.path().filename().string();
        if (IsCharJsonFilename(filename))
        {
            Utilities::LogError(std::string("Ignoring legacy character file at cfg root: ")
                                + entry.path().string());
        }
    }
}

}

static UtilType::TraitsCatalog LoadTraitsCatalog()
{
    Utilities::LogInfo("Reading traits config...");
    return Utilities::BuildTraitsCatalog(Utilities::LoadJsonFromFile("assets/cfg/config_traits.json"));
}

static UtilType::SpellsCatalog LoadSpellsCatalog()
{
    Utilities::LogInfo("Reading spells config...");
    return Utilities::BuildSpellsCatalog(Utilities::LoadJsonFromFile("assets/cfg/config_spells.json"));
}

static int CreatePDFs(const RunMode& mode,
                      const UtilType::TraitsCatalog& traitsCatalog,
                      const UtilType::SpellsCatalog& spellsCatalog)
{
    Utilities::LogInfo("Creating PDFs from config files...");
    WarnLegacyRootCharFiles();
    WarnLooseCharFilesInCharsRoot();

    const std::vector<fs::path> campaigns = CollectCampaignDirs(mode);
    for (const fs::path& campaignDir : campaigns)
    {
        ProcessCampaignDir(campaignDir, traitsCatalog, spellsCatalog);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    try
    {
        const RunMode mode = ParseRunMode(argc, argv);
        const UtilType::TraitsCatalog traitsCatalog = LoadTraitsCatalog();
        const UtilType::SpellsCatalog spellsCatalog = LoadSpellsCatalog();
        const int rc = CreatePDFs(mode, traitsCatalog, spellsCatalog);
        if (rc != 0) { return rc; }
        Utilities::LogInfo("All PDFs created successfully!");
        return 0;
    }
    catch (const std::exception& e)
    {
        Utilities::LogError(std::string("Error: ") + e.what());
        return 1;
    }
}
