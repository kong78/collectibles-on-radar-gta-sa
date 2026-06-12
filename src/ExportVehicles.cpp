#include "CPools.h" // CPools::ms_pVehiclePool
#include "CVector.h"
#include "CTheCarGenerators.h"

#include "ExportVehicles.h"

// Models and generator coordinates taken from the original main.scm source
// (impexp_car_gen[30]); identical in every retail main.scm revision.
const tExportSpawn ExportVehicles::s_spawns[ExportVehicles::TOTAL_SPAWNS] =
{
    // list 1
    { 402, -1673.94f,   439.02f,  7.01f }, // Buffalo
    { 405,   926.45f, -1292.29f, 13.60f }, // Sentinel
    { 411, -2665.44f,   990.77f, 64.45f }, // Infernus
    { 483, -2516.60f,  1228.92f, 36.43f }, // Camper
    { 445,  1122.29f, -1699.76f, 13.43f }, // Admiral
    { 470, -1006.41f,  -628.27f, 32.00f }, // Patriot
    { 468, -2085.23f, -2437.52f, 30.31f }, // Sanchez
    { 409, -1922.19f,   288.34f, 40.84f }, // Stretch
    { 533,   -16.66f, -2521.17f, 36.37f }, // Feltzer
    { 534,  1803.38f, -1931.05f, 13.66f }, // Remington

    // list 2
    { 415,  1272.24f,  2603.03f, 10.49f }, // Cheetah
    { 489,  -112.40f,   -41.82f,  3.26f }, // Rancher
    { 439, -2456.10f,   741.65f, 34.92f }, // Stallion
    { 514, -1951.81f,  2393.83f, 50.08f }, // Tanker
    { 480, -2751.79f,  -281.50f,  6.81f }, // Comet
    { 535,  1923.93f, -2118.89f, 13.35f }, // Slamvan
    { 496, -1675.94f,  -618.74f, 13.86f }, // Blista Compact
    { 580, -2430.22f,   320.84f, 34.97f }, // Stafford
    { 475, -2265.33f,   200.65f, 34.97f }, // Sabre
    { 521,  2282.70f,  2535.88f, 10.39f }, // FCR-900

    // list 3
    { 429,  2133.04f,  1009.75f, 10.49f }, // Banshee
    { 506,  2229.30f,  1402.99f, 10.82f }, // Super GT
    { 508, -1550.40f,  2687.54f, 56.22f }, // Journey
    { 579, -2068.69f,   -83.75f, 35.10f }, // Huntley
    { 424,   682.17f, -1867.46f,  4.82f }, // BF Injection
    { 536,  1747.87f, -2098.03f, 13.28f }, // Blade
    { 463,  1144.46f, -1101.26f, 25.35f }, // Freeway
    { 500, -2406.25f, -2180.84f, 33.39f }, // Mesa
    { 477,  2163.79f,  1810.23f, 10.58f }, // ZR-350
    { 587,  2207.43f,  1286.13f, 10.57f }, // Euros
};

bool ExportVehicles::s_wanted[ExportVehicles::TOTAL_SPAWNS];
CVehicle* ExportVehicles::s_vehicles[ExportVehicles::TOTAL_SPAWNS];

void ExportVehicles::update()
{
    for (int i = 0; i < TOTAL_SPAWNS; i++)
    {
        s_wanted[i] = false;
        s_vehicles[i] = nullptr;
    }

    for (int i = 0; i < 500; i++) // static CCarGenerator CarGeneratorArray[500]
    {
        const CCarGenerator& gen = CTheCarGenerators::CarGeneratorArray[i];
        if (!gen.m_bIsUsed || gen.m_nGenerateCount == 0)
        {
            continue;
        }

        for (int j = 0; j < TOTAL_SPAWNS; j++)
        {
            if (gen.m_nModelId != s_spawns[j].model)
            {
                continue;
            }

            const CVector genPos = gen.m_vecPosn.Uncompressed();
            if (std::fabs(genPos.x - s_spawns[j].x) < 3.f && std::fabs(genPos.y - s_spawns[j].y) < 3.f)
            {
                s_wanted[j] = true;

                // the car generated at this spawn, as long as it still exists
                CVehicle* vehicle = CPools::ms_pVehiclePool->GetAtRef(gen.m_nVehicleHandle);
                if (vehicle != nullptr && vehicle->m_fHealth > 0.f)
                {
                    s_vehicles[j] = vehicle;
                }
                break;
            }
        }
    }
}

int ExportVehicles::wantedSpawnIndexForModel(short model)
{
    for (int i = 0; i < TOTAL_SPAWNS; i++)
    {
        if (s_wanted[i] && s_spawns[i].model == model)
        {
            return i;
        }
    }
    return -1;
}

bool ExportVehicles::isTrackedVehicle(const CVehicle* vehicle)
{
    for (int i = 0; i < TOTAL_SPAWNS; i++)
    {
        if (s_wanted[i] && s_vehicles[i] == vehicle)
        {
            return true;
        }
    }
    return false;
}

bool ExportVehicles::isAtOwnSpawn(short model, float x, float y)
{
    for (int i = 0; i < TOTAL_SPAWNS; i++)
    {
        if (s_wanted[i] && s_spawns[i].model == model
            && std::fabs(x - s_spawns[i].x) < 5.f && std::fabs(y - s_spawns[i].y) < 5.f)
        {
            return true;
        }
    }
    return false;
}
