#pragma once

#include <cstddef>

struct Coords
{
    double x;
    double y;
    constexpr Coords(double x, double y) : x(x), y(y) {}
};

namespace PageConstants
{

inline constexpr size_t TEXTWRAP_LINE_LIMIT = 1000;

enum class PageSide
{
    LEFT_SIDE,
    RIGHT_SIDE
};

enum class FontType
{
    Seagram,
    Arial,
    ArialBold,
    ArialItalic,
    TimesNewRoman,
    TimesNewRomanBold,
    TimesNewRomanItalic,
};

constexpr double A4_LANDSCAPE_WIDTH = 842;
constexpr double A4_LANDSCAPE_HEIGHT = 595;
constexpr double MARGIN = 0.75;
constexpr double MIDDLE_LINE = A4_LANDSCAPE_WIDTH / 2;
constexpr double HALF_PAGE_WIDTH = MIDDLE_LINE - (2 * MARGIN);

constexpr double CORNER_WIDTH = 125;
constexpr double CORNER_HEIGHT = 156;
constexpr double CORNER_SPACING = 0.1;

constexpr double DESCRIPTION_PAGE_BOX_INSET_LEFT = 24;
constexpr double DESCRIPTION_PAGE_BOX_INSET_RIGHT = 24;
constexpr double DESCRIPTION_PAGE_BOX_INSET_TOP = 33;
constexpr double DESCRIPTION_PAGE_BOX_INSET_BOTTOM = 16;
constexpr double DESCRIPTION_PAGE_BOX_WIDTH =
    HALF_PAGE_WIDTH - DESCRIPTION_PAGE_BOX_INSET_LEFT - DESCRIPTION_PAGE_BOX_INSET_RIGHT;
constexpr double DESCRIPTION_PAGE_BOX_HEIGHT =
    A4_LANDSCAPE_HEIGHT - DESCRIPTION_PAGE_BOX_INSET_TOP - DESCRIPTION_PAGE_BOX_INSET_BOTTOM;
constexpr double DESCRIPTION_PAGE_CONTENT_MARGIN = MARGIN;
constexpr double DESCRIPTION_PAGE_TEXT_WIDTH =
    DESCRIPTION_PAGE_BOX_WIDTH - 2 * DESCRIPTION_PAGE_CONTENT_MARGIN;
constexpr double DESCRIPTION_PAGE_TEXT_USABLE_HEIGHT =
    DESCRIPTION_PAGE_BOX_HEIGHT - 2 * DESCRIPTION_PAGE_CONTENT_MARGIN;
constexpr double TEXTBOX_BLOCK_SPACING = 6.0;
constexpr double DESCRIPTION_PAGE_FONT_SIZE = 10.0;
constexpr double MAIN_PAGE_PROFICIENCIES_FONT_SIZE = 9.0;
constexpr double DESCRIPTION_PAGE_EST_CHAR_WIDTH_RATIO = 0.48;
constexpr double DESCRIPTION_PAGE_EST_CHAR_WIDTH =
    DESCRIPTION_PAGE_FONT_SIZE * DESCRIPTION_PAGE_EST_CHAR_WIDTH_RATIO;

inline constexpr Coords STAT_VAL_COORDS[6] = {
    Coords(MIDDLE_LINE / 2 - 158, 206), // STR
    Coords(MIDDLE_LINE / 2 - 108, 157), // DEX
    Coords(MIDDLE_LINE / 2 - 26, 138),  // CON
    Coords(MIDDLE_LINE / 2 + 28, 138),  // INT
    Coords(MIDDLE_LINE / 2 + 82, 188),  // WIS
    Coords(MIDDLE_LINE / 2 + 145, 216)  // CHA
};

inline constexpr Coords STAT_MOD_COORDS[6] = {
    Coords(MIDDLE_LINE / 2 - 186, 210), // STR
    Coords(MIDDLE_LINE / 2 - 136, 166), // DEX
    Coords(MIDDLE_LINE / 2 - 56, 138),  // CON
    Coords(MIDDLE_LINE / 2 + 50, 138),  // INT
    Coords(MIDDLE_LINE / 2 + 105, 179), // WIS
    Coords(MIDDLE_LINE / 2 + 168, 216)  // CHA
};
}
