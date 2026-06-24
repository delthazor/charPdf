#pragma once

#include "pagetypes/PageBase.h"
#include "syshelpers/Config.h"
#include "syshelpers/PageConstants.h"
#include "syshelpers/PdfDoc.h"

class CharSheet
{
  public:
    CharSheet(const Config& config,
              const UtilType::TraitsCatalog& traitsCatalog,
              const UtilType::SpellsCatalog& spellsCatalog,
              const std::string& outputPath);
    ~CharSheet() = default;

  private:
    void FillSheet();
    void AddMainPage();
    void AddClassPages();
    void AddInventoryPage();
    void AddTraitPages();
    void AddSpellPages();
    void advanceSlot();
    template <typename PageT> void AddContent(PageParams params = {});

    Config config;
    UtilType::TraitsCatalog traitsCatalog;
    UtilType::SpellsCatalog spellsCatalog;
    PdfDoc doc;
    PageConstants::PageSide currentSlot;
    bool afterFirstPage;
};
