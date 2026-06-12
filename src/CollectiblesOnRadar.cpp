#include "plugin.h"
#include "common.h"
#include "C3dMarkers.h" // arrows above traffic vehicles
#include "CCamera.h" // TheCamera
#include "CHud.h" // CHud::SetHelpMessage
#include "CMenuManager.h" // FrontEndMenuManager.drawRadarOrMap
#include "CPickups.h"
#include "CRadar.h"
#include "CTheScripts.h" // CTheScripts::IsPlayerOnAMission
#include "CTimer.h" // CTimer::m_snTimeInMilliseconds

#include "Util.h"
#include "Settings.h"
#include "ExportVehicles.h"

// the same hook points as plugin's drawBlipsEvent, but with PRIORITY_BEFORE -
// whatever is drawn here ends up above the radar ring, below every game blip
static plugin::CdeclEvent<plugin::AddressList<0x58AA2D, plugin::H_JUMP, 0x575B44, plugin::H_CALL>,
    plugin::PRIORITY_BEFORE, plugin::ArgPickNone, void()> drawBeforeBlipsEvent;

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

        drawBeforeBlipsEvent += []
        {
            CPlayerPed* playa = FindPlayerPed();
            if (!s_modEnabled || !playa || !Settings::s_drawExportVehicles)
            {
                return;
            }

            // the export script empties the wanted list during missions anyway
            if (CTheScripts::IsPlayerOnAMission())
            {
                return;
            }

            ExportVehicles::update();

            if (FrontEndMenuManager.m_bDrawRadarOrMap) // map
            {
                drawMapExports();
            }
            else if (playa->m_nAreaCode == 0) // radar
            {
                drawRadarExports(FindPlayerCentreOfWorld_NoSniperShift(0));
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

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, tagPos.To2D());
            const float mag = CRadar::LimitRadarPoint(radarSpace); // returns distance
            const float distance = CVector2D::Distance(playaPos.To2D(), tagPos.To2D());
            if (mag <= 1.f) // (distance < CRadar::m_radarRange)
            {
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

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, tagPos.To2D());
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

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, pickupPos.To2D());
                const float mag = CRadar::LimitRadarPoint(radarSpace); // returns distance
                const float distance = CVector2D::Distance(playaPos.To2D(), pickupPos.To2D());
                if (mag <= 1.f) // (distance < CRadar::m_radarRange)
                {
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

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, pickupPos.To2D());
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

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, pickupPos.To2D());
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

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, pickupPos.To2D());
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

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, usjPos.To2D());
                const float mag = CRadar::LimitRadarPoint(radarSpace); // returns distance
                const float distance = CVector2D::Distance(playaPos.To2D(), usjPos.To2D());
                if (mag <= 1.f) // (distance < CRadar::m_radarRange)
                {
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

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, usjPos.To2D());
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

    static void drawExportIcon(float x, float y, bool onMap)
    {
        const float stretchX = RsGlobal.maximumWidth / 640.f;
        const float stretchY = RsGlobal.maximumHeight / 448.f;

        if (onMap) // map positions are in the menu's base space
        {
            x *= stretchX;
            y *= stretchY;
        }

        const float halfW = Settings::s_exportSpriteSize * stretchX * 0.5f;
        const float halfH = Settings::s_exportSpriteSize * stretchY * 0.5f;
        CRadar::RadarBlipSprites[Settings::s_exportSpriteId].Draw(
            CRect(x - halfW, y - halfH, x + halfW, y + halfH), CRGBA(255, 255, 255, 255));
    }

    static void drawRadarExports(const CVector& playaPos)
    {
        if (!Settings::s_drawExportVehicles)
        {
            return;
        }

        static CVector2D radarSpace, screenSpace;

        CVehicle* playaVehicle = FindPlayerVehicle(-1, false);
        const short playaModel = playaVehicle != nullptr ? playaVehicle->m_nModelIndex : -1;

        bool modelIconDrawn[ExportVehicles::TOTAL_SPAWNS] = {};

        for (int i = 0; i < ExportVehicles::TOTAL_SPAWNS; i++)
        {
            if (!ExportVehicles::s_wanted[i] || ExportVehicles::s_spawns[i].model == playaModel)
            {
                continue;
            }

            const CVehicle* tracked = ExportVehicles::s_vehicles[i];

            const CVector2D worldPos = tracked != nullptr
                ? tracked->GetPosition().To2D()
                : CVector2D(ExportVehicles::s_spawns[i].x, ExportVehicles::s_spawns[i].y);

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, worldPos);
            if (CRadar::LimitRadarPoint(radarSpace) <= 1.f)
            {
                CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);
                drawExportIcon(screenSpace.x, screenSpace.y, false);

                if (tracked != nullptr)
                {
                    modelIconDrawn[i] = true;
                }
            }
        }

        // at most one icon per model - the icon sticks to its vehicle as long as
        // it stays on the radar, then moves to the nearest matching one
        CVehicle* nearestVehicle[ExportVehicles::TOTAL_SPAWNS] = {};
        float nearestDistance[ExportVehicles::TOTAL_SPAWNS];
        static int stickyVehicleRef[ExportVehicles::TOTAL_SPAWNS]; // 0 = none

        for (int i = 0; i < CPools::ms_pVehiclePool->m_nSize; i++)
        {
            CVehicle* vehicle = CPools::ms_pVehiclePool->GetAt(i);
            if (vehicle == nullptr || !isIconCandidate(vehicle, playaModel))
            {
                continue;
            }

            const int spawnIndex = ExportVehicles::wantedSpawnIndexForModel(vehicle->m_nModelIndex);
            if (spawnIndex < 0 || modelIconDrawn[spawnIndex])
            {
                continue; // not wanted, or its tracked spawn car is already on the radar
            }

            const float distance = CVector2D::Distance(playaPos.To2D(), vehicle->GetPosition().To2D());
            if (nearestVehicle[spawnIndex] == nullptr || distance < nearestDistance[spawnIndex])
            {
                nearestVehicle[spawnIndex] = vehicle;
                nearestDistance[spawnIndex] = distance;
            }
        }

        for (int i = 0; i < ExportVehicles::TOTAL_SPAWNS; i++)
        {
            CVehicle* sticky = stickyVehicleRef[i] != 0 ? CPools::ms_pVehiclePool->GetAtRef(stickyVehicleRef[i]) : nullptr;
            if (sticky != nullptr
                && (sticky->m_nModelIndex != ExportVehicles::s_spawns[i].model
                    || !ExportVehicles::s_wanted[i] || modelIconDrawn[i]
                    || !isIconCandidate(sticky, playaModel)))
            {
                sticky = nullptr;
            }

            stickyVehicleRef[i] = 0;

            if (sticky != nullptr)
            {
                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, sticky->GetPosition().To2D());
                if (CRadar::LimitRadarPoint(radarSpace) <= 1.f)
                {
                    CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);
                    drawExportIcon(screenSpace.x, screenSpace.y, false);
                    drawVehicleArrow(sticky, i);
                    stickyVehicleRef[i] = CPools::ms_pVehiclePool->GetRef(sticky);
                    continue;
                }
            }

            if (nearestVehicle[i] != nullptr)
            {
                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, nearestVehicle[i]->GetPosition().To2D());
                if (CRadar::LimitRadarPoint(radarSpace) <= 1.f)
                {
                    CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);
                    drawExportIcon(screenSpace.x, screenSpace.y, false);
                    drawVehicleArrow(nearestVehicle[i], i);
                    stickyVehicleRef[i] = CPools::ms_pVehiclePool->GetRef(nearestVehicle[i]);
                }
            }
        }
    }

    // a blue arrow above an iconed traffic vehicle, to tell it apart in dense
    // traffic; parked spawns get none - their fixed locations are easy to spot
    static void drawVehicleArrow(CVehicle* vehicle, int spawnIndex)
    {
        if (!Settings::s_drawExportArrows || vehicle->m_nCreatedBy != RANDOM_VEHICLE)
        {
            return;
        }

        if (vehicle->bHasBeenOwnedByPlayer)
        {
            return; // the player has already driven it - no need to point at it
        }

        const float bob = 0.25f * std::sin(CTimer::m_snTimeInMilliseconds * 0.006f);

        CVector posn = vehicle->GetPosition();
        posn.z += vehicle->GetColModel()->m_boundBox.m_vecMax.z + 2.5f + bob;

        C3dMarkers::PlaceMarkerSet(0x45585000 + spawnIndex, MARKER3D_ARROW, posn, 1.5f,
            0, 128, 255, 200, 1024, 0.2f, 0);
    }

    static bool isIconCandidate(CVehicle* vehicle, short playaModel)
    {
        if (vehicle->m_nModelIndex == playaModel || vehicle->m_fHealth <= 0.f || vehicle->m_nAreaCode != 0)
        {
            return false;
        }

        if (ExportVehicles::isTrackedVehicle(vehicle))
        {
            return false; // handled by the spawn loop
        }

        const CVector& vehiclePos = vehicle->GetPosition();
        if (ExportVehicles::isAtOwnSpawn(vehicle->m_nModelIndex, vehiclePos.x, vehiclePos.y))
        {
            return false; // spawn marker is already drawn there
        }

        // distant traffic behind the camera despawns quickly, so its icons only
        // confuse; nearby vehicles stay in memory, so they keep theirs
        const CVector2D toVehicle((vehiclePos - TheCamera.GetPosition()).To2D());
        if (TheCamera.GetForward().To2D().Dot(toVehicle) < 0.f && toVehicle.Magnitude() > 50.f)
        {
            return false;
        }

        return true;
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

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, tagPos.To2D());
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

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, pickupPos.To2D());
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

                CRadar::TransformRealWorldPointToRadarSpace(radarSpace, usjPos.To2D());
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

    static void drawMapExports()
    {
        if (!Settings::s_drawExportVehicles)
        {
            return;
        }

        static CVector2D radarSpace, screenSpace;

        CVehicle* playaVehicle = FindPlayerVehicle(-1, false);
        const short playaModel = playaVehicle != nullptr ? playaVehicle->m_nModelIndex : -1;

        for (int i = 0; i < ExportVehicles::TOTAL_SPAWNS; i++)
        {
            if (!ExportVehicles::s_wanted[i] || ExportVehicles::s_spawns[i].model == playaModel)
            {
                continue;
            }

            const CVehicle* tracked = ExportVehicles::s_vehicles[i];

            const CVector2D worldPos = tracked != nullptr
                ? tracked->GetPosition().To2D()
                : CVector2D(ExportVehicles::s_spawns[i].x, ExportVehicles::s_spawns[i].y);

            CRadar::TransformRealWorldPointToRadarSpace(radarSpace, worldPos);
            CRadar::TransformRadarPointToScreenSpace(screenSpace, radarSpace);
            drawExportIcon(screenSpace.x, screenSpace.y, true);
        }
    }
} collectiblesOnRadar;

bool CollectiblesOnRadar::s_modEnabled = true;
int CollectiblesOnRadar::s_keyPressTime = 0;
