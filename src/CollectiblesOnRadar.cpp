#include "plugin.h"
#include "common.h"
#include "CHud.h" // CHud::SetHelpMessage
#include "CMenuManager.h" // FrontEndMenuManager.drawRadarOrMap
#include "CPickups.h"
#include "CRadar.h"
#include "CTimer.h" // CTimer::m_snTimeInMilliseconds

#include "Util.h"
#include "Settings.h"

class CollectiblesOnRadar
{
private:
    static bool s_modEnabled;
    static int s_keyPressTime;

public:
    CollectiblesOnRadar()
    {
        //AllocConsole();
        //FILE* f = new FILE;
        //freopen_s(&f, "CONOUT$", "w", stdout);

        plugin::Events::initRwEvent += []
        {
            Settings::read();
            s_modEnabled = Settings::s_enabledOnStartup;
        };

        plugin::Events::gameProcessEvent += []
        {
            if (plugin::KeyPressed(Settings::s_keyCodeOnOff) && CTimer::m_snTimeInMillisecondsPauseMode - s_keyPressTime > 200)
            {
                s_modEnabled = !s_modEnabled;
                CHud::SetHelpMessage(s_modEnabled ? "Collectibles on radar ON" : "Collectibles on radar OFF", true, false, false);
                s_keyPressTime = CTimer::m_snTimeInMillisecondsPauseMode;

                if (s_modEnabled) Settings::read();
            }
        };

        plugin::Events::drawBlipsEvent += []
        {
            CPlayerPed* playa = FindPlayerPed();
            if (s_modEnabled && playa)
            {
                const CVector& playaPos = FindPlayerCentreOfWorld_NoSniperShift(0);
                if (!FrontEndMenuManager.m_bDrawRadarOrMap) // radar
                {
                    if (playa->m_nAreaCode == 0)
                    {
                        drawRadarTags(playaPos);
                        drawRadarPickups(playaPos); // oysters, horseshoes and snapshots
                        drawRadarUSJs(playaPos);
                    }
                }
                else // map
                {
                    const bool showHeight = playa->m_nAreaCode == 0;
                    drawMapTags(playaPos, showHeight);
                    drawMapPickups(playaPos, showHeight); // oysters, horseshoes and snapshots
                    drawMapUSJs(playaPos, showHeight);
                }
            }
        };
    }

private:
    static void drawRadarTags(const CVector& playaPos)
    {
        if (!Settings::s_drawTags)
        {
            return;
        }

        static CVector2D radarSpace, screenSpace;

        bool drawTag = true;
        float nearestTag = 9999999.f;
        int indexTag = -1;

        for (int i = 0; i < Util::s_totalTags; i++)
        {
            if (Util::s_tagList[i].paint > 228) continue; // already painted

            if (Util::s_tagList[i].tag == nullptr) // DEBUG
            {
                continue;
            }

            const CVector& tagPos = Util::s_tagList[i].tag->pos;
            const float distance = DistanceBetweenPoints(CVector2D(playaPos), CVector2D(tagPos));
            if (distance < CRadar::m_radarRange)
            {
                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(tagPos));
                //CRadar::LimitRadarPoint(radarSpace); // inside range already // returns distance
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                int mode = RADAR_TRACE_LOW;
                if (tagPos.z - playaPos.z <= 2.0f)
                {
                    if (tagPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                    else mode = RADAR_TRACE_NORMAL;
                }

                drawTag = false;

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                    Settings::s_colorTag.r, Settings::s_colorTag.g, Settings::s_colorTag.b, Settings::s_colorTag.a, mode);
            }
            else
            {
                if (Settings::s_drawNearest)
                {
                    if (drawTag && distance < nearestTag)
                    {
                        nearestTag = distance;
                        indexTag = i;
                    }
                }
            }
        }

        if (!Settings::s_drawNearest)
        {
            return;
        }

        if (drawTag && indexTag >= 0)
        {
            if (Util::s_tagList[indexTag].tag == nullptr) // DEBUG
            {
                return;
            }

            const CVector& tagPos = Util::s_tagList[indexTag].tag->pos;

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(tagPos));
            CRadar::LimitRadarPoint(radarSpace);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

            int mode = RADAR_TRACE_LOW;
            if (tagPos.z - playaPos.z <= 2.0f)
            {
                if (tagPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                else mode = RADAR_TRACE_NORMAL;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                Settings::s_colorTag.r, Settings::s_colorTag.g, Settings::s_colorTag.b, Settings::s_colorTag.a, mode);
        }
    }

    static void drawRadarPickups(const CVector& playaPos)
    {
        static CVector2D radarSpace, screenSpace;

        bool drawOyster = true; // draw the nearest pickup outside range
        bool drawHorse = true;
        bool drawSnap = true;

        float nearestOyster = 9999999.f; // nearest pickup distance
        float nearestHorse = 9999999.f;
        float nearestSnap = 9999999.f;

        int indexOyster = -1; // index of the nearest pickup
        int indexHorse = -1;
        int indexSnap = -1;

        for (int i = 0; i < static_cast<int>(MAX_NUM_PICKUPS); i++)
        {
            const short model = CPickups::aPickUps[i].m_nModelIndex;
            const unsigned char type = CPickups::aPickUps[i].m_nPickupType;
            const bool disabled = static_cast<bool>(CPickups::aPickUps[i].m_nFlags.bDisabled);

            boolean pickupOnStreet = (type == PICKUP_ON_STREET || type == PICKUP_ON_STREET_SLOW);
            boolean pickupOnce = (type == PICKUP_ONCE || type == PICKUP_ONCE_TIMEOUT || type == PICKUP_ONCE_TIMEOUT_SLOW);

            if ((Settings::s_drawOysters && model == 953 && type == PICKUP_ONCE)
                || (Settings::s_drawHorseshoes && model == 954 && type == PICKUP_ONCE)
                || (Settings::s_drawSnapshots && model == 1253 && type == PICKUP_SNAPSHOT)
                || (!disabled
                    && (pickupOnStreet && model == 1247 && Settings::s_drawBribes
                        || pickupOnStreet && model == 1242 && Settings::s_drawArmours
                        || (pickupOnStreet || Settings::s_drawDroppedWeapons && pickupOnce) && (model >= 330 && model <= 372) && Settings::s_drawWeapons)))
            {
                const CVector pickupPos(CPickups::aPickUps[i].m_vecPos.Uncompressed());
                if (pickupPos.z >= 960.f)
                {
                    continue;
                }

                const float distance = DistanceBetweenPoints(CVector2D(playaPos), CVector2D(pickupPos));
                if (distance < CRadar::m_radarRange)
                {
                    CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
                    //CRadar::LimitRadarPoint(radarSpace); // inside range already // returns distance
                    CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                    int mode = RADAR_TRACE_LOW;
                    if (pickupPos.z - playaPos.z <= 2.0f)
                    {
                        if (pickupPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                        else mode = RADAR_TRACE_NORMAL;
                    }

                    CRGBA color; // = Settings::COLOR_DEFAULT
                    switch (model)
                    {
                    case 953:
                        color = Settings::s_colorOyster;
                        drawOyster = false;
                        break;
                    case 954:
                        color = Settings::s_colorHorseshoe;
                        drawHorse = false;
                        break;
                    case 1253:
                        color = Settings::s_colorSnapshot;
                        drawSnap = false;
                        break;
                    case 1247:
                        color = Settings::s_colorBribe;
                        break;
                    case 1242:
                        color = Settings::s_colorArmour;
                        break;
                    default:
                        color = Settings::s_colorWeapon;
                    }

                    CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color.r, color.g, color.b, color.a, mode);
                }
                else
                {
                    if (Settings::s_drawNearest)
                    {
                        switch (model)
                        {
                        case 953: // oyster
                            if (drawOyster && distance < nearestOyster)
                            {
                                nearestOyster = distance;
                                indexOyster = i;
                            }
                            break;
                        case 954: // horse
                            if (drawHorse && distance < nearestHorse)
                            {
                                nearestHorse = distance;
                                indexHorse = i;
                            }
                            break;
                        case 1253: // snap
                            if (drawSnap && distance < nearestSnap)
                            {
                                nearestSnap = distance;
                                indexSnap = i;
                            }
                            break;
                        }
                    }
                }
            }
        }

        if (!Settings::s_drawNearest)
        {
            return;
        }

        if (drawOyster && indexOyster >= 0)
        {
            const CVector pickupPos(CPickups::aPickUps[indexOyster].m_vecPos.Uncompressed());

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
            CRadar::LimitRadarPoint(radarSpace);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

            int mode = RADAR_TRACE_LOW;
            if (pickupPos.z - playaPos.z <= 2.0f)
            {
                if (pickupPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                else mode = RADAR_TRACE_NORMAL;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                Settings::s_colorOyster.r, Settings::s_colorOyster.g, Settings::s_colorOyster.b, Settings::s_colorOyster.a, mode);
        }

        if (drawHorse && indexHorse >= 0)
        {
            const CVector pickupPos(CPickups::aPickUps[indexHorse].m_vecPos.Uncompressed());

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
            CRadar::LimitRadarPoint(radarSpace);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

            int mode = RADAR_TRACE_LOW;
            if (pickupPos.z - playaPos.z <= 2.0f)
            {
                if (pickupPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                else mode = RADAR_TRACE_NORMAL;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                Settings::s_colorHorseshoe.r, Settings::s_colorHorseshoe.g, Settings::s_colorHorseshoe.b, Settings::s_colorHorseshoe.a, mode);
        }

        if (drawSnap && indexSnap >= 0)
        {
            const CVector pickupPos(CPickups::aPickUps[indexSnap].m_vecPos.Uncompressed());

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
            CRadar::LimitRadarPoint(radarSpace);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

            int mode = RADAR_TRACE_LOW;
            if (pickupPos.z - playaPos.z <= 2.0f)
            {
                if (pickupPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                else mode = RADAR_TRACE_NORMAL;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                Settings::s_colorSnapshot.r, Settings::s_colorSnapshot.g, Settings::s_colorSnapshot.b, Settings::s_colorSnapshot.a, mode);
        }
    }

    static void drawRadarUSJs(const CVector& playaPos)
    {
        if (!Settings::s_drawUSJs)
        {
            return;
        }

        static CVector2D radarSpace, screenSpace;

        bool drawUsj = true;
        float nearestUsj = 9999999.f;
        int indexUsj = -1;

        for (int i = 0; i < Util::ms_pUsjPool->m_nSize; i++)
        {
            const tUsj& usj = Util::ms_pUsjPool->m_pObjects[i];

            if (!Util::ms_pUsjPool->m_byteMap[i].bEmpty && !usj.done)
            {
                const CVector usjPos(
                    (usj.start1.x + usj.start2.x) / 2.f,
                    (usj.start1.y + usj.start2.y) / 2.f,
                    usj.start1.z);
                const float distance = DistanceBetweenPoints(CVector2D(playaPos), CVector2D(usjPos));
                if (distance < CRadar::m_radarRange)
                {
                    CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(usjPos));
                    //CRadar::LimitRadarPoint(radarSpace); // inside range already // returns distance
                    CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                    int mode = RADAR_TRACE_LOW;
                    if (usjPos.z - playaPos.z <= 2.0f)
                    {
                        if (usjPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                        else mode = RADAR_TRACE_NORMAL;
                    }

                    drawUsj = false;

                    CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, 
                        Settings::s_colorUSJ.r, Settings::s_colorUSJ.g, Settings::s_colorUSJ.b, Settings::s_colorUSJ.a, mode);
                }
                else
                {
                    if (Settings::s_drawNearest)
                    {
                        if (drawUsj && distance < nearestUsj)
                        {
                            nearestUsj = distance;
                            indexUsj = i;
                        }
                    }
                }
            }
        }

        if (!Settings::s_drawNearest)
        {
            return;
        }

        if (drawUsj && indexUsj >= 0)
        {
            const tUsj& usj = Util::ms_pUsjPool->m_pObjects[indexUsj];

            const CVector usjPos(
                (usj.start1.x + usj.start2.x) / 2.f,
                (usj.start1.y + usj.start2.y) / 2.f,
                usj.start1.z);

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(usjPos));
            CRadar::LimitRadarPoint(radarSpace);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

            int mode = RADAR_TRACE_LOW;
            if (usjPos.z - playaPos.z <= 2.0f)
            {
                if (usjPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                else mode = RADAR_TRACE_NORMAL;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                Settings::s_colorUSJ.r, Settings::s_colorUSJ.g, Settings::s_colorUSJ.b, Settings::s_colorUSJ.a, mode);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    static void drawMapTags(const CVector& playaPos, bool showHeight)
    {
        if (!Settings::s_drawTags)
        {
            return;
        }

        static CVector2D radarSpace, screenSpace;

        for (int i = 0; i < Util::s_totalTags; i++)
        {
            if (Util::s_tagList[i].tag == nullptr) // DEBUG
            {
                continue;
            }

            if (Util::s_tagList[i].paint > 228) continue; // continue if tag already sprayed

            const CVector& tagPos = Util::s_tagList[i].tag->pos;

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(tagPos));
            //CRadar::LimitRadarPoint(radarSpace);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

            int mode = RADAR_TRACE_LOW;
            if (!showHeight)
            {
                mode = RADAR_TRACE_NORMAL;
            }
            else if (tagPos.z - playaPos.z <= 2.0f)
            {
                if (tagPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                else mode = RADAR_TRACE_NORMAL;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                Settings::s_colorTag.r, Settings::s_colorTag.g, Settings::s_colorTag.b, Settings::s_colorTag.a, mode);
        }
    }

    static void drawMapPickups(const CVector& playaPos, bool showHeight)
    {
        static CVector2D radarSpace, screenSpace;

        for (int i = 0; i < static_cast<int>(MAX_NUM_PICKUPS); i++)
        {
            const short model = CPickups::aPickUps[i].m_nModelIndex;
            const unsigned char type = CPickups::aPickUps[i].m_nPickupType;
            const bool disabled = static_cast<bool>(CPickups::aPickUps[i].m_nFlags.bDisabled);

            boolean pickupOnStreet = (type == PICKUP_ON_STREET || type == PICKUP_ON_STREET_SLOW);
            boolean pickupOnce = (type == PICKUP_ONCE || type == PICKUP_ONCE_TIMEOUT || type == PICKUP_ONCE_TIMEOUT_SLOW);

            if ((Settings::s_drawOysters && model == 953 && type == PICKUP_ONCE)
                || (Settings::s_drawHorseshoes && model == 954 && type == PICKUP_ONCE)
                || (Settings::s_drawSnapshots && model == 1253 && type == PICKUP_SNAPSHOT)
                || (!disabled
                    && (pickupOnStreet && model == 1247 && Settings::s_drawBribes
                        || pickupOnStreet && model == 1242 && Settings::s_drawArmours
                        || (pickupOnStreet || Settings::s_drawDroppedWeapons && pickupOnce) && (model >= 330 && model <= 372) && Settings::s_drawWeapons)))
            {
                const CVector pickupPos(CPickups::aPickUps[i].m_vecPos.Uncompressed());
                if (pickupPos.z >= 960.f)
                    continue;

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
                //CRadar::LimitRadarPoint(radarSpace);
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                int mode = RADAR_TRACE_LOW;
                if (!showHeight)
                {
                    mode = RADAR_TRACE_NORMAL;
                }
                else if (pickupPos.z - playaPos.z <= 2.0f)
                {
                    if (pickupPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                    else mode = RADAR_TRACE_NORMAL;
                }

                CRGBA color;
                switch (model)
                {
                case 953:
                    color = Settings::s_colorOyster;
                    break;
                case 954:
                    color = Settings::s_colorHorseshoe;
                    break;
                case 1253:
                    color = Settings::s_colorSnapshot;
                    break;
                case 1247:
                    color = Settings::s_colorBribe;
                    break;
                case 1242:
                    color = Settings::s_colorArmour;
                    break;
                default:
                    color = Settings::s_colorWeapon;
                }

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color.r, color.g, color.b, color.a, mode);
            }
        }
    }

    static void drawMapUSJs(const CVector& playaPos, bool showHeight)
    {
        if (!Settings::s_drawUSJs)
        {
            return;
        }

        static CVector2D radarSpace, screenSpace;

        for (int i = 0; i < Util::ms_pUsjPool->m_nSize; i++)
        {
            const tUsj& usj = Util::ms_pUsjPool->m_pObjects[i];

            if (!Util::ms_pUsjPool->m_byteMap[i].bEmpty && !usj.done)
            {
                const CVector usjPos(
                    (usj.start1.x + usj.start2.x) / 2.f,
                    (usj.start1.y + usj.start2.y) / 2.f,
                    usj.start1.z);

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(usjPos));
                //CRadar::LimitRadarPoint(radarSpace);
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                int mode = RADAR_TRACE_LOW;
                if (!showHeight)
                {
                    mode = RADAR_TRACE_NORMAL;
                }
                else if (usjPos.z - playaPos.z <= 2.0f)
                {
                    if (usjPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                    else mode = RADAR_TRACE_NORMAL;
                }

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1,
                    Settings::s_colorUSJ.r, Settings::s_colorUSJ.g, Settings::s_colorUSJ.b, Settings::s_colorUSJ.a, mode);
            }
        }
    }
} collectiblesOnRadar;

bool CollectiblesOnRadar::s_modEnabled = true;
int CollectiblesOnRadar::s_keyPressTime = 0;
