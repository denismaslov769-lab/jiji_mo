// =====================================================================
//  Администрирование. Уровни: 1 хелпер, 2 модератор, 3 админ,
//  4 ст. админ, 5 руководство. Первый админ — /makeadmin после /rcon login
// =====================================================================
#define MAX_REPORTS 30
enum E_REPORT { bool:rUsed, rFrom, rText[128], rTime }
new gReport[MAX_REPORTS][E_REPORT];

stock bool:AdmCheck(playerid, level)
{
    if (P[playerid][pAdmin] >= level) return true;
    Err(playerid, "Недостаточно прав.");
    return false;
}

stock Adm_Msg(level, const msg[])
{
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsLogged(i) && P[i][pAdmin] >= level) SCM(i, C_ADMIN, msg);
}

stock Adm_Log(playerid, const action[])
{
    new s[160]; format(s, sizeof s, "[A] %s: %s", PName(playerid), action);
    Adm_Msg(1, s);
    Log("admin", PName(playerid), action);
}

CMD:makeadmin(playerid, const params[])
{
    if (!IsPlayerAdmin(playerid)) return Err(playerid, "Сначала /rcon login [пароль].");
    new t = GetParamPlayer(params, 0), lvl = GetParamInt(params, 1, 5);
    if (t == INVALID_PLAYER_ID || !IsLogged(t)) return Err(playerid, "/makeadmin [id] [уровень 1-5]");
    if (lvl < 0 || lvl > 5) lvl = 5;
    P[t][pAdmin] = lvl; P_SetInt(t, "admin", lvl);
    new s[96]; format(s, sizeof s, "выдал %s админ-уровень %d (RCON)", PName(t), lvl); Adm_Log(playerid, s);
    Ok(t, "Вам выданы права администратора. /ahelp");
    return 1;
}

CMD:ahelp(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    SCM(playerid, C_ADMIN, "1: /a /ans /reports /goto /spec /specoff /mute /unmute /kick /admins");
    if (P[playerid][pAdmin] >= 2) SCM(playerid, C_ADMIN, "2: /gethere /jail /unjail /warn /hp /slap /freeze /unfreeze");
    if (P[playerid][pAdmin] >= 3) SCM(playerid, C_ADMIN, "3: /ban /offban /unban /veh /dveh /tp /setskin /fixveh /check");
    if (P[playerid][pAdmin] >= 4) SCM(playerid, C_ADMIN, "4: /givemoney /setlevel /setleader /giveitem /setloc /locs");
    if (P[playerid][pAdmin] >= 5) SCM(playerid, C_ADMIN, "5: /setadmin /gmx /settreasury");
    return 1;
}

CMD:a(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    if (isnull(params)) return Err(playerid, "/a [текст]");
    new s[160]; format(s, sizeof s, "[A-чат] %s[%d]: %s", PName(playerid), playerid, params);
    Adm_Msg(1, s);
    return 1;
}

CMD:admins(playerid, const params[])
{
    SCM(playerid, C_ADMIN, "Администрация онлайн:");
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsLogged(i) || !P[i][pAdmin]) continue;
        new s[64]; format(s, sizeof s, " %s[%d] — ур. %d", PName(i), i, P[i][pAdmin]); SCM(playerid, C_WHITE, s);
    }
    return 1;
}

CMD:report(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/report [вопрос или жалоба]");
    if (gettime() - P[playerid][pReportCD] < 60) return Err(playerid, "Не чаще раза в минуту.");
    new k = -1;
    for (new i = 0; i < MAX_REPORTS; i++) if (!gReport[i][rUsed]) { k = i; break; }
    if (k == -1) return Err(playerid, "Очередь репортов заполнена, попробуйте позже.");
    gReport[k][rUsed] = true; gReport[k][rFrom] = playerid; gReport[k][rTime] = gettime();
    strcpy(gReport[k][rText], params, 128);
    P[playerid][pReportCD] = gettime();
    new s[200]; format(s, sizeof s, "[Репорт #%d] %s[%d]: %s", k, PName(playerid), playerid, params);
    Adm_Msg(1, s);
    Ok(playerid, "Репорт отправлен администрации.");
    return 1;
}

CMD:reports(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    new c = 0;
    for (new i = 0; i < MAX_REPORTS; i++)
    {
        if (!gReport[i][rUsed]) continue;
        if (!IsPlayerConnected(gReport[i][rFrom])) { gReport[i][rUsed] = false; continue; }
        new s[200]; format(s, sizeof s, "#%d %s[%d]: %s", i, PName(gReport[i][rFrom]), gReport[i][rFrom], gReport[i][rText]);
        SCM(playerid, C_WHITE, s); c++;
    }
    if (!c) Info(playerid, "Репортов нет.");
    return 1;
}

CMD:ans(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    new t = GetParamPlayer(params, 0), text[128];
    GetRest(params, 1, text);
    if (t == INVALID_PLAYER_ID || isnull(text)) return Err(playerid, "/ans [id] [ответ]");
    new s[200]; format(s, sizeof s, "Администратор %s: %s", PName(playerid), text);
    SCM(t, C_ADMIN, s);
    for (new i = 0; i < MAX_REPORTS; i++) if (gReport[i][rUsed] && gReport[i][rFrom] == t) gReport[i][rUsed] = false;
    format(s, sizeof s, "ответил %s: %s", PName(t), text); Adm_Log(playerid, s);
    return 1;
}

stock Adm_Target(playerid, const params[], const usage[])
{
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || !IsPlayerConnected(t)) { Err(playerid, usage); return INVALID_PLAYER_ID; }
    if (t != playerid && P[t][pAdmin] > P[playerid][pAdmin]) { Err(playerid, "Нельзя применить к старшему администратору."); return INVALID_PLAYER_ID; }
    return t;
}

CMD:goto(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    new t = Adm_Target(playerid, params, "/goto [id]"); if (t == INVALID_PLAYER_ID) return 1;
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z);
    SetPlayerInterior(playerid, GetPlayerInterior(t)); SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(t));
    SetPlayerPos(playerid, x + 1.0, y, z);
    return 1;
}

CMD:gethere(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/gethere [id]"); if (t == INVALID_PLAYER_ID) return 1;
    new Float:x, Float:y, Float:z; GetPlayerPos(playerid, x, y, z);
    SetPlayerInterior(t, GetPlayerInterior(playerid)); SetPlayerVirtualWorld(t, GetPlayerVirtualWorld(playerid));
    SetPlayerPos(t, x + 1.0, y, z);
    Info(t, "Вас телепортировал администратор.");
    return 1;
}

CMD:tp(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new code[24]; GetParam(params, 0, code);
    if (isnull(code) || Loc_Find(code) == -1) return Err(playerid, "/tp [код точки] — список: /locs");
    Loc_Teleport(playerid, code);
    return 1;
}

CMD:locs(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new line[144]; line[0] = 0;
    for (new i = 0; i < gLocCount; i++)
    {
        if (strlen(line) + strlen(gLoc[i][lCode]) > 130) { SCM(playerid, C_GREY, line); line[0] = 0; }
        strcat(line, gLoc[i][lCode]); strcat(line, " ");
    }
    if (line[0]) SCM(playerid, C_GREY, line);
    return 1;
}

CMD:setloc(playerid, const params[])
{
    if (!AdmCheck(playerid, 4)) return 1;
    new code[24]; GetParam(params, 0, code);
    if (isnull(code)) return Err(playerid, "/setloc [код] — записать текущую позицию в точку (вступит в силу после /gmx)");
    for (new i = 0; code[i]; i++) if (!((code[i] >= 'a' && code[i] <= 'z') || (code[i] >= '0' && code[i] <= '9') || code[i] == '_')) return Err(playerid, "Код: латиница, цифры, _");
    new Float:x, Float:y, Float:z, Float:a, q[256];
    GetPlayerPos(playerid, x, y, z); GetPlayerFacingAngle(playerid, a);
    if (IsPlayerInAnyVehicle(playerid)) GetVehicleZAngle(GetPlayerVehicleID(playerid), a);
    format(q, sizeof q, "INSERT OR IGNORE INTO locations (code,title) VALUES ('%s','%s')", code, code);
    DB_Exec(q);
    format(q, sizeof q, "UPDATE locations SET x=%f,y=%f,z=%f,a=%f,interior=%d,vw=%d WHERE code='%s'",
        x, y, z, a, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid), code);
    DB_Exec(q);
    new k = Loc_Find(code);
    if (k != -1) { gLoc[k][lX] = x; gLoc[k][lY] = y; gLoc[k][lZ] = z; gLoc[k][lA] = a; gLoc[k][lInt] = GetPlayerInterior(playerid); gLoc[k][lVW] = GetPlayerVirtualWorld(playerid); }
    new s[128]; format(s, sizeof s, "точка %s -> %.2f, %.2f, %.2f (int %d)", code, x, y, z, GetPlayerInterior(playerid));
    Adm_Log(playerid, s);
    Ok(playerid, "Точка сохранена. Метки/пикапы обновятся после /gmx.");
    return 1;
}

CMD:spec(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    new t = Adm_Target(playerid, params, "/spec [id]"); if (t == INVALID_PLAYER_ID || t == playerid) return 1;
    if (P[playerid][pSpecTarget] == 0) { P[playerid][pPrevInt] = GetPlayerInterior(playerid); P[playerid][pPrevVW] = GetPlayerVirtualWorld(playerid); }
    P[playerid][pSpecTarget] = t + 1;
    TogglePlayerSpectating(playerid, true);
    SetPlayerInterior(playerid, GetPlayerInterior(t)); SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(t));
    if (IsPlayerInAnyVehicle(t)) PlayerSpectateVehicle(playerid, GetPlayerVehicleID(t));
    else PlayerSpectatePlayer(playerid, t);
    new s[64]; format(s, sizeof s, "Слежка за %s. /specoff — выход.", PName(t)); Info(playerid, s);
    return 1;
}

CMD:specoff(playerid, const params[])
{
    if (!P[playerid][pSpecTarget]) return 1;
    P[playerid][pSpecTarget] = 0;
    SetPlayerInterior(playerid, P[playerid][pPrevInt]); SetPlayerVirtualWorld(playerid, P[playerid][pPrevVW]);
    TogglePlayerSpectating(playerid, false);
    return 1;
}

CMD:kick(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    new t = Adm_Target(playerid, params, "/kick [id] [причина]"); if (t == INVALID_PLAYER_ID) return 1;
    new r[64]; GetRest(params, 1, r); if (isnull(r)) r = "не указана";
    new s[160]; format(s, sizeof s, "Администратор %s кикнул %s. Причина: %s", PName(playerid), PName(t), r);
    SendClientMessageToAll(C_ADMIN, s);
    Log("admin", PName(playerid), s);
    KickEx(t);
    return 1;
}

CMD:mute(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    new t = Adm_Target(playerid, params, "/mute [id] [минут] [причина]"); if (t == INVALID_PLAYER_ID) return 1;
    new m = GetParamInt(params, 1, 0), r[64]; GetRest(params, 2, r);
    if (m < 1 || m > 600) return Err(playerid, "Время: 1–600 минут.");
    P[t][pMute] = m;
    new s[160]; format(s, sizeof s, "Администратор %s выдал молчанку %s на %d мин. Причина: %s", PName(playerid), PName(t), m, isnull(r) ? ("—") : r);
    SendClientMessageToAll(C_ADMIN, s); Log("admin", PName(playerid), s);
    return 1;
}

CMD:unmute(playerid, const params[])
{
    if (!AdmCheck(playerid, 1)) return 1;
    new t = Adm_Target(playerid, params, "/unmute [id]"); if (t == INVALID_PLAYER_ID) return 1;
    P[t][pMute] = 0; Ok(t, "С вас снята молчанка.");
    Adm_Log(playerid, "снял молчанку");
    return 1;
}

CMD:jail(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/jail [id] [минут] [причина]"); if (t == INVALID_PLAYER_ID) return 1;
    new m = GetParamInt(params, 1, 0), r[64]; GetRest(params, 2, r);
    if (m < 1 || m > 600) return Err(playerid, "Время: 1–600 минут.");
    P[t][pJail] = m; ResetPlayerWeapons(t); Loc_Teleport(t, "jail");
    new s[160]; format(s, sizeof s, "Администратор %s посадил %s в деморган на %d мин. Причина: %s", PName(playerid), PName(t), m, isnull(r) ? ("—") : r);
    SendClientMessageToAll(C_ADMIN, s); Log("admin", PName(playerid), s);
    return 1;
}

CMD:unjail(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/unjail [id]"); if (t == INVALID_PLAYER_ID) return 1;
    P[t][pJail] = 0; Loc_Teleport(t, "jail_out"); Ok(t, "Вас освободили.");
    Adm_Log(playerid, "освободил из тюрьмы");
    return 1;
}

CMD:warn(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/warn [id] [причина]"); if (t == INVALID_PLAYER_ID) return 1;
    new r[64]; GetRest(params, 1, r); if (isnull(r)) r = "нарушение правил";
    P[t][pWarns]++; P_SetInt(t, "warns", P[t][pWarns]);
    new s[180]; format(s, sizeof s, "Администратор %s выдал предупреждение %s (%d/3). Причина: %s", PName(playerid), PName(t), P[t][pWarns], r);
    SendClientMessageToAll(C_ADMIN, s); Log("admin", PName(playerid), s);
    if (P[t][pWarns] >= 3)
    {
        new ip[16]; GetPlayerIp(t, ip, sizeof ip);
        P[t][pWarns] = 0; P_SetInt(t, "warns", 0);
        Ban_Add(PName(t), "", 7 * 24 * 60, "3/3 предупреждений", PName(playerid));
        SCM(t, C_RED, "3/3 предупреждений — блокировка на 7 дней.");
        KickEx(t);
    }
    return 1;
}

CMD:ban(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new t = Adm_Target(playerid, params, "/ban [id] [дней, 0=навсегда] [причина]"); if (t == INVALID_PLAYER_ID) return 1;
    new d = GetParamInt(params, 1, -1), r[64]; GetRest(params, 2, r);
    if (d < 0 || d > 3650) return Err(playerid, "/ban [id] [дней, 0=навсегда] [причина]");
    if (isnull(r)) r = "нарушение правил";
    new ip[16]; GetPlayerIp(t, ip, sizeof ip);
    Ban_Add(PName(t), d == 0 ? ip : (""), d * 24 * 60, r, PName(playerid));
    new s[180];
    if (d) format(s, sizeof s, "Администратор %s заблокировал %s на %d дн. Причина: %s", PName(playerid), PName(t), d, r);
    else format(s, sizeof s, "Администратор %s заблокировал %s навсегда. Причина: %s", PName(playerid), PName(t), r);
    SendClientMessageToAll(C_ADMIN, s); Log("admin", PName(playerid), s);
    KickEx(t);
    return 1;
}

CMD:offban(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new name[MAX_PLAYER_NAME], r[64]; GetParam(params, 0, name);
    new d = GetParamInt(params, 1, -1); GetRest(params, 2, r);
    if (isnull(name) || d < 0) return Err(playerid, "/offban [Имя_Фамилия] [дней, 0=навсегда] [причина]");
    if (isnull(r)) r = "нарушение правил";
    Ban_Add(name, "", d * 24 * 60, r, PName(playerid));
    new s[128]; format(s, sizeof s, "оффлайн-бан %s на %d дн.: %s", name, d, r); Adm_Log(playerid, s);
    return 1;
}

CMD:unban(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new name[MAX_PLAYER_NAME]; GetParam(params, 0, name);
    if (isnull(name)) return Err(playerid, "/unban [Имя_Фамилия]");
    new q[128]; format(q, sizeof q, "DELETE FROM bans WHERE name='%s'", SqlEsc(name)); DB_Exec(q);
    new s[64]; format(s, sizeof s, "разбанил %s", name); Adm_Log(playerid, s);
    return 1;
}

CMD:hp(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/hp [id] [0-100]"); if (t == INVALID_PLAYER_ID) return 1;
    new v = GetParamInt(params, 1, 100); if (v < 0 || v > 100) v = 100;
    SetPlayerHealth(t, float(v));
    if (P[t][pInjured]) Hospital_Clear(t);
    Adm_Log(playerid, "изменил здоровье");
    return 1;
}

CMD:slap(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/slap [id]"); if (t == INVALID_PLAYER_ID) return 1;
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z); SetPlayerPos(t, x, y, z + 5.0);
    PlayerPlaySound(t, 1130, 0.0, 0.0, 0.0);
    return 1;
}

CMD:freeze(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/freeze [id]"); if (t == INVALID_PLAYER_ID) return 1;
    TogglePlayerControllable(t, false); Info(t, "Вас заморозил администратор.");
    return 1;
}

CMD:unfreeze(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new t = Adm_Target(playerid, params, "/unfreeze [id]"); if (t == INVALID_PLAYER_ID) return 1;
    TogglePlayerControllable(t, true);
    return 1;
}

new gAdmVeh[MAX_VEHICLES];

CMD:veh(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new model = GetParamInt(params, 0, 0), c1 = GetParamInt(params, 1, 1), c2 = GetParamInt(params, 2, 1);
    if (model < 400 || model > 611) return Err(playerid, "/veh [модель 400-611] [цвет1] [цвет2]");
    new Float:x, Float:y, Float:z, Float:a; GetPlayerPos(playerid, x, y, z); GetPlayerFacingAngle(playerid, a);
    new v = CreateVehicle(model, x, y, z + 0.5, a, c1, c2, -1);
    if (v == INVALID_VEHICLE_ID || v >= MAX_VEHICLES) return Err(playerid, "Лимит транспорта.");
    Veh_Reset(v); Veh_SetFuel(v, 100); gAdmVeh[v] = 1;
    LinkVehicleToInterior(v, GetPlayerInterior(playerid)); SetVehicleVirtualWorld(v, GetPlayerVirtualWorld(playerid));
    PutPlayerInVehicle(playerid, v, 0);
    new s[48]; format(s, sizeof s, "создал транспорт %d (id %d)", model, v); Adm_Log(playerid, s);
    return 1;
}

CMD:dveh(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new v = GetPlayerVehicleID(playerid);
    if (!v) v = GetParamInt(params, 0, 0);
    if (v <= 0 || v >= MAX_VEHICLES || !gAdmVeh[v]) return Err(playerid, "Можно удалить только админ-транспорт (/veh). Сядьте в него или укажите id.");
    DestroyVehicle(v); gAdmVeh[v] = 0;
    return 1;
}

CMD:fixveh(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new v = GetPlayerVehicleID(playerid); if (!v) return Err(playerid, "Сядьте в транспорт.");
    RepairVehicle(v); Veh_SetFuel(v, 100);
    return 1;
}

CMD:setskin(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new t = Adm_Target(playerid, params, "/setskin [id] [скин]"); if (t == INVALID_PLAYER_ID) return 1;
    new sk = GetParamInt(params, 1, -1); if (sk < 0 || sk > 311 || sk == 74) return Err(playerid, "Скин 0–311.");
    SetPlayerSkin(t, sk); P[t][pSkin] = sk; P_SetInt(t, "skin", sk);
    Adm_Log(playerid, "сменил скин игроку");
    return 1;
}

CMD:check(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new t = Adm_Target(playerid, params, "/check [id]"); if (t == INVALID_PLAYER_ID) return 1;
    new s[180], ip[16]; GetPlayerIp(t, ip, sizeof ip);
    format(s, sizeof s, "%s [acc %d] ур.%d | нал. %s | банк %s | орг %d/%d | розыск %d | варны %d | IP %s",
        PName(t), P[t][pID], P[t][pLevel], FormatMoney(P[t][pCash]), FormatMoney(P[t][pBank]), P[t][pOrg], P[t][pOrgRank], P[t][pWanted], P[t][pWarns], ip);
    SCM(playerid, C_ADMIN, s);
    return 1;
}

CMD:givemoney(playerid, const params[])
{
    if (!AdmCheck(playerid, 4)) return 1;
    new t = Adm_Target(playerid, params, "/givemoney [id] [сумма]"); if (t == INVALID_PLAYER_ID) return 1;
    new m = GetParamInt(params, 1, 0); if (m == 0 || m > 10000000 || m < -10000000) return Err(playerid, "Сумма до 10 000 000.");
    if (m > 0) Money_Give(t, m, "admin"); else Money_Take(t, -m, "admin");
    new s[64]; format(s, sizeof s, "выдал %s %d$", PName(t), m); Adm_Log(playerid, s);
    return 1;
}

CMD:setlevel(playerid, const params[])
{
    if (!AdmCheck(playerid, 4)) return 1;
    new t = Adm_Target(playerid, params, "/setlevel [id] [уровень]"); if (t == INVALID_PLAYER_ID) return 1;
    new l = GetParamInt(params, 1, 0); if (l < 1 || l > 100) return Err(playerid, "Уровень 1–100.");
    P[t][pLevel] = l; P[t][pExp] = 0; SetPlayerScore(t, l); P_Save(t, false);
    Adm_Log(playerid, "изменил уровень игроку");
    return 1;
}

CMD:setleader(playerid, const params[])
{
    if (!AdmCheck(playerid, 4)) return 1;
    new t = Adm_Target(playerid, params, "/setleader [id] [орг 0-8]"); if (t == INVALID_PLAYER_ID) return 1;
    new o = GetParamInt(params, 1, -1); if (o < 0 || o >= MAX_ORGS) return Err(playerid, "Организация 0–8 (0 — снять).");
    Org_SetMember(t, o, o ? 6 : 0);
    new s[96]; format(s, sizeof s, "назначил %s лидером орг. %d", PName(t), o); Adm_Log(playerid, s);
    if (o) { format(s, sizeof s, "Вы назначены лидером: %s. /lmenu", Org_Name(o)); Ok(t, s); }
    return 1;
}

CMD:giveitem(playerid, const params[])
{
    if (!AdmCheck(playerid, 4)) return 1;
    new t = Adm_Target(playerid, params, "/giveitem [id] [ключ предмета] [кол-во]"); if (t == INVALID_PLAYER_ID) return 1;
    new key[16]; GetParam(params, 1, key);
    new it = Item_ByKey(key), n = GetParamInt(params, 2, 1);
    if (it <= 0 || n < 1) return Err(playerid, "Неизвестный предмет. Ключи: bottle, rod, bait, detector, shovel, medkit...");
    Inv_Add(t, it, n);
    Adm_Log(playerid, "выдал предмет");
    return 1;
}

CMD:setadmin(playerid, const params[])
{
    if (!AdmCheck(playerid, 5)) return 1;
    new t = GetParamPlayer(params, 0), l = GetParamInt(params, 1, -1);
    if (t == INVALID_PLAYER_ID || !IsLogged(t) || l < 0 || l > 5) return Err(playerid, "/setadmin [id] [0-5]");
    P[t][pAdmin] = l; P_SetInt(t, "admin", l);
    new s[64]; format(s, sizeof s, "выдал %s админ-уровень %d", PName(t), l); Adm_Log(playerid, s);
    return 1;
}

CMD:settreasury(playerid, const params[])
{
    if (!AdmCheck(playerid, 5)) return 1;
    gTreasury = GetParamInt(params, 0, 0);
    Adm_Log(playerid, "изменил казну");
    return 1;
}

CMD:gmx(playerid, const params[])
{
    if (!AdmCheck(playerid, 5)) return 1;
    SendClientMessageToAll(C_ADMIN, "Сервер перезапускается через 5 секунд, данные сохранены.");
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i)) P_Save(i);
    SetTimer("Adm_Gmx", 5000, false);
    return 1;
}

forward Adm_Gmx();
public Adm_Gmx() SendRconCommand("gmx");
