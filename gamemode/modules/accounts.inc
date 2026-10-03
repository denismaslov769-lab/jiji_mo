// =====================================================================
//  Аккаунты: вход, регистрация, создание персонажа (всё на CEF)
// =====================================================================
static const gSkinsM[] = {78, 79, 230};
static const gSkinsF[] = {77, 130, 196};

stock HashPass(const pass[], const salt[], out[65])
{
    new buf[160];
    SHA256_PassHash(pass, salt, out, 65);
    for (new i = 0; i < HASH_ROUNDS; i++)
    {
        format(buf, sizeof buf, "%s%s%d", out, pass, i);
        SHA256_PassHash(buf, salt, out, 65);
    }
}

stock Ban_Check(playerid)
{
    new q[256], ip[16];
    GetPlayerIp(playerid, ip, sizeof ip);
    format(q, sizeof q, "SELECT reason,until,admin FROM bans WHERE (name='%s' OR ip='%s') AND (until=0 OR until>%d) LIMIT 1", SqlEsc(PName(playerid)), ip, gettime());
    new DBResult:r = DB_Query(q);
    if (!r) return 0;
    new banned = 0;
    if (db_num_rows(r))
    {
        new reason[64], adm[24], s[144];
        DB_GetStr(r, "reason", reason); DB_GetStr(r, "admin", adm);
        new until = DB_GetInt(r, "until");
        if (until == 0) format(s, sizeof s, "Вы заблокированы навсегда. Причина: %s (%s)", reason, adm);
        else format(s, sizeof s, "Вы заблокированы ещё на %d мин. Причина: %s (%s)", (until - gettime()) / 60 + 1, reason, adm);
        SCM(playerid, C_RED, s);
        banned = 1;
    }
    db_free_result(r);
    return banned;
}

stock Ban_Add(const name[], const ip[], minutes, const reason[], const admin[])
{
    new q[300];
    format(q, sizeof q, "INSERT INTO bans (name,ip,until,reason,admin) VALUES ('%s','%s',%d,'%s','%s')",
        SqlEsc(name), ip, minutes > 0 ? gettime() + minutes * 60 : 0, SqlEsc(reason), SqlEsc(admin));
    DB_Exec(q);
}

forward bool:Agent_IsReservedName(const name[]);
forward DelayedKick(playerid);
public DelayedKick(playerid) Kick(playerid);
stock KickEx(playerid, delay = 500) { SetTimerEx("DelayedKick", delay, false, "i", playerid); return 1; }

stock Auth_OnConnect(playerid)
{
    P_Reset(playerid);
    Inv_Clear(playerid);
    if (Ban_Check(playerid)) return KickEx(playerid);
    if (!IsRpName(PName(playerid)))
    {
        SCM(playerid, C_RED, "Ник должен быть в формате Имя_Фамилия (например, Ivan_Petrov).");
        SCM(playerid, C_RED, "Смените ник в лаунчере Godjo RP и перезайдите.");
        return KickEx(playerid, 1500);
    }
    SetPlayerColor(playerid, 0xFFFFFF00);
    SCM(playerid, C_GREY, "Добро пожаловать на " GM_NAME ". Загружаем интерфейс...");
    P[playerid][pLoginTimer] = SetTimerEx("Auth_Timeout", LOGIN_TIMEOUT_SEC * 1000, false, "i", playerid);
    return 1;
}

forward Auth_Timeout(playerid);
public Auth_Timeout(playerid)
{
    P[playerid][pLoginTimer] = -1;
    if (!P[playerid][pLogged])
    {
        SCM(playerid, C_RED, "Время на авторизацию истекло.");
        KickEx(playerid);
    }
}

public OnCefInitialize(player_id, success)
{
    if (IsPlayerNPC(player_id)) return 1;   // у NPC (FCNPC) нет CEF-клиента — не кикаем
    if (!success)
    {
        SCM(player_id, C_RED, "Не найден CEF-плагин Godjo RP. Запускайте игру через лаунчер GodjoLauncher.exe.");
        SCM(player_id, C_GREY, "Лаунчер сам установит плагин и интерфейсы.");
        KickEx(player_id, 3000);
        return 1;
    }
    cef_create_browser(player_id, CEF_BROWSER_ID, CEF_URL, false, false);
    return 1;
}

public OnCefBrowserCreated(player_id, browser_id, status_code)
{
    if (IsPlayerNPC(player_id)) return 1;
    if (browser_id != CEF_BROWSER_ID) return 1;
    if (status_code == 0)
    {
        SCM(player_id, C_RED, "Не удалось загрузить интерфейс. Проверьте файлы игры в лаунчере.");
        KickEx(player_id, 3000);
        return 1;
    }
    P[player_id][pCef] = true;
    cef_always_listen_keys(player_id, CEF_BROWSER_ID, true);
    Auth_Show(player_id);
    return 1;
}

stock Auth_Show(playerid)
{
    new q[160], j[256], registered = 0;
    if (Agent_IsReservedName(PName(playerid)))
    {
        SCM(playerid, C_RED, "Это имя занято жителем города. Зайдите под другим ником (Имя_Фамилия).");
        SetTimerEx("DelayedKick", 500, false, "i", playerid);
        return;
    }
    format(q, sizeof q, "SELECT * FROM accounts WHERE name='%s' LIMIT 1", SqlEsc(PName(playerid)));
    new DBResult:r = DB_Query(q);
    if (r)
    {
        if (db_num_rows(r)) { P_Load(playerid, r); registered = 1; }
        db_free_result(r);
    }
    format(j, sizeof j, "{\"name\":\"%s\",\"registered\":%s,\"server\":\"" GM_NAME "\",\"online\":%d}",
        PName(playerid), registered ? ("true") : ("false"), Iter_Online());
    UI_Open(playerid, "auth", j);
}

UI:auth(playerid, const a[])
{
    if (P[playerid][pLogged]) return 1;
    new act[12];
    CefField(a, 1, act);
    if (StrEq(act, "login"))
    {
        if (!P[playerid][pID]) return 1;
        new pass[48], hash[65];
        CefField(a, 2, pass);
        HashPass(pass, P[playerid][pSalt], hash);
        if (strcmp(hash, P[playerid][pPass], false) != 0 || !pass[0])
        {
            P[playerid][pLoginTries]++;
            new left = MAX_LOGIN_TRIES - P[playerid][pLoginTries];
            if (left <= 0)
            {
                new ip[16]; GetPlayerIp(playerid, ip, sizeof ip);
                Ban_Add("", ip, 15, "Подбор пароля", "Система");
                Log("auth", PName(playerid), "bruteforce ip-ban 15 min");
                UI_Data(playerid, "auth_error", "{\"code\":\"banned\"}");
                return KickEx(playerid, 1500);
            }
            new j[64]; format(j, sizeof j, "{\"code\":\"wrong\",\"left\":%d}", left);
            UI_Data(playerid, "auth_error", j);
            return 1;
        }
        Auth_Success(playerid);
    }
    else if (StrEq(act, "register"))
    {
        if (P[playerid][pID]) return 1;
        new pass[48], pass2[48], email[64], ref[MAX_PLAYER_NAME];
        CefField(a, 2, pass); CefField(a, 3, pass2); CefField(a, 4, email); CefField(a, 5, ref);
        new plen = strlen(pass);
        if (plen < 6 || plen > 32) return UI_Data(playerid, "auth_error", "{\"code\":\"pass_len\"}");
        if (strcmp(pass, pass2, false) != 0) return UI_Data(playerid, "auth_error", "{\"code\":\"pass_match\"}");
        if (!IsValidEmail(email)) return UI_Data(playerid, "auth_error", "{\"code\":\"email\"}");
        if (ref[0])
        {
            new q[128];
            format(q, sizeof q, "SELECT COUNT(*) FROM accounts WHERE name='%s'", SqlEsc(ref));
            if (DB_ScalarInt(q) == 0) return UI_Data(playerid, "auth_error", "{\"code\":\"ref\"}");
            if (StrEq(ref, PName(playerid))) return UI_Data(playerid, "auth_error", "{\"code\":\"ref\"}");
        }
        new salt[17], hash[65], ip[16], q[700];
        RandomString(salt, 16);
        HashPass(pass, salt, hash);
        GetPlayerIp(playerid, ip, sizeof ip);
        format(q, sizeof q, "INSERT INTO accounts (name,pass,salt,email,ref_by,reg_ip,reg_date,last_ip,cash) VALUES ('%s','%s','%s','%s','%s','%s','%s','%s',%d)",
            SqlEsc(PName(playerid)), hash, salt, SqlEsc(email), SqlEsc(ref), ip, DateStr(), ip, START_CASH);
        DB_Exec(q);
        format(q, sizeof q, "SELECT * FROM accounts WHERE name='%s' LIMIT 1", SqlEsc(PName(playerid)));
        new DBResult:r = DB_Query(q);
        if (r) { if (db_num_rows(r)) P_Load(playerid, r); db_free_result(r); }
        if (!P[playerid][pID]) return UI_Data(playerid, "auth_error", "{\"code\":\"db\"}");
        Log("auth", PName(playerid), "register");
        Auth_Success(playerid);
    }
    return 1;
}

stock bool:IsValidEmail(const e[])
{
    new len = strlen(e), at = -1, dot = -1;
    if (len < 5 || len > 60) return false;
    for (new i = 0; i < len; i++)
    {
        new c = e[i];
        if (c == '@') { if (at != -1) return false; at = i; }
        else if (c == '.' && at != -1) dot = i;
        else if (c <= ' ' || c == '\'' || c == '"' || c > 126) return false;
    }
    return at > 0 && dot > at + 1 && dot < len - 1;
}

stock Auth_Success(playerid)
{
    if (P[playerid][pLoginTimer] != -1) { KillTimer(P[playerid][pLoginTimer]); P[playerid][pLoginTimer] = -1; }
    P[playerid][pLogged] = true;
    new q[200], ip[16];
    GetPlayerIp(playerid, ip, sizeof ip);
    format(q, sizeof q, "UPDATE accounts SET last_login='%s',last_ip='%s' WHERE id=%d", DateStr(), ip, P[playerid][pID]);
    DB_Exec(q);
    Inv_Load(playerid);
    Quest_Load(playerid);
    Items_SendCatalog(playerid);
    UI_Close(playerid, "auth");
    if (!P[playerid][pCharDone]) return Char_Begin(playerid);
    new s[128];
    format(s, sizeof s, "С возвращением, %s! Уровень: %d, наличные: %s.", PName(playerid), P[playerid][pLevel], FormatMoney(P[playerid][pCash]));
    SCM(playerid, C_GREEN, s);
    if (P[playerid][pAdmin]) { format(s, sizeof s, "Вы вошли как администратор %d уровня.", P[playerid][pAdmin]); SCM(playerid, C_ADMIN, s); }
    Spawn_Prepare(playerid);
    Veh_LoadForPlayer(playerid);
    TogglePlayerSpectating(playerid, false);
    return 1;
}

// ---------- создание персонажа ----------
stock Char_Begin(playerid)
{
    P[playerid][pCreateStep] = 1;
    new Float:x, Float:y, Float:z, Float:a;
    Loc_Get("shelter", x, y, z, a);
    SetSpawnInfo(playerid, 0, gSkinsM[0], x, y, z, a, 0, 0, 0, 0, 0, 0);
    P[playerid][pSkin] = gSkinsM[0];
    TogglePlayerSpectating(playerid, false);
    return 1;
}

stock Char_OnSpawn(playerid)
{
    new Float:x, Float:y, Float:z, Float:a;
    Loc_Get("shelter", x, y, z, a);
    SetPlayerVirtualWorld(playerid, VW_PREVIEW_BASE + playerid);
    SetPlayerInterior(playerid, 0);
    SetPlayerPos(playerid, x, y, z);
    SetPlayerSkin(playerid, P[playerid][pSkin]);
    TogglePlayerControllable(playerid, false);
    P[playerid][pPreviewAng] = 0.0;
    if (P[playerid][pPreviewTimer] != -1) KillTimer(P[playerid][pPreviewTimer]);
    P[playerid][pPreviewTimer] = SetTimerEx("Char_Orbit", 100, true, "i", playerid);
    new j[512];
    format(j, sizeof j, "{\"m\":[%d,%d,%d],\"f\":[%d,%d,%d],\"name\":\"%s\"}", gSkinsM[0], gSkinsM[1], gSkinsM[2], gSkinsF[0], gSkinsF[1], gSkinsF[2], PName(playerid));
    UI_Open(playerid, "char", j);
    return 1;
}

forward Char_Orbit(playerid);
public Char_Orbit(playerid)
{
    if (!IsPlayerConnected(playerid) || P[playerid][pCharDone]) return;
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new Float:a1 = P[playerid][pPreviewAng];
    P[playerid][pPreviewAng] += 2.0;
    if (P[playerid][pPreviewAng] >= 360.0) P[playerid][pPreviewAng] -= 360.0;
    new Float:a2 = P[playerid][pPreviewAng];
    InterpolateCameraPos(playerid, x + 3.2 * floatcos(a1, degrees), y + 3.2 * floatsin(a1, degrees), z + 0.7,
        x + 3.2 * floatcos(a2, degrees), y + 3.2 * floatsin(a2, degrees), z + 0.7, 100, CAMERA_MOVE);
    InterpolateCameraLookAt(playerid, x, y, z + 0.2, x, y, z + 0.2, 100, CAMERA_MOVE);
}

UI:char(playerid, const a[])
{
    if (P[playerid][pCharDone] || !P[playerid][pLogged]) return 1;
    new act[8];
    CefField(a, 1, act);
    new sex = CefFieldInt(a, 2) ? 1 : 0, idx = CefFieldInt(a, 3);
    if (idx < 0 || idx > 2) idx = 0;
    new skin = sex ? gSkinsF[idx] : gSkinsM[idx];
    if (StrEq(act, "skin"))
    {
        P[playerid][pSkin] = skin;
        P[playerid][pSex] = sex;
        SetPlayerSkin(playerid, skin);
        ApplyAnimation(playerid, "PED", "IDLE_CHAT", 4.0, 0, 0, 0, 0, 0);
    }
    else if (StrEq(act, "done"))
    {
        new age = CefFieldInt(a, 4), origin = CefFieldInt(a, 5);
        if (age < 18 || age > 70) return UI_Data(playerid, "char_error", "{\"code\":\"age\"}");
        if (origin < 0 || origin > 3) origin = 0;
        P[playerid][pSex] = sex; P[playerid][pSkin] = skin; P[playerid][pAge] = age; P[playerid][pOrigin] = origin;
        P[playerid][pCharDone] = 1;
        P[playerid][pLevel] = 1;
        P[playerid][pHunger] = 45.0;
        P[playerid][pFatigue] = 70.0;
        if (P[playerid][pPreviewTimer] != -1) { KillTimer(P[playerid][pPreviewTimer]); P[playerid][pPreviewTimer] = -1; }
        Phone_AssignNumber(playerid);
        P_Save(playerid, false);
        UI_Close(playerid, "char");
        Ach_Give(playerid, "born", "Новая жизнь");
        Spawn_Prepare(playerid);
        P[playerid][pCreateStep] = 0;
        SpawnPlayer(playerid);
        SetTimerEx("Char_Intro", 1500, false, "i", playerid);
        Log("auth", PName(playerid), "character created");
    }
    return 1;
}

forward Char_Intro(playerid);
public Char_Intro(playerid)
{
    SCM(playerid, C_YELLOW, "Холодная ночь. Ты проснулся под эстакадой у вокзала Юнити: ни денег, ни документов.");
    SCM(playerid, C_YELLOW, "Неподалёку у бочки греется старик. Подойди к нему и нажми ALT.");
    Info(playerid, "Подойдите к Дяде Вове и нажмите ALT");
}

// ---------- точка появления ----------
stock Spawn_Prepare(playerid)
{
    new Float:x, Float:y, Float:z, Float:a, it = 0, vw = 0, skin = P[playerid][pSkin];
    if (P[playerid][pOrgSkin] && P[playerid][pDuty]) skin = P[playerid][pOrgSkin];
    if (P[playerid][pJail] > 0) Loc_Get("jail", x, y, z, a, it, vw);
    else if (P[playerid][pInjured]) Loc_Get("hospital_spawn", x, y, z, a, it, vw);
    else
    {
        new q[128];
        format(q, sizeof q, "SELECT x,y,z,a,interior,vw FROM accounts WHERE id=%d", P[playerid][pID]);
        new DBResult:r = DB_Query(q);
        if (r)
        {
            if (db_num_rows(r))
            {
                x = DB_GetFloat(r, "x"); y = DB_GetFloat(r, "y"); z = DB_GetFloat(r, "z"); a = DB_GetFloat(r, "a");
                it = DB_GetInt(r, "interior"); vw = DB_GetInt(r, "vw");
            }
            db_free_result(r);
        }
        if ((x == 0.0 && y == 0.0) || vw >= VW_PREVIEW_BASE && vw < VW_PREVIEW_BASE + MAX_PLAYERS) { Loc_Get("shelter", x, y, z, a, it, vw); }
    }
    SetSpawnInfo(playerid, 0, skin, x, y, z, a, 0, 0, 0, 0, 0, 0);
    P[playerid][pPrevInt] = it;
    P[playerid][pPrevVW] = vw;
}

stock Spawn_Apply(playerid)
{
    SetPlayerInterior(playerid, P[playerid][pPrevInt]);
    SetPlayerVirtualWorld(playerid, P[playerid][pPrevVW]);
    SetPlayerSkin(playerid, (P[playerid][pDuty] && P[playerid][pOrgSkin]) ? P[playerid][pOrgSkin] : P[playerid][pSkin]);
    SetCameraBehindPlayer(playerid);
    TogglePlayerControllable(playerid, true);
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, P[playerid][pCash]);
    SetPlayerScore(playerid, P[playerid][pLevel]);
    Acc_Apply(playerid);
    World_MapIcons(playerid);
    Quest_UpdateTracker(playerid);
    Police_UpdateWanted(playerid);
    UI_Hud(playerid);
    if (P[playerid][pJail] > 0)
    {
        new s[96]; format(s, sizeof s, "Вы в КПЗ. Осталось: %d мин.", P[playerid][pJail]); Info(playerid, s);
    }
    if (P[playerid][pInjured])
    {
        P[playerid][pInjured] = 0;
        SetPlayerHealth(playerid, 60.0);
        Info(playerid, "Вас доставили в больницу. Лечение оплачено с вашего счёта.");
    }
}
