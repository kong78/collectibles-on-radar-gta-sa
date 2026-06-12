#pragma once

class CVehicle;

struct tExportSpawn
{
    short model;
    float x, y, z;
};

class ExportVehicles
{
public:
    static const int TOTAL_SPAWNS = 30;
    static const tExportSpawn s_spawns[TOTAL_SPAWNS];

    // refreshed by update() every frame
    static bool s_wanted[TOTAL_SPAWNS];
    static CVehicle* s_vehicles[TOTAL_SPAWNS]; // car spawned by the generator, if it still exists

    static void update();
    static int wantedSpawnIndexForModel(short model); // -1 if the model is not currently wanted
    static bool isAtOwnSpawn(short model, float x, float y);
    static bool isTrackedVehicle(const CVehicle* vehicle);
};
