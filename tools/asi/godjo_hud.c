/*
 * godjo-hud.asi — скрывает стандартный HUD GTA SA (оружие, патроны, деньги, здоровье,
 * броня, кислород, звёзды розыска, часы), чтобы не дублировать интерфейс Godjo (CEF).
 * Радар, названия районов и машин остаются.
 *
 * Только для gta_sa.exe 1.0 US: перед патчем сверяются байты функции, на других
 * версиях плагин ничего не делает. Отключить: godjo.ini -> [hud] hide=0.
 *
 * Сборка: i686-w64-mingw32-gcc -O2 -s -shared -o godjo-hud.asi godjo_hud.c
 */
#include <windows.h>
#include <string.h>

typedef struct { DWORD addr; unsigned char expect[6]; int n; } Patch;

/* CHud::DrawPlayerInfo (0x58EAF0): sub esp, 1A0h  -> ret */
static const Patch PATCHES[] = {
    { 0x58EAF0, { 0x81, 0xEC, 0xA0, 0x01, 0x00, 0x00 }, 6 },
};

static int apply(const Patch *p)
{
    unsigned char *a = (unsigned char *)p->addr;
    DWORD old;
    if (IsBadReadPtr(a, p->n) || memcmp(a, p->expect, p->n) != 0) return 0;
    if (!VirtualProtect(a, 1, PAGE_EXECUTE_READWRITE, &old)) return 0;
    a[0] = 0xC3;
    VirtualProtect(a, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), a, 1);
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID res)
{
    (void)res;
    if (reason == DLL_PROCESS_ATTACH)
    {
        char ini[MAX_PATH], *s;
        DisableThreadLibraryCalls(inst);
        GetModuleFileNameA(inst, ini, MAX_PATH);
        s = strrchr(ini, '\\');
        if (s) strcpy(s + 1, "godjo.ini"); else strcpy(ini, "godjo.ini");
        if (GetPrivateProfileIntA("hud", "hide", 1, ini))
        {
            size_t i;
            for (i = 0; i < sizeof PATCHES / sizeof PATCHES[0]; i++) apply(&PATCHES[i]);
        }
    }
    return TRUE;
}
