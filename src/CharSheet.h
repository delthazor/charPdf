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
              const UtilType::SpellsCatalog& spellsCatalog);
    ~CharSheet() = default;

  private:
    void FillSheet();
    void advanceSlot();
    template <typename PageT> void AddContent(PageParams params = {});

    Config config;
    UtilType::TraitsCatalog traitsCatalog;
    UtilType::SpellsCatalog spellsCatalog;
    PdfDoc doc;
    PageConstants::PageSide currentSlot;
    bool afterFirstPage;
};
