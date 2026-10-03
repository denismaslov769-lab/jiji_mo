// =====================================================================
//  Мост с CEF-интерфейсом
//  Сервер -> UI:  gj:open(screen, json), gj:close(screen), gj:notify(type, text),
//                 gj:data(key, json), gj:hud(json)
//  UI -> сервер:  событие "gj", один аргумент "модуль|поле|поле..."
// =====================================================================
#define UI:%1(%2) forward ui_%1(%2); public ui_%1(%2)
#define MENU:%1(%2) forward menu_%1(%2); public menu_%1(%2)
#define PROMPT:%1(%2) forward prompt_%1(%2); public prompt_%1(%2)

new gMB[6144];
new gMBCount;

stock UI_Open(playerid, const screen[], const json[] = "{}")
{
    if (!P[playerid][pCef]) return 0;
    strcpy(P[playerid][pScreen], screen, 16);
    cef_emit_event(playerid, "gj:open", CEFSTR(screen), CEFSTR(json));
    cef_focus_browser(playerid, CEF_BROWSER_ID, true);
    return 1;
}

stock UI_Close(playerid, const screen[] = "")
{
    if (!P[playerid][pCef]) return 0;
    cef_emit_event(playerid, "gj:close", CEFSTR(screen));
    P[playerid][pScreen][0] = 0;
    cef_focus_browser(playerid, CEF_BROWSER_ID, false);
    return 1;
}

stock UI_Data(playerid, const key[], const json[])
{
    if (!P[playerid][pCef]) return 0;
    cef_emit_event(playerid, "gj:data", CEFSTR(key), CEFSTR(json));
    return 1;
}

stock UI_Notify(playerid, const type[], const text[])
{
    if (!P[playerid][pCef]) { SCM(playerid, C_YELLOW, text); return 1; }
    cef_emit_event(playerid, "gj:notify", CEFSTR(type), CEFSTR(text));
    return 1;
}

stock Info(playerid, const text[]) return UI_Notify(playerid, "info", text);
stock Ok(playerid, const text[]) return UI_Notify(playerid, "ok", text);
stock Err(playerid, const text[]) return UI_Notify(playerid, "error", text);

// ---------- универсальное меню ----------
stock Menu_Begin(const title[], const sub[] = "")
{
    format(gMB, sizeof gMB, "{\"title\":\"%s\",\"sub\":\"%s\",\"items\":[", JsonEsc(title), JsonEsc(sub));
    gMBCount = 0;
}

stock Menu_Add(id, const title[], const desc[] = "", price = -1, const icon[] = "")
{
    new it[400];
    format(it, sizeof it, "%s{\"id\":%d,\"t\":\"%s\",\"d\":\"%s\",\"p\":%d,\"i\":\"%s\"}",
        gMBCount ? "," : "", id, JsonEsc(title), JsonEsc(desc), price, icon);
    strcat(gMB, it);
    gMBCount++;
}

stock Menu_Send(playerid, const menuid[])
{
    new tail[48];
    format(tail, sizeof tail, "],\"menu\":\"%s\"}", menuid);
    strcat(gMB, tail);
    UI_Open(playerid, "menu", gMB);
}

// ---------- окно подтверждения / ввода ----------
stock UI_Prompt(playerid, const id[], const title[], const text[], bool:input = false, const placeholder[] = "", const okText[] = "Ок")
{
    new j[700];
    format(j, sizeof j, "{\"id\":\"%s\",\"title\":\"%s\",\"text\":\"%s\",\"input\":%s,\"ph\":\"%s\",\"ok\":\"%s\"}",
        id, JsonEsc(title), JsonEsc(text), input ? ("true") : ("false"), JsonEsc(placeholder), JsonEsc(okText));
    UI_Open(playerid, "prompt", j);
}

// ---------- HUD ----------
stock UI_Hud(playerid)
{
    if (!P[playerid][pCef] || !P[playerid][pLogged]) return;
    new j[640], Float:hp, Float:ar, veh = GetPlayerVehicleID(playerid), speed = 0, fuel = -1;
    GetPlayerHealth(playerid, hp);
    GetPlayerArmour(playerid, ar);
    if (veh && GetPlayerState(playerid) == PLAYER_STATE_DRIVER)
    {
        new Float:vx, Float:vy, Float:vz;
        GetVehicleVelocity(veh, vx, vy, vz);
        speed = floatround(floatsqroot(vx * vx + vy * vy + vz * vz) * 180.0);
        fuel = Veh_GetFuel(veh);
    }
    new h, m, s;
    gettime(h, m, s);
    format(j, sizeof j, "{\"cash\":%d,\"bank\":%d,\"hunger\":%d,\"fatigue\":%d,\"hp\":%d,\"ar\":%d,\"lvl\":%d,\"exp\":%d,\"need\":%d,\"wanted\":%d,\"speed\":%d,\"fuel\":%d,\"time\":\"%02d:%02d\",\"id\":%d,\"online\":%d,\"det\":%d,\"wep\":%d,\"ammo\":%d}",
        P[playerid][pCash], P[playerid][pBank], floatround(P[playerid][pHunger]), floatround(P[playerid][pFatigue]),
        floatround(hp), floatround(ar), P[playerid][pLevel], P[playerid][pExp], Lvl_Need(P[playerid][pLevel]),
        P[playerid][pWanted], speed, fuel, h, m, playerid, Iter_Online(), P[playerid][pDetector], GetPlayerWeapon(playerid), GetPlayerAmmo(playerid));
    cef_emit_event(playerid, "gj:hud", CEFSTR(j));
}

stock Iter_Online()
{
    new c = 0;
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsPlayerConnected(i) && !IsPlayerNPC(i)) c++;
    return c;
}

// ---------- приём событий ----------
forward OnCefGj(playerid, const arguments[]);
public OnCefGj(playerid, const arguments[])
{
    if (!IsPlayerConnected(playerid)) return 1;
    new module[24], fn[32];
    CefField(arguments, 0, module);
    if (!module[0]) return 1;
    for (new i = 0; module[i]; i++)
    {
        new c = module[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return 1;
    }
    format(fn, sizeof fn, "ui_%s", module);
    if (funcidx(fn) == -1) return 1;
    // авторизация разрешена всегда, остальное — только после входа
    if (!P[playerid][pLogged] && !StrEq(module, "auth") && !StrEq(module, "char") && !StrEq(module, "sys")) return 1;
    CallLocalFunction(fn, "is", playerid, arguments[0] ? arguments : ("\1"));
    return 1;
}

UI:sys(playerid, const a[])
{
    new act[16];
    CefField(a, 1, act);
    if (StrEq(act, "closed"))
    {
        P[playerid][pScreen][0] = 0;
        cef_focus_browser(playerid, CEF_BROWSER_ID, false);
    }
    return 1;
}

UI:menu(playerid, const a[])
{
    new id[24], fn[32], extra[64];
    CefField(a, 1, id);
    new item = CefFieldInt(a, 2);
    CefField(a, 3, extra);
    format(fn, sizeof fn, "menu_%s", id);
    if (funcidx(fn) == -1) return 1;
    CallLocalFunction(fn, "iis", playerid, item, extra[0] ? extra : ("\1"));
    return 1;
}

UI:prompt(playerid, const a[])
{
    new id[24], fn[32], text[128];
    CefField(a, 1, id);
    new resp = CefFieldInt(a, 2);
    CefField(a, 3, text);
    format(fn, sizeof fn, "prompt_%s", id);
    if (funcidx(fn) == -1) return 1;
    CallLocalFunction(fn, "iis", playerid, resp, text[0] ? text : ("\1"));
    return 1;
}
