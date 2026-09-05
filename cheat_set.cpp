#include "../../src/tesmio_plugin.h"

// ---------------------------------------------------------------- constants and addresses

// Pointer to world object (to detect new game loads)
#define RVA_WORLD_PTR     0x9941F0

// Cheat menu flag (global in .data)
#define RVA_CHEAT_MENU_FLAG 0x9D4F18

// Offsets in the World object for cheat checkboxes
#define OFFSET_SPEED_UP_CONSTRUCTION   0x14448
#define OFFSET_SPEED_UP_RESEARCH       0x14449
#define OFFSET_SPEED_UP_VEHICLES       0x1444a
#define OFFSET_SPEED_UP_TREES          0x1444b
#define OFFSET_SPEED_UP_POLLUTION      0x1444c
#define OFFSET_SPEED_UP_WEAR           0x1444d
#define OFFSET_LANDSCAPE_EDITOR        0x1090
#define OFFSET_EXP_TRAFFIC             0x1444e
#define OFFSET_CO_COOPERATE            0x14455
#define OFFSET_TRAIN_ROUTE_SIGNAL      0x1444f

// Terrain render export for per-frame tick
#define SYM_TERRAIN_RENDER "?Render@C3D_TERRAIN@@QEAAX_NPEAVC3D_CAMERA@@0HH@Z"

// ---------------------------------------------------------------- settings

static int g_enabled = 1;
static int g_autoKeys = 0;
static int g_memoryPatch = 1;

static int g_speedUpConstruction = 0;
static int g_speedUpResearch = 0;
static int g_speedUpVehicleProduction = 0;
static int g_speedUpGrowingTrees = 0;
static int g_speedUpPollution = 0;
static int g_speedUpWearAndTear = 0;
static int g_landscapeEditorMode = 0;
static int g_experimentalTrafficPathfinding = 0;
static int g_coCooperate = 1;
static int g_trainRouteSignal = 1;

// ---------------------------------------------------------------- state

typedef void (*t_TerrainRender)(void*, bool, void*, void*, int, int);
static t_TerrainRender o_TerrainRender = NULL;

static void* g_lastWorld = NULL;
static int g_frameTimer = 0;
static bool g_keysSent = false;
static bool g_memoryPatched = false;

// ---------------------------------------------------------------- fast logic

static void* GetWorldPtr(void)
{
    void** slot = (void**)(g_exeBase + RVA_WORLD_PTR);
    if (!ReadablePtr(slot, sizeof(void*))) return NULL;
    return *slot;
}

static void SendCheatKeystrokes()
{
    INPUT inputs[6] = {0};

    // C
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = 'C';
    inputs[0].ki.wScan = 0x2E;
    
    // H
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'H';
    inputs[1].ki.wScan = 0x23;

    // E
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'E';
    inputs[2].ki.wScan = 0x12;

    // C up
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = 'C';
    inputs[3].ki.wScan = 0x2E;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    // H up
    inputs[4].type = INPUT_KEYBOARD;
    inputs[4].ki.wVk = 'H';
    inputs[4].ki.wScan = 0x23;
    inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;

    // E up
    inputs[5].type = INPUT_KEYBOARD;
    inputs[5].ki.wVk = 'E';
    inputs[5].ki.wScan = 0x12;
    inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

    HMODULE hUser32 = LoadLibraryA("user32.dll");
    if (hUser32) {
        typedef UINT (WINAPI *t_SendInput)(UINT, LPINPUT, int);
        t_SendInput pSendInput = (t_SendInput)GetProcAddress(hUser32, "SendInput");
        if (pSendInput) {
            pSendInput(6, inputs, sizeof(INPUT));
            Logf("cheat_set  sent C+H+E keystrokes to activate cheat menu");
        }
    }
}

static void PatchCheatMemory(void* currentWorld)
{
    // Enable cheat menu flag (global flag in SOVIET64.exe .data)
    int* pCheatFlag = (int*)(g_exeBase + RVA_CHEAT_MENU_FLAG);
    if (ReadablePtr(pCheatFlag, sizeof(int)))
    {
        *pCheatFlag = 256;
        Logf("cheat_set  patched cheat menu memory (0x9D4F18 = 256)");
    }

    if (currentWorld)
    {
        BYTE* pWorld = (BYTE*)currentWorld;
        
        // Block of flags around 0x14448..0x14455
        if (ReadablePtr(pWorld + OFFSET_SPEED_UP_CONSTRUCTION, 0x14456 - OFFSET_SPEED_UP_CONSTRUCTION))
        {
            pWorld[OFFSET_SPEED_UP_CONSTRUCTION] = g_speedUpConstruction ? 1 : 0;
            pWorld[OFFSET_SPEED_UP_RESEARCH] = g_speedUpResearch ? 1 : 0;
            pWorld[OFFSET_SPEED_UP_VEHICLES] = g_speedUpVehicleProduction ? 1 : 0;
            pWorld[OFFSET_SPEED_UP_TREES] = g_speedUpGrowingTrees ? 1 : 0;
            pWorld[OFFSET_SPEED_UP_POLLUTION] = g_speedUpPollution ? 1 : 0;
            pWorld[OFFSET_SPEED_UP_WEAR] = g_speedUpWearAndTear ? 1 : 0;
            pWorld[OFFSET_EXP_TRAFFIC] = g_experimentalTrafficPathfinding ? 1 : 0;
            pWorld[OFFSET_CO_COOPERATE] = g_coCooperate ? 1 : 0;
            pWorld[OFFSET_TRAIN_ROUTE_SIGNAL] = g_trainRouteSignal ? 1 : 0;
        }

        if (ReadablePtr(pWorld + OFFSET_LANDSCAPE_EDITOR, sizeof(BYTE)))
        {
            pWorld[OFFSET_LANDSCAPE_EDITOR] = g_landscapeEditorMode ? 1 : 0;
        }

        Logf("cheat_set  patched world cheat features via exact game offsets");
    }
}

static void ReadSettings(void);

static void FastProcessCheats(void)
{
    void* currentWorld = GetWorldPtr();
    if (!currentWorld)
    {
        g_lastWorld = NULL;
        g_keysSent = false;
        g_memoryPatched = false;
        g_frameTimer = 0;
        return;
    }

    bool isNewWorld = (currentWorld != g_lastWorld);
    if (isNewWorld)
    {
        g_lastWorld = currentWorld;
        g_keysSent = false;
        g_memoryPatched = false;
        g_frameTimer = 0;
        
        // Znovu načíst INI při každém načtení mapy
        ReadSettings();
    }
    
    // We can apply the memory patch immediately when the world is valid
    if (g_memoryPatch && !g_memoryPatched)
    {
        PatchCheatMemory(currentWorld);
        g_memoryPatched = true;
    }

    if (g_autoKeys && !g_keysSent)
    {
        g_frameTimer++;
        // Wait ~2 seconds (assuming 60fps, 120 frames) before sending keys
        // so the game is fully loaded and can process input.
        if (g_frameTimer > 120)
        {
            SendCheatKeystrokes();
            g_keysSent = true;
        }
    }
}

static void HookTerrainRender(void* self, bool a, void* b, void* c, int d, int e)
{
    if (o_TerrainRender)
    {
        o_TerrainRender(self, a, b, c, d, e);
    }

    if (g_enabled)
    {
        FastProcessCheats();
    }
}

// ---------------------------------------------------------------- configuration and exports

static void ReadSettings(void)
{
    const char* ini = "plugins\\cheat_set.ini";

    g_enabled = H->configInt(ini, "cheat_set", "enabled", g_enabled);
    g_autoKeys = H->configInt(ini, "cheat_set", "auto_keys", g_autoKeys);
    g_memoryPatch = H->configInt(ini, "cheat_set", "memory_patch", g_memoryPatch);
    
    g_speedUpConstruction = H->configInt(ini, "cheat_set", "speed_up_construction", g_speedUpConstruction);
    g_speedUpResearch = H->configInt(ini, "cheat_set", "speed_up_research", g_speedUpResearch);
    g_speedUpVehicleProduction = H->configInt(ini, "cheat_set", "speed_up_vehicle_production", g_speedUpVehicleProduction);
    g_speedUpGrowingTrees = H->configInt(ini, "cheat_set", "speed_up_growing_trees", g_speedUpGrowingTrees);
    
    g_speedUpPollution = H->configInt(ini, "cheat_set", "speed_up_pollution", g_speedUpPollution);
    g_speedUpWearAndTear = H->configInt(ini, "cheat_set", "speed_up_wear_and_tear", g_speedUpWearAndTear);
    g_landscapeEditorMode = H->configInt(ini, "cheat_set", "landscape_editor_mode", g_landscapeEditorMode);
    g_experimentalTrafficPathfinding = H->configInt(ini, "cheat_set", "experimental_traffic_pathfinding", g_experimentalTrafficPathfinding);
    g_coCooperate = H->configInt(ini, "cheat_set", "co_cooperate", g_coCooperate);
    g_trainRouteSignal = H->configInt(ini, "cheat_set", "train_route_signal", g_trainRouteSignal);
}

extern "C" __declspec(dllexport) unsigned TsmPluginApiVersion(void)
{
    return TSM_API_VERSION;
}

extern "C" __declspec(dllexport) int TsmPluginInit(const TsmHost* host, TsmPluginInfo* info)
{
    TsmBind(host);
    info->name    = "cheat_set";
    info->version = "1.0";

    ReadSettings();

    if (!g_enabled)
    {
        Logf("cheat_set  disabled");
        return 1;
    }

    Logf("cheat_set  init (auto keys: %d)", g_autoKeys);
    return 0;
}

extern "C" __declspec(dllexport) int TsmPluginStart(void)
{
    if (!g_enabled) return 1;

    if (!PatchIat(g_exe, DLL_ENGINE, SYM_TERRAIN_RENDER,
                  (void*)HookTerrainRender, (void**)&o_TerrainRender, "terrain render"))
    {
        Logf("cheat_set  failed to hook terrain render");
        return 1;
    }

    Logf("cheat_set  active");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
