#include "Settings.h"

#include <iostream>
#include "IniReader.h"

// Default values

const unsigned int Settings::KEY_CODE_ON_OFF = VK_F12;  // Default ON/OFF key
const CRGBA Settings::COLOR_TAG(85, 255, 85);           // 1442797055
const CRGBA Settings::COLOR_SNAPSHOT(255, 161, 208);    // -6172417
const CRGBA Settings::COLOR_HORSESHOE(147, 192, 238);   // -1816072449
const CRGBA Settings::COLOR_OYSTER(255, 255, 153);      // -26113
const CRGBA Settings::COLOR_USJ(204, 153, 255);         // -862322689
const CRGBA Settings::COLOR_USJ_FOUND(222, 136, 255);   // -561446913
const CRGBA Settings::COLOR_DEFAULT(255, 255, 255);

// [MAIN]

unsigned int Settings::s_keyCodeOnOff = Settings::KEY_CODE_ON_OFF;

bool Settings::s_drawTags = true;
bool Settings::s_drawSnapshots = true;
bool Settings::s_drawHorseshoes = true;
bool Settings::s_drawOysters = true;
bool Settings::s_drawUSJs = true;

bool Settings::s_drawNearest = true;

// [COLORS]

CRGBA Settings::s_colorTag(Settings::COLOR_TAG);
CRGBA Settings::s_colorSnapshot(Settings::COLOR_SNAPSHOT);
CRGBA Settings::s_colorHorseshoe(Settings::COLOR_HORSESHOE);
CRGBA Settings::s_colorOyster(Settings::COLOR_OYSTER);
CRGBA Settings::s_colorUSJ(Settings::COLOR_USJ);

// [EXTRA]

bool Settings::s_drawBribes = false;
bool Settings::s_drawArmours = false;
bool Settings::s_drawWeapons = false;

CRGBA Settings::s_colorBribe(Settings::COLOR_DEFAULT);
CRGBA Settings::s_colorArmour(Settings::COLOR_DEFAULT);
CRGBA Settings::s_colorWeapon(Settings::COLOR_DEFAULT);

void Settings::read() {
    CIniReader iniReader("");

    // [MAIN]

    s_keyCodeOnOff = static_cast<unsigned int>(iniReader.ReadInteger("MAIN", "on_off_key", static_cast<int>(KEY_CODE_ON_OFF)));

    s_drawTags = iniReader.ReadBoolean("MAIN", "show_tags", true);
    s_drawSnapshots = iniReader.ReadBoolean("MAIN", "show_snapshots", true);
    s_drawHorseshoes = iniReader.ReadBoolean("MAIN", "show_horseshoes", true);
    s_drawOysters = iniReader.ReadBoolean("MAIN", "show_oysters", true);
    s_drawUSJs = iniReader.ReadBoolean("MAIN", "show_usjs", true);

    s_drawNearest = iniReader.ReadBoolean("MAIN", "show_nearest", true);

    // [COLORS]

    s_colorTag.Set(static_cast<unsigned int>(iniReader.ReadInteger("COLORS", "color_tag", static_cast<int>(COLOR_TAG.ToInt()))));
    s_colorSnapshot.Set(static_cast<unsigned int>(iniReader.ReadInteger("COLORS", "color_snapshot", static_cast<int>(COLOR_SNAPSHOT.ToInt()))));
    s_colorHorseshoe.Set(static_cast<unsigned int>(iniReader.ReadInteger("COLORS", "color_horseshoe", static_cast<int>(COLOR_HORSESHOE.ToInt()))));
    s_colorOyster.Set(static_cast<unsigned int>(iniReader.ReadInteger("COLORS", "color_oyster", static_cast<int>(COLOR_OYSTER.ToInt()))));
    s_colorUSJ.Set(static_cast<unsigned int>(iniReader.ReadInteger("COLORS", "color_usj", static_cast<int>(COLOR_USJ.ToInt()))));

    // [EXTRA]

    s_drawBribes = iniReader.ReadBoolean("EXTRA", "show_bribes", false);
    s_drawArmours = iniReader.ReadBoolean("EXTRA", "show_armours", false);
    s_drawWeapons = iniReader.ReadBoolean("EXTRA", "show_weapons", false);

    s_colorBribe.Set(static_cast<unsigned int>(iniReader.ReadInteger("EXTRA", "color_bribe", static_cast<int>(COLOR_DEFAULT.ToInt()))));
    s_colorArmour.Set(static_cast<unsigned int>(iniReader.ReadInteger("EXTRA", "color_armour", static_cast<int>(COLOR_DEFAULT.ToInt()))));
    s_colorWeapon.Set(static_cast<unsigned int>(iniReader.ReadInteger("EXTRA", "color_weapon", static_cast<int>(COLOR_DEFAULT.ToInt()))));
}
