#include "Settings.h"

#include "IniReader.h"

const std::string Settings::MAIN("MAIN");
const std::string Settings::COLORS("COLORS");
const std::string Settings::EXTRA("EXTRA");

// Default values

const unsigned int Settings::KEY_CODE_ON_OFF = VK_F12; // default ON/OFF key
const CRGBA Settings::COLOR_TAG(85, 255, 85);
const CRGBA Settings::COLOR_SNAPSHOT(255, 161, 208);
const CRGBA Settings::COLOR_HORSESHOE(147, 192, 238);
const CRGBA Settings::COLOR_OYSTER(255, 255, 153);
const CRGBA Settings::COLOR_USJ(204, 153, 255);
const CRGBA Settings::COLOR_USJ_FOUND(222, 136, 255);
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
bool Settings::s_enabledOnStartup = true;
bool Settings::s_drawDroppedWeapons = false;

void Settings::read() {
    CIniReader iniReader("");

    // [MAIN]

    s_keyCodeOnOff = static_cast<unsigned int>(iniReader.ReadInteger(MAIN, "on_off_key", static_cast<int>(KEY_CODE_ON_OFF)));

    s_drawTags = iniReader.ReadBoolean(MAIN, "show_tags", true);
    s_drawSnapshots = iniReader.ReadBoolean(MAIN, "show_snapshots", true);
    s_drawHorseshoes = iniReader.ReadBoolean(MAIN, "show_horseshoes", true);
    s_drawOysters = iniReader.ReadBoolean(MAIN, "show_oysters", true);
    s_drawUSJs = iniReader.ReadBoolean(MAIN, "show_usjs", true);

    s_drawNearest = iniReader.ReadBoolean(MAIN, "show_nearest", true);

    // [COLORS]

    s_colorTag.Set(toRGBA(iniReader.ReadString(COLORS, "color_tag", ""), COLOR_TAG.ToInt()));
    s_colorSnapshot.Set(toRGBA(iniReader.ReadString(COLORS, "color_snapshot", ""), COLOR_SNAPSHOT.ToInt()));
    s_colorHorseshoe.Set(toRGBA(iniReader.ReadString(COLORS, "color_horseshoe", ""), COLOR_HORSESHOE.ToInt()));
    s_colorOyster.Set(toRGBA(iniReader.ReadString(COLORS, "color_oyster", ""), COLOR_OYSTER.ToInt()));
    s_colorUSJ.Set(toRGBA(iniReader.ReadString(COLORS, "color_usj", ""), COLOR_USJ.ToInt()));

    // [EXTRA]

    s_drawBribes = iniReader.ReadBoolean(EXTRA, "show_bribes", false);
    s_drawArmours = iniReader.ReadBoolean(EXTRA, "show_armours", false);
    s_drawWeapons = iniReader.ReadBoolean(EXTRA, "show_weapons", false);
    
    s_colorBribe.Set(toRGBA(iniReader.ReadString(EXTRA, "color_bribe", ""), COLOR_DEFAULT.ToInt()));
    s_colorArmour.Set(toRGBA(iniReader.ReadString(EXTRA, "color_armour", ""), COLOR_DEFAULT.ToInt()));
    s_colorWeapon.Set(toRGBA(iniReader.ReadString(EXTRA, "color_weapon", ""), COLOR_DEFAULT.ToInt()));

    s_enabledOnStartup = iniReader.ReadBoolean(EXTRA, "enabled_on_startup", true);
    s_drawDroppedWeapons = iniReader.ReadBoolean(EXTRA, "draw_dropped_weapons", false);
}

unsigned int Settings::toRGBA(const std::string& str, unsigned int defaultValue)
{
    //static const std::regex patternRGBA("^#[A-Fa-f0-9]{6,8}$");

    // if (std::regex_match(str, patternRGBA))
    try
    {
        char color[9] = "ffffffff";
        str.copy(color, str.size() - 1, 1);
        return static_cast<unsigned int>(std::stoul(color, nullptr, 16));
    }
    catch (...)
    {}

    return defaultValue;
}
