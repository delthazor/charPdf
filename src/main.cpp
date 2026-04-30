#include <filesystem>
#include <string>
#include <string_view>

#include "CharSheet.h"
#include "syshelpers/Config.h"
#include "syshelpers/Utilities.h"

namespace fs = std::filesystem;

namespace
{

std::string MakeProcessingBanner(std::string_view name)
{
    const size_t dashNum = 40 - name.size();
    return std::string(dashNum, '-') + " " + std::string(name) + " " + std::string(dashNum, '-');
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

static void CreatePDFs(const UtilType::TraitsCatalog& traitsCatalog,
                       const UtilType::SpellsCatalog& spellsCatalog)
{
    Utilities::LogInfo("Creating PDFs from config files...");
    for (const auto& entry : fs::directory_iterator("assets/cfg"))
    {
        const std::string filename = entry.path().filename().string();

        if (entry.path().extension() == ".json" && filename.substr(0, 5) == "char_")
        {
            const Config config(Utilities::LoadJsonFromFile(entry.path().string()));

            const std::string name = config.getString("name");
            Utilities::LogInfo(MakeProcessingBanner(name));
            Utilities::LogInfo(std::string("Processing: ") + name);

            CharSheet sheet(config, traitsCatalog, spellsCatalog);

            Utilities::LogInfo(std::string("PDF created: ") + Utilities::SanitizePdfStem(name) + ".pdf");
        }
    }
}

int main()
{
    try
    {
        const UtilType::TraitsCatalog traitsCatalog = LoadTraitsCatalog();
        const UtilType::SpellsCatalog spellsCatalog = LoadSpellsCatalog();
        CreatePDFs(traitsCatalog, spellsCatalog);
        Utilities::LogInfo("All PDFs created successfully!");
        return 0;
    }
    catch (const std::exception& e)
    {
        Utilities::LogError(std::string("Error: ") + e.what());
        return 1;
    }
}
