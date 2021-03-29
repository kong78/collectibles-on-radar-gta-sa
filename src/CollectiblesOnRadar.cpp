#include "plugin.h"
#include "common.h"
#include "CHud.h"
#include "CMenuManager.h"
#include "CPickups.h"
#include "CRadar.h"
#include "CTimer.h"

#include "Util.h"

bool modEnabled = true;
//bool showTags = true;
//bool showUSJs = true;
//bool showOysters = true;
//bool showSnapshots = true;
//bool showHorseshoes = true;

class CollectiblesOnRadar
{
public:
    CollectiblesOnRadar()
    {
        static int keyPressTime = 0;

        plugin::Events::gameProcessEvent += []
        {
            if (plugin::KeyPressed(VK_F1) && CTimer::m_snTimeInMilliseconds - keyPressTime > 200)
            {
                modEnabled = !modEnabled;
                CHud::SetHelpMessage(modEnabled ? "Collectibles on radar activated" : "Collectibles on radar deactivated", true, false, false);
                //CMessages::AddMessageJumpQ(modEnabled ? "Collectibles on radar activated" : "Collectibles on radar deactivated", 2000, false, false);
                keyPressTime = CTimer::m_snTimeInMilliseconds;
            }
        };

        plugin::Events::drawBlipsEvent += []
        {
            CPlayerPed* playa = FindPlayerPed();
            if (playa && modEnabled)
            {
                const CVector& playaPos = FindPlayerCentreOfWorld_NoSniperShift(0);
                if (!FrontEndMenuManager.drawRadarOrMap)
                {
                    if (playa->m_nAreaCode == 0)
                    {
                        drawRadarTags(playaPos); // tags
                        drawRadarPickups(playaPos); // oysters, horseshoes and snapshots
                        drawRadarUSJ(playaPos); // USJs
                    }
                }
                else
                {
                    drawMapTags(playaPos, playa->m_nAreaCode); // tags
                    drawMapPickups(playaPos, playa->m_nAreaCode); // oysters, horseshoes and snapshots
                    drawMapUSJ(playaPos, playa->m_nAreaCode); // USJs
                }
            }
        };
    }

    static void drawRadarTags(const CVector& playaPos)
    {
        static CVector2D radarSpace, screenSpace;
        static const CRGBA colorTag(85, 255, 85);

        bool drawTag = true;
        float nearestTag = 9999999.f;
        int indexTag = -1;

        for (int i = 0; i < Util::s_totalTags; i++)
        {
            if (Util::s_tagList[i].paint > 228) continue; // already painted

            if (Util::s_tagList[i].tag == nullptr) // DEBUG
            {
                //lg << "[ERROR] Util::s_tagList[" << i << "].tag == nullptr" << "\n";
                //lg.flush();
                continue;
            }

            const CVector& tagPos = Util::s_tagList[i].tag->pos;
            const float distance = DistanceBetweenPoints(CVector2D(playaPos), CVector2D(tagPos));
            if (distance < CRadar::m_radarRange)
            {
                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(tagPos));
                CRadar::LimitRadarPoint(radarSpace); // inside range anyway, returns distance (1.f for radar radius)
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                int mode = RADAR_TRACE_LOW;
                if (tagPos.z - playaPos.z <= 2.0f)
                {
                    if (tagPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                    else mode = RADAR_TRACE_NORMAL;
                }

                drawTag = false;

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, colorTag.r, colorTag.g, colorTag.b, colorTag.a, mode);
            }
            else
            {
                if (distance < nearestTag)
                {
                    nearestTag = distance;
                    indexTag = i;
                }
            }
        }

        if (drawTag && indexTag >= 0) // if there's no tag on range and found the closest tag, draw it
        {
            if (Util::s_tagList[indexTag].tag == nullptr) // DEBUG
            {
                //lg << "[ERROR] Util::s_tagList[" << indexTag << "].tag == nullptr" << "\n";
                //lg.flush();
            }
            else
            {
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

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, colorTag.r, colorTag.g, colorTag.b, colorTag.a, mode);
            }
        }
    }

    static void drawRadarPickups(const CVector& playaPos)
    {
        static CVector2D radarSpace, screenSpace;
        static const CRGBA colorOyster(255, 255, 153);
        static const CRGBA colorHorse(147, 192, 238);
        static const CRGBA colorSnap(255, 161, 208);

        bool drawOyster = true; // draw the nearest pickup outside range?
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
                    //CRadar::LimitRadarPoint(radarSpace); // inside range anyway
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
                        color = colorOyster;
                        drawOyster = false;
                        break;
                    case 954:
                        color = colorHorse;
                        drawHorse = false;
                        break;
                    case 1253:
                        color = colorSnap;
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
                        if (distance < nearestOyster)
                        {
                            nearestOyster = distance;
                            indexOyster = i;
                        }
                        break;
                    case 954: // horse
                        if (distance < nearestHorse)
                        {
                            nearestHorse = distance;
                            indexHorse = i;
                        }
                        break;
                    case 1253: // snap
                        if (distance < nearestSnap)
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, colorOyster.r, colorOyster.g, colorOyster.b, colorOyster.a, mode);
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, colorHorse.r, colorHorse.g, colorHorse.b, colorHorse.a, mode);
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

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, colorSnap.r, colorSnap.g, colorSnap.b, colorSnap.a, mode);
        }
    }

    static void drawRadarUSJ(const CVector& playaPos)
    {
        static CVector2D radarSpace, screenSpace;
        static const CRGBA colorUndone(204, 153, 255);
        static const CRGBA colorFound(222, 136, 255);

        bool drawUsj = true;
        float nearestUsj = 9999999.f;
        int indexUsj = -1;

        for (int i = 0; i < (Util::s_usjPool)->size; i++)
        {
            const tUsj& usj = (Util::s_usjPool)->entries[i];

            if ((Util::s_usjPool)->flags[i] == 0x01 && !usj.done)
            {
                const CVector usjPos(
                    (usj.start1.x + usj.start2.x) / 2.f,
                    (usj.start1.y + usj.start2.y) / 2.f,
                    usj.start1.z);
                const float distance = DistanceBetweenPoints(CVector2D(playaPos), CVector2D(usjPos));
                if (distance < CRadar::m_radarRange)
                {
                    CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(usjPos));
                    CRadar::LimitRadarPoint(radarSpace);
                    CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                    int mode = RADAR_TRACE_LOW;
                    if (usjPos.z - playaPos.z <= 2.0f)
                    {
                        if (usjPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                        else mode = RADAR_TRACE_NORMAL;
                    }

                    static const CRGBA* color = &colorUndone;
                    if (usj.found)
                    {
                        color = &colorFound;
                    }

                    drawUsj = false;

                    CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color->r, color->g, color->b, color->a, mode);
                }
                else
                {
                    if (distance < nearestUsj)
                    {
                        nearestUsj = distance;
                        indexUsj = i;
                    }
                }
            }
        }

        if (drawUsj && indexUsj >= 0)
        {
            const tUsj& usj = (Util::s_usjPool)->entries[indexUsj];

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

            const CRGBA* color = &colorUndone;
            if (usj.found)
            {
                color = &colorFound;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color->r, color->g, color->b, color->a, mode);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    static void drawMapTags(const CVector& playaPos, unsigned char areaCode)
    {
        static CVector2D radarSpace, screenSpace;
        static const CRGBA colorTag(85, 255, 85);

        for (int i = 0; i < Util::s_totalTags; i++)
        {
            if (Util::s_tagList[i].tag == nullptr) // DEBUG
            {
                //lg << "Util::s_tagList[" << i << "].tag == nullptr" << "\n";
                //lg.flush();
                continue;
            }

            if (Util::s_tagList[i].paint > 228) continue; // continue if tag already sprayed

            const CVector& tagPos = Util::s_tagList[i].tag->pos;

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(tagPos));
            CRadar::LimitRadarPoint(radarSpace);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

            int mode = RADAR_TRACE_LOW;
            if (areaCode != 0)
            {
                mode = RADAR_TRACE_NORMAL;
            }
            else if (tagPos.z - playaPos.z <= 2.0f)
            {
                if (tagPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                else mode = RADAR_TRACE_NORMAL;
            }

            CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, colorTag.r, colorTag.g, colorTag.b, colorTag.a, mode);
        }
    }

    static void drawMapPickups(const CVector& playaPos, unsigned char areaCode)
    {
        static CVector2D radarSpace, screenSpace;
        static const CRGBA colorOyster(255, 255, 153);
        static const CRGBA colorHorse(147, 192, 238);
        static const CRGBA colorSnap(255, 161, 208);

        for (int i = 0; i < static_cast<int>(MAX_NUM_PICKUPS); i++)
        {
            const short& model = CPickups::aPickUps[i].m_nModelIndex;
            const unsigned char& type = CPickups::aPickUps[i].m_nPickupType;

            if (((model == 953 || model == 954) && type == PICKUP_ONCE) || (model == 1253 && type == PICKUP_SNAPSHOT)) // oyster = 953, horseshoe = 954, snapshot = 1253
            {
                const CVector pickupPos(CPickups::aPickUps[i].m_vecPos.Uncompressed());

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(pickupPos));
                CRadar::LimitRadarPoint(radarSpace);
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                int mode = RADAR_TRACE_LOW;
                if (areaCode != 0)
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
                    color = colorOyster;
                    break;
                case 954:
                    color = colorHorse;
                    break;
                case 1253:
                    color = colorSnap;
                    break;
                }

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color.r, color.g, color.b, color.a, mode);
            }
        }
    }

    static void drawMapUSJ(const CVector& playaPos, unsigned char areaCode)
    {
        static CVector2D radarSpace, screenSpace;
        static const CRGBA colorUndone(204, 153, 255);
        static const CRGBA colorFound(222, 136, 255);

        for (int i = 0; i < (Util::s_usjPool)->size; i++)
        {
            const tUsj& usj = (Util::s_usjPool)->entries[i];

            if ((Util::s_usjPool)->flags[i] == 0x01 && !usj.done)
            {
                const CVector usjPos(
                    (usj.start1.x + usj.start2.x) / 2.f,
                    (usj.start1.y + usj.start2.y) / 2.f,
                    usj.start1.z);

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, CVector2D(usjPos));
                CRadar::LimitRadarPoint(radarSpace);
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);

                int mode = RADAR_TRACE_LOW;
                if (areaCode != 0)
                {
                    mode = RADAR_TRACE_NORMAL;
                }
                else if (usjPos.z - playaPos.z <= 2.0f)
                {
                    if (usjPos.z - playaPos.z < -4.0f) mode = RADAR_TRACE_HIGH;
                    else mode = RADAR_TRACE_NORMAL;
                }

                const CRGBA* color = &colorUndone;
                if (usj.found)
                {
                    color = &colorFound;
                }

                CRadar::ShowRadarTraceWithHeight(screenSpace.x, screenSpace.y, 1, color->r, color->g, color->b, color->a, mode);
            }
        }
    }
} collectiblesOnRadar;
