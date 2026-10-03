/*
 * godjo-hud.asi — скрывает стандартный HUD GTA SA (оружие, патроны, деньги, здоровье,
 * броня, кислород, звёзды розыска, часы), чтобы не дублировать интерфейс Godjo (CEF).
 * Радар, названия районов и машин остаются.
 *
 * Только для gta_sa.exe 1.0 US: перед патчем сверяются байты функции, на других
 * версиях плагин ничего не делает. Отключить: godjo.ini -> [hud] hide=0.
 *
 * Также чинит «игра не разворачивается после сворачивания» (Alt+Tab / Win):
 * окно игры подменяет оконную процедуру и при активации/клике по панели задач
 * принудительно восстанавливает окно (SW_RESTORE + фокус), а сторожевой поток
 * каждые 0.4 с проверяет, не осталось ли активное окно свёрнутым.
 * Отключить: godjo.ini -> [window] fix=0.
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

/* ---------- восстановление окна после сворачивания ---------- */
static HWND g_wnd;
static WNDPROC g_old;

static void restore_window(HWND w)
{
    if (!IsIconic(w)) return;
    ShowWindow(w, SW_RESTORE);
    SetForegroundWindow(w);
    SetFocus(w);
}

static LRESULT CALLBACK hook_proc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    switch (m)
    {
    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_RESTORE && IsIconic(w))
        {
            LRESULT r = CallWindowProcA(g_old, w, m, wp, lp);
            restore_window(w);
            return r;
        }
        break;
    case WM_ACTIVATEAPP:
        if (wp && IsIconic(w)) PostMessageA(w, WM_SYSCOMMAND, SC_RESTORE, 0);
        break;
    case WM_ACTIVATE:
        if (LOWORD(wp) != WA_INACTIVE && HIWORD(wp)) PostMessageA(w, WM_SYSCOMMAND, SC_RESTORE, 0);
        break;
    }
    return CallWindowProcA(g_old, w, m, wp, lp);
}

static BOOL CALLBACK find_wnd(HWND w, LPARAM lp)
{
    DWORD pid = 0;
    char cls[64];
    GetWindowThreadProcessId(w, &pid);
    if (pid != GetCurrentProcessId() || GetParent(w)) return TRUE;
    GetClassNameA(w, cls, sizeof cls);
    if (!lstrcmpiA(cls, "Grand theft auto San Andreas")) { *(HWND *)lp = w; return FALSE; }
    return TRUE;
}

static DWORD WINAPI window_thread(LPVOID p)
{
    int i;
    (void)p;
    for (i = 0; i < 1200 && !g_wnd; i++) /* до 10 минут ждём окно игры */
    {
        HWND w = NULL;
        EnumWindows(find_wnd, (LPARAM)&w);
        if (w && IsWindowVisible(w)) g_wnd = w;
        else Sleep(500);
    }
    if (!g_wnd) return 0;
    g_old = (WNDPROC)SetWindowLongPtrA(g_wnd, GWLP_WNDPROC, (LONG_PTR)hook_proc);
    for (;;)
    {
        Sleep(400);
        if (!IsWindow(g_wnd)) break;
        /* окно активно (клик по панели задач/Alt+Tab), но осталось свёрнутым */
        if (IsIconic(g_wnd) && GetForegroundWindow() == g_wnd) PostMessageA(g_wnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    }
    return 0;
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
        if (GetPrivateProfileIntA("window", "fix", 1, ini))
        {
            HANDLE t = CreateThread(NULL, 0, window_thread, NULL, 0, NULL);
            if (t) CloseHandle(t);
        }
    }
    return TRUE;
}
