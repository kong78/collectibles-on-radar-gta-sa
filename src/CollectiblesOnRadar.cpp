#include "plugin.h"
#include "common.h"
#include "CHud.h" // CHud::SetHelpMessage
#include "CMenuManager.h" // FrontEndMenuManager.drawRadarOrMap
#include "CPickups.h"
#include "CRadar.h"
#include "CTimer.h" // CTimer::m_snTimeInMilliseconds

#include "Util.h"

// TODO:
// INI file: trace colors, on/off key, show tags not completely painted
// 228 = 1.f alpha
// ...
// 255 = 0.f alpha // don't draw

class CollectiblesOnRadar
{
private:
    static const CRGBA COLOR_TAG;
    static const CRGBA COLOR_USJ_UNDONE;
    static const CRGBA COLOR_USJ_FOUND;
    static const CRGBA COLOR_OYSTER;
    static const CRGBA COLOR_HORSE;
    static const CRGBA COLOR_SNAP;

    static bool s_modEnabled;
    static int s_keyPressTime;

public:
    CollectiblesOnRadar()
    {
        //AllocConsole();
        //FILE* f = new FILE;
        //freopen_s(&f, "CONOUT$", "w", stdout);

        plugin::Events::gameProcessEvent += []
        {
            if (plugin::KeyPressed(VK_F12) && CTimer::m_snTimeInMilliseconds - s_keyPressTime > 200)
            {
                s_modEnabled = !s_modEnabled;
                CHud::SetHelpMessage(s_modEnabled ? "Collectibles on radar ON" : "Collectibles on radar OFF", true, false, false);
                s_keyPressTime = CTimer::m_snTimeInMilliseconds;
            }
        };

        plugin::Events::drawBlipsEvent += []
        {
            CPlayerPed* playa = FindPlayerPed();
            if (s_modEnabled && playa)
            {
                const CVector& playaPos = FindPlayerCentreOfWorld_NoSniperShift(0);
                if (!FrontEndMenuManager.drawRadarOrMap) // radar
                {
                    if (playa->m_nAreaCode == 0)
                    {
                        drawRadarTags(playaPos);
                        drawRadarPickups(playaPos); // oysters, horseshoes and snapshots
                        drawRadarUSJ(playaPos);
                    }
                }
                else // map
                {
                    const bool showHeight = playa->m_nAreaCode == 0;
                    drawMapTags(playaPos, showHeight);
                    drawMapPickups(playaPos, showHeight); // oysters, horseshoes and snapshots
                    drawMapUSJ(playaPos, showHeight);
                }
            }
        };
    }

private:
    static void drawRadarTags(const CVector& playaPos)
    {
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
                //CRadar::LimitRadarPoint(radarSpace); // inside range anyway (returns distance)
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                int mode = RADAR_TRACE_LOW;
                if (tagPos.z - playaPos.z <= 2.0f)
                {
                    if (tagPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                    else mode = RADAR_TRACE_NORMAL;
                }

                drawTag = false;

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, COLOR_TAG.r, COLOR_TAG.g, COLOR_TAG.b, COLOR_TAG.a, mode);
            }
            else
            {
                if (drawTag && distance < nearestTag)
                {
                    nearestTag = distance;
                    indexTag = i;
                }
            }
        }

        if (drawTag && indexTag >= 0) // if there's no tag in range and found a nearest tag, draw it
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, COLOR_TAG.r, COLOR_TAG.g, COLOR_TAG.b, COLOR_TAG.a, mode);
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
            const short& model = CPickups::aPickUps[i].m_nModelIndex;
            const unsigned char& type = CPickups::aPickUps[i].m_nPickupType;

            if (((model == 953 || model == 954) && type == PICKUP_ONCE) || (model == 1253 && type == PICKUP_SNAPSHOT)) // oyster = 953, horseshoe = 954, snapshot = 1253
            {
                const CVector pickupPos(CPickups::aPickUps[i].m_vecPos.Uncompressed());
                const float distance = DistanceBetweenPoints(CVector2D(playaPos), CVector2D(pickupPos));
                if (distance < CRadar::m_radarRange)
                {
                    CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
                    //CRadar::LimitRadarPoint(radarSpace); // inside range anyway (returns distance)
                    CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                    int mode = RADAR_TRACE_LOW;
                    if (pickupPos.z - playaPos.z <= 2.0f)
                    {
                        if (pickupPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                        else mode = RADAR_TRACE_NORMAL;
                    }

                    CRGBA color(255, 255, 255);
                    switch (model)
                    {
                    case 953:
                        color = COLOR_OYSTER;
                        drawOyster = false;
                        break;
                    case 954:
                        color = COLOR_HORSE;
                        drawHorse = false;
                        break;
                    case 1253:
                        color = COLOR_SNAP;
                        drawSnap = false;
                        break;
                    }

                    CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color.r, color.g, color.b, color.a, mode);
                }
                else
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, COLOR_OYSTER.r, COLOR_OYSTER.g, COLOR_OYSTER.b, COLOR_OYSTER.a, mode);
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, COLOR_HORSE.r, COLOR_HORSE.g, COLOR_HORSE.b, COLOR_HORSE.a, mode);
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, COLOR_SNAP.r, COLOR_SNAP.g, COLOR_SNAP.b, COLOR_SNAP.a, mode);
        }
    }

    static void drawRadarUSJ(const CVector& playaPos)
    {
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
                    //CRadar::LimitRadarPoint(radarSpace); // inside range anyway (returns distance)
                    CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                    int mode = RADAR_TRACE_LOW;
                    if (usjPos.z - playaPos.z <= 2.0f)
                    {
                        if (usjPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                        else mode = RADAR_TRACE_NORMAL;
                    }

                    const CRGBA* color = &COLOR_USJ_UNDONE;
                    if (usj.found)
                    {
                        color = &COLOR_USJ_FOUND;
                    }

                    drawUsj = false;

                    CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color->r, color->g, color->b, color->a, mode);
                }
                else
                {
                    if (drawUsj && distance < nearestUsj)
                    {
                        nearestUsj = distance;
                        indexUsj = i;
                    }
                }
            }
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

            const CRGBA* color = &COLOR_USJ_UNDONE;
            if (usj.found)
            {
                color = &COLOR_USJ_FOUND;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color->r, color->g, color->b, color->a, mode);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    static void drawMapTags(const CVector& playaPos, bool showHeight)
    {
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
            //CRadar::LimitRadarPoint(radarSpace); // does nothing in map
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, COLOR_TAG.r, COLOR_TAG.g, COLOR_TAG.b, COLOR_TAG.a, mode);
        }
    }

    static void drawMapPickups(const CVector& playaPos, bool showHeight)
    {
        static CVector2D radarSpace, screenSpace;

        for (int i = 0; i < static_cast<int>(MAX_NUM_PICKUPS); i++)
        {
            const short& model = CPickups::aPickUps[i].m_nModelIndex;
            const unsigned char& type = CPickups::aPickUps[i].m_nPickupType;

            if (((model == 953 || model == 954) && type == PICKUP_ONCE) || (model == 1253 && type == PICKUP_SNAPSHOT)) // oyster = 953, horseshoe = 954, snapshot = 1253
            {
                const CVector pickupPos(CPickups::aPickUps[i].m_vecPos.Uncompressed());

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
                //CRadar::LimitRadarPoint(radarSpace); // does nothing in map
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

                CRGBA color(255, 255, 255);
                switch (model)
                {
                case 953:
                    color = COLOR_OYSTER;
                    break;
                case 954:
                    color = COLOR_HORSE;
                    break;
                case 1253:
                    color = COLOR_SNAP;
                    break;
                }

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color.r, color.g, color.b, color.a, mode);
            }
        }
    }

    static void drawMapUSJ(const CVector& playaPos, bool showHeight)
    {
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
                //CRadar::LimitRadarPoint(radarSpace); // does nothing in map
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

                const CRGBA* color = &COLOR_USJ_UNDONE;
                if (usj.found)
                {
                    color = &COLOR_USJ_FOUND;
                }

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color->r, color->g, color->b, color->a, mode);
            }
        }
    }
} collectiblesOnRadar;

const CRGBA CollectiblesOnRadar::COLOR_TAG(85, 255, 85);
const CRGBA CollectiblesOnRadar::COLOR_USJ_UNDONE(204, 153, 255);
const CRGBA CollectiblesOnRadar::COLOR_USJ_FOUND(222, 136, 255);
const CRGBA CollectiblesOnRadar::COLOR_OYSTER(255, 255, 153);
const CRGBA CollectiblesOnRadar::COLOR_HORSE(147, 192, 238);
const CRGBA CollectiblesOnRadar::COLOR_SNAP(255, 161, 208);

bool CollectiblesOnRadar::s_modEnabled = true;
int CollectiblesOnRadar::s_keyPressTime = 0;
