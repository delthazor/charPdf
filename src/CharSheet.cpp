#include <vector>

#include "CharSheet.h"
#include "pagetypes/ClassPage.h"
#include "pagetypes/InventoryPage.h"
#include "pagetypes/MainPage.h"
#include "pagetypes/SpellPage.h"
#include "pagetypes/TraitPage.h"
#include "syshelpers/Utilities.h"

using namespace PageConstants;

CharSheet::CharSheet(const Config& cfg,
                     const UtilType::TraitsCatalog& traitsCat,
                     const UtilType::SpellsCatalog& spellsCat)
    : config(cfg), traitsCatalog(traitsCat), spellsCatalog(spellsCat),
      doc(Utilities::SanitizePdfStem(config.getString("name")) + ".pdf",
          A4_LANDSCAPE_WIDTH,
          A4_LANDSCAPE_HEIGHT),
      currentSlot(PageSide::LEFT_SIDE), afterFirstPage(false)
{
    FillSheet();
}

void CharSheet::advanceSlot()
{
    if (currentSlot == PageSide::RIGHT_SIDE) { afterFirstPage = true; }
    currentSlot = (currentSlot == PageSide::LEFT_SIDE) ? PageSide::RIGHT_SIDE : PageSide::LEFT_SIDE;
}

template <typename PageT> void CharSheet::AddContent(PageParams params)
{
    if (currentSlot == PageSide::LEFT_SIDE && afterFirstPage)
    {
        doc.CreateNewPage();
        afterFirstPage = false;
    }
    PageT page(doc, config, currentSlot, params);
    page.Draw();
    page.Fill();
    advanceSlot();
}

void CharSheet::AddMainPage()
{
    Utilities::LogInfo("Add Main page");
    AddContent<MainPage>();
}

void CharSheet::AddClassPages()
{
    if (config.hasKey("classes"))
    {
        for (const auto& classId : config.getObject("classes").getKeys())
        {
            Utilities::LogInfo("Add Class page: " + classId);
            AddContent<ClassPage>({{"classname", classId}});
        }
    }
    else
    {
        Utilities::LogError("No classes found");
    }
}

void CharSheet::AddInventoryPage()
{
    Utilities::LogInfo("Add Inventory page");
    AddContent<InventoryPage>();
}

void CharSheet::AddTraitPages()
{
    if (config.hasKey("traits"))
    {
        const std::string fullText = Utilities::BuildFullTraitsText(config, traitsCatalog);
        if (!fullText.empty())
        {
            const std::vector<std::string> pageChunks = Utilities::SplitDescriptionTextIntoPages(fullText);
            Utilities::LogInfo("Adding traits, split into " + std::to_string(pageChunks.size()) + " pages");
            for (const std::string& chunk : pageChunks)
            {
                AddContent<TraitPage>({{"traitPageText", chunk}});
            }
        }
        else
        {
            Utilities::LogError("No valid traits found, trait block is empty");
        }
    }
    else
    {
        Utilities::LogError("No traits found");
    }
}

void CharSheet::AddSpellPages()
{
    if (config.hasKey("classes"))
    {
        const std::string fullSpellText = Utilities::BuildFullSpellsText(config, spellsCatalog);
        if (!fullSpellText.empty())
        {
            const std::vector<std::string> spellChunks =
                Utilities::SplitDescriptionTextIntoPages(fullSpellText);
            Utilities::LogInfo("Adding spells, split into " + std::to_string(spellChunks.size()) + " pages");
            for (const std::string& chunk : spellChunks)
            {
                AddContent<SpellPage>({{"spellPageText", chunk}});
            }
        }
        else
        {
            Utilities::LogError("No valid spells found, spell block is empty");
        }
    }
    else
    {
        Utilities::LogError("No spells found");
    }
}

void CharSheet::FillSheet()
{
    AddMainPage();
    AddClassPages();
    AddInventoryPage();
    AddTraitPages();
    AddSpellPages();
}
