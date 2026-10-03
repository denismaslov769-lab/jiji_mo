// =====================================================================
//  Главные колбэки SA-MP и таймеры
// =====================================================================
new gTimerSec, gTimerMin;

stock Treasury_Load() { gTreasury = DB_ScalarInt("SELECT v FROM meta WHERE k='treasury'", 0); }
stock Treasury_Save() { new q[96]; format(q, sizeof q, "INSERT OR REPLACE INTO meta VALUES ('treasury','%d')", gTreasury); DB_Exec(q); }

public OnGameModeInit()
{
    print("\n=============================================");
    print("  " GM_NAME " v" GM_VERSION);
    print("=============================================\n");
    SetGameModeText(GM_SHORT " " GM_VERSION);
    SendRconCommand("language Русский");
    SendRconCommand("mapname Los Santos");
    UsePlayerPedAnims();
    DisableInteriorEnterExits();
    EnableStuntBonusForAll(false);
    ShowPlayerMarkers(PLAYER_MARKERS_MODE_STREAMED);
    SetNameTagDrawDistance(20.0);
    ManualVehicleEngineAndLights();
    AddPlayerClass(78, 1759.0, -1898.0, 13.56, 270.0, 0, 0, 0, 0, 0, 0);

    DB_Init();
    Loc_Load();
    Treasury_Load();
    Quest_LoadAll();
    Org_Load();
    World_Build();
    Houses_Load();
    Biz_Load();
    Jobs_Init();
    Rent_Init();
    Bottles_Init();
    Vova_Create();
    Extras_Init();

    cef_subscribe("gj", "OnCefGj");
    gTimerSec = SetTimer("Timer_Second", 1000, true);
    gTimerMin = SetTimer("Timer_Minute", 60000, true);
    print("[GODJO] Мод загружен.");
    return 1;
}

public OnGameModeExit()
{
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i)) { P_Save(i); Veh_UnloadForPlayer(i); }
    Treasury_Save();
    KillTimer(gTimerSec); KillTimer(gTimerMin);
    db_close(gDB);
    return 1;
}

forward Timer_Second();
public Timer_Second()
{
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsLogged(i)) continue;
        Treasure_Tick(i);
        if (P[i][pSpecTarget] && !IsPlayerConnected(P[i][pSpecTarget] - 1)) cmd_specoff(i, "\1");
        UI_Hud(i);
    }
    Veh_FuelTick();
}

forward Timer_Minute();
public Timer_Minute()
{
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsLogged(i) || P[i][pCreateStep]) continue;
        P[i][pPlayed]++;
        P[i][pMinuteTick]++;
        if (P[i][pMute] > 0 && --P[i][pMute] == 0) Ok(i, "Срок молчанки истёк.");
        Needs_Tick(i);
        Jail_Tick(i);
        if (++P[i][pSaveTick] >= 5) { P[i][pSaveTick] = 0; P_Save(i); }
    }
    Payday_Check();
    Rent_Minute();
    Drops_Cleanup();
    Treasury_Save();
    Extras_Minute();
}

public OnPlayerConnect(playerid)
{
    if (IsPlayerNPC(playerid)) return 1;
    Auth_OnConnect(playerid);
    RemoveBuildingForPlayer(playerid, 1302, 0.0, 0.0, 0.0, 6000.0); // автоматы с газировкой
    RemoveBuildingForPlayer(playerid, 1209, 0.0, 0.0, 0.0, 6000.0);
    RemoveBuildingForPlayer(playerid, 955, 0.0, 0.0, 0.0, 6000.0);
    RemoveBuildingForPlayer(playerid, 956, 0.0, 0.0, 0.0, 6000.0);
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    if (P[playerid][pLogged] && !P[playerid][pCreateStep]) { Spawn_Prepare(playerid); SpawnPlayer(playerid); return 1; }
    TogglePlayerSpectating(playerid, true);
    SetPlayerCameraPos(playerid, 1696.0, -1950.0, 60.0);
    SetPlayerCameraLookAt(playerid, 1759.0, -1898.0, 13.5);
    return 1;
}

public OnPlayerRequestSpawn(playerid) return P[playerid][pLogged] ? 1 : 0;

public OnPlayerDisconnect(playerid, reason)
{
    if (IsPlayerNPC(playerid)) return 1;
    if (P[playerid][pLogged])
    {
        if (P[playerid][pFishState]) Fish_Stop(playerid);
        if (P[playerid][pExamVeh] != INVALID_VEHICLE_ID) School_Fail(playerid, "выход из игры");
        Job_End(playerid, false);
        Phone_OnDisconnect(playerid);
        Rent_OnDisconnect(playerid);
        if (P[playerid][pCuffed] || P[playerid][pWanted] >= 3) // уход от ареста
        {
            new bool:copNear = false, Float:x, Float:y, Float:z;
            GetPlayerPos(playerid, x, y, z);
            for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (i != playerid && IsLogged(i) && IsCop(i) && IsPlayerInRangeOfPoint(i, 30.0, x, y, z)) copNear = true;
            if (copNear) { P[playerid][pJail] = P[playerid][pWanted] * 10 + 10; P[playerid][pWanted] = 0; SendClientMessageToAll(C_RED, "Игрок вышел от ареста и будет посажен при входе."); }
        }
        P_Save(playerid, P[playerid][pInjured] == 0 && P[playerid][pHouseIn] == 0 && P[playerid][pBizIn] == 0);
        Veh_UnloadForPlayer(playerid);
    }
    Hospital_Clear(playerid);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsPlayerConnected(i) && P[i][pSpecTarget] == playerid + 1) cmd_specoff(i, "\1");
    for (new k = 0; k < MAX_REPORTS; k++) if (gReport[k][rUsed] && gReport[k][rFrom] == playerid) gReport[k][rUsed] = false;
    Extras_Reset(playerid);
    P_Reset(playerid);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    if (IsPlayerNPC(playerid)) return 1;
    if (!P[playerid][pLogged]) { Kick(playerid); return 1; }
    SetPlayerTeam(playerid, NO_TEAM);
    if (P[playerid][pCreateStep]) return Char_OnSpawn(playerid);
    if (P[playerid][pInjured] == 2) { Hospital_OnSpawnWounded(playerid); return 1; }
    Spawn_Apply(playerid);
    Extras_OnSpawn(playerid);
    return 1;
}

public OnPlayerDeath(playerid, killerid, reason)
{
    if (P[playerid][pFishState]) Fish_Stop(playerid);
    if (P[playerid][pExamVeh] != INVALID_VEHICLE_ID) School_Fail(playerid, "вы погибли");
    if (P[playerid][pDetector]) { P[playerid][pDetector] = 0; RemovePlayerAttachedObject(playerid, 8); }
    Job_End(playerid, false);
    if (P[playerid][pCuffed]) { P[playerid][pCuffed] = false; SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE); }
    // часть наличных выпадает при смерти
    if (P[playerid][pCash] >= 200 && killerid != INVALID_PLAYER_ID)
    {
        new lost = P[playerid][pCash] / 10;
        Money_Take(playerid, lost, "death");
        Money_Give(killerid, lost, "loot");
    }
    Hospital_OnDeath(playerid, killerid);
    return 1;
}

public OnPlayerText(playerid, text[])
{
    if (!P[playerid][pLogged]) return 0;
    if (P[playerid][pMute] > 0) { new s[64]; format(s, sizeof s, "У вас молчанка ещё %d мин.", P[playerid][pMute]); Err(playerid, s); return 0; }
    if (GetTickCount() - P[playerid][pChatTick] < 800) { Err(playerid, "Не флудите."); return 0; }
    P[playerid][pChatTick] = GetTickCount();
    if (Phone_OnText(playerid, text)) return 0;
    new s[160];
    format(s, sizeof s, "%s[%d] говорит: %s", PName(playerid), playerid, text);
    ProxMsg(playerid, 20.0, C_WHITE, s);
    if (!IsPlayerInAnyVehicle(playerid)) ApplyAnimation(playerid, "PED", "IDLE_chat", 4.1, 0, 1, 1, 1, 1000, 1);
    return 0;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!P[playerid][pLogged] || P[playerid][pCreateStep]) { Err(playerid, "Сначала войдите в игру."); return 1; }
    new name[32], i = 1, j = 0;
    while (cmdtext[i] > ' ' && j < sizeof name - 1) { new c = cmdtext[i++]; if (c >= 'A' && c <= 'Z') c += 32; name[j++] = c; }
    name[j] = 0;
    while (cmdtext[i] == ' ') i++;
    new fn[40]; format(fn, sizeof fn, "cmd_%s", name);
    if (!name[0] || funcidx(fn) == -1)
    {
        Err(playerid, "Неизвестная команда. Список: /help");
        return 1;
    }
    new params[128];
    strmid(params, cmdtext, i, strlen(cmdtext), sizeof params);
    if (!params[0]) params = "\1";
    CallLocalFunction(fn, "is", playerid, params);
    return 1;
}

CMD:help(playerid, const params[])
{
    Menu_Begin("Помощь " GM_SHORT, "Основное управление и команды");
    Menu_Add(1, "ALT", "Взаимодействие с NPC, дверями, кассами", -1, "key");
    Menu_Add(2, "I — инвентарь, P — телефон", "Также /phone", -1, "key");
    Menu_Add(3, "/quest", "Квесты и подсказки", -1, "quest");
    Menu_Add(4, "/me /do /try /b /s /w", "Ролевой чат", -1, "chat");
    Menu_Add(5, "/pass /med /lic [id]", "Показать документы", -1, "doc");
    Menu_Add(6, "/lock /engine /cars /fill /park", "Транспорт", -1, "car");
    Menu_Add(7, "/house /fish /stopfish /sleep", "Дом, рыбалка, сон", -1, "home");
    Menu_Add(8, "/r /d /duty /members /lmenu", "Организации", -1, "org");
    Menu_Add(9, "/fc /fmenu /finvite", "Семьи", -1, "family");
    Menu_Add(10, "/taxi /call911 /report /admins /ach", "Прочее", -1, "info");
    Menu_Add(11, "/anims /sit /lay /dance /wave /stopanim", "Анимации (новое в 2.0)", -1, "chat");
    Menu_Add(12, "/belt /hood /trunk /lights /eject", "Транспорт (новое в 2.0)", -1, "car");
    Menu_Add(13, "/dice /coin /pay /id /online /rules", "Полезное (новое в 2.0)", -1, "info");
    Menu_Send(playerid, "noop");
    return 1;
}

CMD:me(playerid, const params[]) { if (isnull(params)) return Err(playerid, "/me [действие]"); SendMe(playerid, params); return 1; }
CMD:do(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/do [описание]");
    new s[160]; format(s, sizeof s, "%s (( %s ))", params, PName(playerid)); ProxMsg(playerid, 20.0, C_ME, s); return 1;
}
CMD:try(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/try [действие]");
    new s[180]; format(s, sizeof s, "%s %s %s", PName(playerid), params, random(2) ? ("{6EE07A}(удачно)") : ("{FF5A5A}(неудачно)"));
    ProxMsg(playerid, 20.0, C_ME, s); return 1;
}
CMD:b(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/b [OOC текст]");
    if (P[playerid][pMute] > 0) return Err(playerid, "У вас молчанка.");
    new s[160]; format(s, sizeof s, "(( %s[%d]: %s ))", PName(playerid), playerid, params); ProxMsg(playerid, 20.0, C_GREY, s); return 1;
}
CMD:s(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/s [крик]");
    if (P[playerid][pMute] > 0) return Err(playerid, "У вас молчанка.");
    new s[160]; format(s, sizeof s, "%s кричит: %s!", PName(playerid), params); ProxMsg(playerid, 40.0, C_WHITE, s); return 1;
}
CMD:w(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/w [шёпот]");
    new s[160]; format(s, sizeof s, "%s шепчет: %s", PName(playerid), params); ProxMsg(playerid, 3.0, C_GREY, s); return 1;
}
CMD:pm(playerid, const params[])
{
    new t = GetParamPlayer(params, 0), text[128]; GetRest(params, 1, text);
    if (t == INVALID_PLAYER_ID || isnull(text)) return Err(playerid, "/pm [id] [текст] (OOC)");
    if (P[playerid][pMute] > 0) return Err(playerid, "У вас молчанка.");
    new s[180];
    format(s, sizeof s, "(( PM от %s[%d]: %s ))", PName(playerid), playerid, text); SCM(t, C_YELLOW, s);
    format(s, sizeof s, "(( PM для %s[%d]: %s ))", PName(t), t, text); SCM(playerid, C_YELLOW, s);
    return 1;
}
CMD:inv(playerid, const params[]) return Inv_Open(playerid);
CMD:time(playerid, const params[])
{
    new h, m, s, st[96]; gettime(h, m, s);
    format(st, sizeof st, "Время: %02d:%02d. Отыграно: %d ч %d мин. До PayDay нужно %d мин. игры в час.", h, m, P[playerid][pPlayed] / 60, P[playerid][pPlayed] % 60, PAYDAY_MIN_PLAYED);
    Info(playerid, st); return 1;
}
CMD:stats(playerid, const params[])
{
    new s[220], ph[12];
    if (P[playerid][pPhone][0]) strcpy(ph, P[playerid][pPhone]); else ph = "нет";
    format(s, sizeof s, "%s | Ур. %d (%d/%d exp) | Наличные %s | Банк %s | Орг.: %s | Работа: %s | Тел.: %s | Отыграно %d ч",
        PName(playerid), P[playerid][pLevel], P[playerid][pExp], Lvl_Need(P[playerid][pLevel]), FormatMoney(P[playerid][pCash]), FormatMoney(P[playerid][pBank]),
        Org_Name(P[playerid][pOrg]), Job_Name(P[playerid][pJob]), ph, P[playerid][pPlayed] / 60);
    SCM(playerid, C_WHITE, s); return 1;
}
CMD:gps(playerid, const params[])
{
    Menu_Begin("GPS-навигатор", "Метка появится на карте");
    static const names[][] = {"Мэрия", "Больница", "Банк", "Магазин 24/7", "Магазин одежды", "Автосалон", "Автошкола", "Полиция LSPD", "Казино", "Рыболовный пирс", "Лавка старьёвщика", "Работа: грузчик", "Работа: дворник", "Работа: водитель автобуса", "Работа: таксист", "Работа: дальнобойщик", "Работа: шахтёр", "Приём стеклотары", "Дядя Вова", "Регистрация семей", "Аренда мопедов", "Шаурма у Ашота", "Ночлежка"};
    for (new i = 0; i < sizeof names; i++) Menu_Add(i, names[i], "", -1, "gps");
    Menu_Send(playerid, "gps");
    return 1;
}
MENU:gps(playerid, item, const extra[])
{
    static const codes[][] = {"cityhall_door", "hospital", "bank", "shop247_door", "binco_door", "dealer", "autoschool_door", "lspd_door", "casino_door", "fishing", "treasure_shop", "job_loader", "job_janitor", "job_bus", "job_taxi", "job_trucker", "job_miner", "recycle", "vova", "family_reg", "rent1", "kiosk", "shelter"};
    UI_Close(playerid);
    if (item < 0 || item >= sizeof codes) return 1;
    if (P[playerid][pCP] == CP_JOB) return Err(playerid, "Сначала закончите рабочее задание.");
    new Float:x, Float:y, Float:z, Float:a, it, vw;
    if (!Loc_Get(codes[item], x, y, z, a, it, vw)) return 1;
    SetPlayerCheckpoint(playerid, x, y, z, 4.0);
    P[playerid][pCP] = CP_GPS;
    Ok(playerid, "Метка установлена.");
    return 1;
}

// ---------- взаимодействие (ALT) ----------
stock Point_Use(playerid)
{
    new pt = Point_Nearest(playerid, 2.2);
    if (pt == -1) return 0;
    new id = gPt[pt][ptId];
    switch (gPt[pt][ptType])
    {
        case PT_DOOR_IN:     Door_Use(playerid, id, true);
        case PT_DOOR_OUT:    Door_Use(playerid, id, false);
        case PT_VOVA:        Vova_Talk(playerid, id);
        case PT_RECYCLE:     Recycle_Use(playerid);
        case PT_KIOSK:       Kiosk_Open(playerid);
        case PT_QUEUE:       Queue_Take(playerid);
        case PT_WINDOW:      Window_Use(playerid);
        case PT_HR:          HR_Open(playerid);
        case PT_HOSPITAL:    Hospital_Desk(playerid);
        case PT_SHOP247:     Shop247_Open(playerid);
        case PT_BINCO:       Binco_Open(playerid);
        case PT_DEALER:      Dealer_Open(playerid);
        case PT_IMPOUND:     Impound_Open(playerid);
        case PT_SCHOOL:      School_Desk(playerid);
        case PT_CASINO:      Casino_Open(playerid);
        case PT_FISH_MARKET: FishMarket_Open(playerid);
        case PT_FISHING:     cmd_fish(playerid, "\1");
        case PT_JOB:         Job_Desk(playerid, id);
        case PT_ORG:         Org_HQ(playerid, id);
        case PT_ATM:         Bank_Open(playerid, true);
        case PT_BANK:        Bank_Open(playerid, false);
        case PT_GAS:         cmd_fill(playerid, "\1");
        case PT_FAMREG:      FamReg_Use(playerid);
        case PT_JUNK:        Junk_Open(playerid);
        case PT_HOUSE:       House_Door(playerid, id);
        case PT_HOUSE_EXIT:  House_Exit(playerid, id);
        case PT_BIZ:         Biz_Door(playerid, id);
        case PT_LOADER_PICK: Loader_Pick(playerid);
        case PT_LOADER_DROP: Loader_Drop(playerid);
        case PT_MINE_DROP:   Miner_Drop(playerid);
        case PT_ITEM:        Drop_Pick(playerid, id);
        case PT_RENT:        Rent_Desk(playerid);
        default: return 0;
    }
    return 1;
}

public OnPlayerKeyStateChange(playerid, newkeys, oldkeys)
{
    if (!P[playerid][pLogged] || P[playerid][pCreateStep]) return 1;
    if ((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK) && !IsPlayerInAnyVehicle(playerid))
    {
        if (P[playerid][pInjured] == 2 || P[playerid][pCuffed]) return 1;
        Point_Use(playerid);
    }
    if ((newkeys & KEY_NO) && !(oldkeys & KEY_NO) && GetPlayerState(playerid) == PLAYER_STATE_DRIVER) cmd_engine(playerid, "\1");
    if ((newkeys & KEY_YES) && !(oldkeys & KEY_YES) && !IsPlayerInAnyVehicle(playerid)) Inv_Open(playerid);
    if (P[playerid][pFishState] && (newkeys & KEY_SECONDARY_ATTACK)) cmd_stopfish(playerid, "\1");
    return 1;
}

// ---------- чекпоинты, транспорт ----------
public OnPlayerEnterCheckpoint(playerid)
{
    if (P[playerid][pCP] == CP_GPS) { DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE; Ok(playerid, "Вы прибыли на место."); return 1; }
    Jobs_OnEnterCP(playerid);
    return 1;
}

public OnPlayerEnterRaceCheckpoint(playerid)
{
    Jobs_OnEnterRaceCP(playerid);
    return 1;
}

public OnPlayerEnterVehicle(playerid, vehicleid, ispassenger)
{
    Veh_OnEnter(playerid, vehicleid, ispassenger);
    return 1;
}

public OnPlayerExitVehicle(playerid, vehicleid)
{
    Jobs_OnExitVehicle(playerid, vehicleid);
    School_OnExitVehicle(playerid, vehicleid);
    Extras_OnExitVehicle(playerid);
    return 1;
}

public OnPlayerStateChange(playerid, newstate, oldstate)
{
    if (newstate == PLAYER_STATE_DRIVER)
    {
        new v = GetPlayerVehicleID(playerid);
        Veh_OnStateDriver(playerid, v);
        Rent_OnStateDriver(playerid, v);
        Jobs_OnEnterVehicle(playerid, v);
    }
    return 1;
}

public OnVehicleDeath(vehicleid, killerid)
{
    if (Rent_OnVehicleDeath(vehicleid)) return 1;
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i)) continue;
        if (P[i][pJobVeh] == vehicleid) { P[i][pJobVeh] = INVALID_VEHICLE_ID; DestroyVehicle(vehicleid); Job_End(i, true); return 1; }
        if (P[i][pExamVeh] == vehicleid) { School_Fail(i, "учебная машина уничтожена"); return 1; }
    }
    if (vehicleid > 0 && vehicleid < MAX_VEHICLES && gAdmVeh[vehicleid]) { DestroyVehicle(vehicleid); gAdmVeh[vehicleid] = 0; return 1; }
    if (vehicleid > 0 && vehicleid < MAX_VEHICLES && gV[vehicleid][vDB]) Veh_Impound(vehicleid, "транспорт уничтожен");
    return 1;
}

public OnVehicleSpawn(vehicleid)
{
    Rent_OnVehicleSpawn(vehicleid);
    if (vehicleid > 0 && vehicleid < MAX_VEHICLES && !gV[vehicleid][vDB]) Veh_SetFuel(vehicleid, 100);
    return 1;
}

public OnPlayerPickUpPickup(playerid, pickupid)
{
    Bottles_OnPickup(playerid, pickupid);
    return 1;
}

public OnPlayerClickMap(playerid, Float:fX, Float:fY, Float:fZ)
{
    if (P[playerid][pAdmin] >= 3 && !IsPlayerInAnyVehicle(playerid)) { SetPlayerPosFindZ(playerid, fX, fY, fZ + 1.0); SetPlayerInterior(playerid, 0); SetPlayerVirtualWorld(playerid, 0); }
    return 1;
}

public OnRconLoginAttempt(ip[], password[], success)
{
    if (!success) { new s[64]; format(s, sizeof s, "Неудачный RCON-вход с IP %s", ip); Log("rcon", ip, s); }
    return 1;
}

public OnPlayerTakeDamage(playerid, issuerid, Float:amount, weaponid, bodypart)
{
    if (issuerid != INVALID_PLAYER_ID) P[playerid][pLastDamageBy] = issuerid;
    Extras_OnTakeDamage(playerid, amount, weaponid);
    return 1;
}
