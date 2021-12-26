#pragma once

#include <CRGBA.h>

class Settings
{
public:
    // Default values

    static const unsigned int KEY_CODE_ON_OFF;

    static const CRGBA COLOR_TAG;
    static const CRGBA COLOR_SNAPSHOT;
    static const CRGBA COLOR_HORSESHOE;
    static const CRGBA COLOR_OYSTER;
    static const CRGBA COLOR_USJ;
    static const CRGBA COLOR_USJ_FOUND;
    static const CRGBA COLOR_DEFAULT;

    // [MAIN]

    static unsigned int s_keyCodeOnOff;

    static bool s_drawTags;
    static bool s_drawSnapshots;
    static bool s_drawHorseshoes;
    static bool s_drawOysters;
    static bool s_drawUSJs;

    static bool s_drawNearest;

    // [COLORS]

    static CRGBA s_colorTag;
    static CRGBA s_colorSnapshot;
    static CRGBA s_colorHorseshoe;
    static CRGBA s_colorOyster;
    static CRGBA s_colorUSJ;

    // [EXTRA]

    static bool s_drawBribes;
    static bool s_drawArmours;
    static bool s_drawWeapons;

    static CRGBA s_colorBribe;
    static CRGBA s_colorArmour;
    static CRGBA s_colorWeapon;

public:
    static void read();
};
