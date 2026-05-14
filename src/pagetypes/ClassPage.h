#pragma once

#include <vector>

#include "pagetypes/PageBase.h"

class ClassPage : public PageBase
{
  public:
    ClassPage(PdfDoc& doc, const Config& config, PageSide side, const PageParams& params);

    void Draw() override;
    void Fill() override;

  private:
    void drawMarker(int level, int position);

    void AddClass();
    void AddSubclass();
    void AddLevel();
    void AddResourcePoints();
    void AddCastInfo();
    void AddSpellSlots();
    void AddSpellStats();
    void AddSpellNames();

    const Config classData;
    std::vector<std::vector<Coords>> const spellSlotMarker;
    std::vector<std::vector<Coords>> const resourcePointMarker;
    std::vector<Coords> const spellBoxMatrix;
};
