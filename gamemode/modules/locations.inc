// =====================================================================
//  Точки на карте (из таблицы locations), интерактивные точки (ALT), двери
// =====================================================================
#define MAX_LOCS 128
enum E_LOC { lCode[24], Float:lX, Float:lY, Float:lZ, Float:lA, lInt, lVW, lTitle[64] }
new gLoc[MAX_LOCS][E_LOC], gLocCount;

stock Loc_Load()
{
    gLocCount = 0;
    new DBResult:r = DB_Query("SELECT * FROM locations");
    if (!r) return;
    new n = db_num_rows(r);
    for (new i = 0; i < n && gLocCount < MAX_LOCS; i++)
    {
        DB_GetStr(r, "code", gLoc[gLocCount][lCode], 24);
        gLoc[gLocCount][lX] = DB_GetFloat(r, "x");
        gLoc[gLocCount][lY] = DB_GetFloat(r, "y");
        gLoc[gLocCount][lZ] = DB_GetFloat(r, "z");
        gLoc[gLocCount][lA] = DB_GetFloat(r, "a");
        gLoc[gLocCount][lInt] = DB_GetInt(r, "interior");
        gLoc[gLocCount][lVW] = DB_GetInt(r, "vw");
        DB_GetStr(r, "title", gLoc[gLocCount][lTitle], 64);
        gLocCount++;
        db_next_row(r);
    }
    db_free_result(r);
    printf("[LOC] Загружено точек: %d", gLocCount);
}

stock Loc_Find(const code[])
{
    for (new i = 0; i < gLocCount; i++) if (StrEq(gLoc[i][lCode], code)) return i;
    return -1;
}

stock bool:Loc_Get(const code[], &Float:x, &Float:y, &Float:z, &Float:a = 0.0, &interior = 0, &vw = 0)
{
    new i = Loc_Find(code);
    if (i == -1) { x = 1765.9; y = -1938.0; z = 13.57; return false; }
    x = gLoc[i][lX]; y = gLoc[i][lY]; z = gLoc[i][lZ]; a = gLoc[i][lA];
    interior = gLoc[i][lInt]; vw = gLoc[i][lVW];
    return true;
}

stock Loc_Teleport(playerid, const code[])
{
    new Float:x, Float:y, Float:z, Float:a, it, vw;
    Loc_Get(code, x, y, z, a, it, vw);
    SetPlayerInterior(playerid, it);
    SetPlayerVirtualWorld(playerid, vw);
    SetPlayerPos(playerid, x, y, z);
    SetPlayerFacingAngle(playerid, a);
    SetCameraBehindPlayer(playerid);
    if (it) { TogglePlayerControllable(playerid, false); SetTimerEx("Unfreeze", 1200, false, "i", playerid); }
}

forward Unfreeze(playerid);
public Unfreeze(playerid) TogglePlayerControllable(playerid, true);

stock bool:IsPlayerAtLoc(playerid, const code[], Float:range = 3.0)
{
    new i = Loc_Find(code);
    if (i == -1) return false;
    if (GetPlayerVirtualWorld(playerid) != gLoc[i][lVW]) return false;
    return IsPlayerInRangeOfPoint(playerid, range, gLoc[i][lX], gLoc[i][lY], gLoc[i][lZ]) != 0;
}

// ---------------- интерактивные точки ----------------
enum
{
    PT_NONE, PT_DOOR_IN, PT_DOOR_OUT, PT_RECYCLE, PT_KIOSK, PT_QUEUE, PT_WINDOW, PT_HR, PT_HOSPITAL,
    PT_SHOP247, PT_BINCO, PT_DEALER, PT_IMPOUND, PT_SCHOOL, PT_CASINO, PT_FISH_MARKET, PT_FISHING,
    PT_JOB, PT_ORG, PT_ATM, PT_BANK, PT_GAS, PT_FAMREG, PT_JUNK, PT_HOUSE, PT_HOUSE_EXIT, PT_BIZ,
    PT_VOVA, PT_LOADER_PICK, PT_LOADER_DROP, PT_MINE_DROP, PT_GARAGE, PT_ITEM, PT_RENT
}
#define MAX_POINTS 320
enum E_POINT { bool:ptUsed, ptType, ptId, Float:ptX, Float:ptY, Float:ptZ, ptVW, Text3D:ptLabel, ptPickup }
new gPt[MAX_POINTS][E_POINT];

stock Point_Add(type, id, Float:x, Float:y, Float:z, vw, const label[], pickupModel = -1, color = 0xFFFFFFFF)
{
    for (new i = 0; i < MAX_POINTS; i++)
    {
        if (gPt[i][ptUsed]) continue;
        gPt[i][ptUsed] = true;
        gPt[i][ptType] = type; gPt[i][ptId] = id;
        gPt[i][ptX] = x; gPt[i][ptY] = y; gPt[i][ptZ] = z; gPt[i][ptVW] = vw;
        gPt[i][ptLabel] = Text3D:INVALID_3DTEXT_ID;
        gPt[i][ptPickup] = -1;
        if (label[0]) gPt[i][ptLabel] = Create3DTextLabel(label, color, x, y, z + 0.6, 12.0, vw, 1);
        if (pickupModel != -1) gPt[i][ptPickup] = CreatePickup(pickupModel, 1, x, y, z, vw);
        return i;
    }
    print("[POINT] Превышен лимит интерактивных точек");
    return -1;
}

stock Point_AddLoc(type, id, const code[], const label[], pickupModel = -1, color = 0xFFFFFFFF)
{
    new Float:x, Float:y, Float:z, Float:a, it, vw;
    if (!Loc_Get(code, x, y, z, a, it, vw)) { printf("[POINT] Нет точки %s", code); return -1; }
    return Point_Add(type, id, x, y, z, vw, label, pickupModel, color);
}

stock Point_Remove(idx)
{
    if (idx < 0 || idx >= MAX_POINTS || !gPt[idx][ptUsed]) return;
    if (gPt[idx][ptLabel] != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(gPt[idx][ptLabel]);
    if (gPt[idx][ptPickup] != -1) DestroyPickup(gPt[idx][ptPickup]);
    gPt[idx][ptUsed] = false;
}

stock Point_SetLabel(idx, const text[], color = 0xFFFFFFFF)
{
    if (idx < 0 || !gPt[idx][ptUsed]) return;
    if (gPt[idx][ptLabel] != Text3D:INVALID_3DTEXT_ID) Update3DTextLabelText(gPt[idx][ptLabel], color, text);
}

stock Point_Nearest(playerid, Float:range = 2.0)
{
    new best = -1, Float:bd = range, vw = GetPlayerVirtualWorld(playerid);
    for (new i = 0; i < MAX_POINTS; i++)
    {
        if (!gPt[i][ptUsed] || gPt[i][ptVW] != vw) continue;
        new Float:d = GetPlayerDistanceFromPoint(playerid, gPt[i][ptX], gPt[i][ptY], gPt[i][ptZ]);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

// ---------------- двери ----------------
#define MAX_DOORS 12
new gDoorOut[MAX_DOORS][24], gDoorIn[MAX_DOORS][24], gDoorCount;

stock Door_Add(const outCode[], const inCode[], const name[])
{
    if (gDoorCount >= MAX_DOORS) return;
    strcpy(gDoorOut[gDoorCount], outCode, 24);
    strcpy(gDoorIn[gDoorCount], inCode, 24);
    new lbl[96];
    format(lbl, sizeof lbl, "%s\n" H_GREY "[ALT] Войти", name);
    Point_AddLoc(PT_DOOR_IN, gDoorCount, outCode, lbl, 19132, 0xFFC94DFF);
    Point_AddLoc(PT_DOOR_OUT, gDoorCount, inCode, "[ALT] Выйти", 19132);
    gDoorCount++;
}

stock Door_Use(playerid, door, bool:enter)
{
    if (P[playerid][pCuffed]) return Err(playerid, "Вы в наручниках.");
    if (enter)
    {
        Loc_Teleport(playerid, gDoorIn[door]);
        Quest_Event(playerid, "enter", gDoorIn[door]);
    }
    else Loc_Teleport(playerid, gDoorOut[door]);
    return 1;
}

stock World_Build()
{
    Door_Add("cityhall_door", "cityhall_in", H_GOLD "Мэрия Лос-Сантоса");
    Door_Add("shop247_door", "shop247_in", H_GOLD "Магазин 24/7");
    Door_Add("binco_door", "binco_in", H_GOLD "Одежда «Ганни»");
    Door_Add("autoschool_door", "autoschool_in", H_GOLD "Автошкола");
    Door_Add("lspd_door", "lspd_in", H_GOLD "Полицейский департамент");
    Door_Add("casino_door", "casino_in", H_GOLD "Казино «Золотой дракон»");

    Point_AddLoc(PT_RECYCLE, 0, "recycle", H_GOLD "Пункт приёма стеклотары\n" H_GREY "[ALT] Сдать бутылки ($3/шт.)", 1239);
    Point_AddLoc(PT_KIOSK, 0, "kiosk", H_GOLD "Ларёк «Шаурма у Ашота»\n" H_GREY "[ALT] Купить еду", 1239);
    Point_AddLoc(PT_QUEUE, 0, "cityhall_queue", H_GOLD "Электронная очередь\n" H_GREY "[ALT] Взять талон", 1239);
    Point_AddLoc(PT_WINDOW, 0, "cityhall_window", H_GOLD "Паспортный стол\n" H_GREY "[ALT] Подойти к окну", 1239);
    Point_AddLoc(PT_HR, 0, "cityhall_hr", H_GOLD "Отдел кадров госслужбы\n" H_GREY "[ALT] Подать заявление", 1239);
    Point_AddLoc(PT_HOSPITAL, 0, "hospital", H_GOLD "Регистратура больницы\n" H_GREY "[ALT] Медкомиссия / лечение", 1240);
    Point_AddLoc(PT_SHOP247, 0, "shop247_cash", H_GOLD "Касса\n" H_GREY "[ALT] Купить товары", 1239);
    Point_AddLoc(PT_BINCO, 0, "binco_cash", H_GOLD "Примерочная\n" H_GREY "[ALT] Одежда и аксессуары", 1275);
    Point_AddLoc(PT_DEALER, 0, "dealer", H_GOLD "Автосалон\n" H_GREY "[ALT] Каталог", 1239);
    Point_AddLoc(PT_IMPOUND, 0, "impound", H_GOLD "Штрафстоянка\n" H_GREY "[ALT] Забрать транспорт", 1239);
    Point_AddLoc(PT_SCHOOL, 0, "autoschool_desk", H_GOLD "Инструктор\n" H_GREY "[ALT] Экзамены", 1239);
    Point_AddLoc(PT_CASINO, 0, "casino_table", H_GOLD "Рулетка\n" H_GREY "[ALT] Сделать ставку", 1239);
    Point_AddLoc(PT_FISH_MARKET, 0, "fish_market", H_GOLD "Рыбный рынок\n" H_GREY "[ALT] Продать улов", 1239);
    Point_AddLoc(PT_FISHING, 0, "fishing", H_GOLD "Рыболовный пирс\n" H_GREY "Встаньте у края и /fish", -1);
    Point_AddLoc(PT_FAMREG, 0, "family_reg", H_GOLD "Регистрация семей\n" H_GREY "[ALT] Создать семью ($50.000)", 1239);
    Point_AddLoc(PT_JUNK, 0, "treasure_shop", H_GOLD "Лавка старьёвщика\n" H_GREY "[ALT] Металлоискатель / скупка", 1239);
    Point_AddLoc(PT_BANK, 0, "bank", H_GOLD "Банк Лос-Сантоса\n" H_GREY "[ALT] Обслуживание", 1274);
    Point_AddLoc(PT_GARAGE, 0, "vova_garage", H_GOLD "Старый гараж", -1);
    Point_AddLoc(PT_LOADER_PICK, 0, "loader_pick", H_GOLD "Склад\n" H_GREY "[ALT] Взять ящик", -1);
    Point_AddLoc(PT_LOADER_DROP, 0, "loader_drop", H_GOLD "Погрузка\n" H_GREY "[ALT] Положить ящик", -1);
    Point_AddLoc(PT_MINE_DROP, 0, "mine_drop", H_GOLD "Приёмка руды\n" H_GREY "[ALT] Сдать руду", -1);

    new code[16];
    for (new i = 1; i <= 5; i++)
    {
        format(code, sizeof code, "atm%d", i);
        new pi = Point_AddLoc(PT_ATM, i, code, H_GOLD "Банкомат\n" H_GREY "[ALT] Использовать", -1);
        if (pi == -1) continue;
        // банкомат стоит «экраном» в сторону угла точки; игрок — перед экраном
        new Float:ax, Float:ay, Float:az, Float:aa;
        Loc_Get(code, ax, ay, az, aa);
        if (aa == 0.0) CreateObject(2942, ax, ay, az - 0.36, 0.0, 0.0, 0.0);
        else CreateObject(2942, ax + 0.95 * floatsin(aa, degrees), ay - 0.95 * floatcos(aa, degrees), az - 0.36, 0.0, 0.0, aa);
    }
    Point_AddLoc(PT_GAS, 1, "gas1", H_GOLD "Колонка АЗС\n" H_GREY "/fill — заправиться", -1);
    Point_AddLoc(PT_GAS, 2, "gas2", H_GOLD "Колонка АЗС\n" H_GREY "/fill — заправиться", -1);
    // штабы организаций и вербовщики
    new lbl[96];
    for (new o = 1; o <= 8; o++)
    {
        format(code, sizeof code, "org_%d", o);
        format(lbl, sizeof lbl, H_GOLD "%s\n" H_GREY "[ALT] Склад / вступление", Org_Name(o));
        Point_AddLoc(PT_ORG, o, code, lbl, 1314);
    }
}

stock World_MapIcons(playerid)
{
    new Float:x, Float:y, Float:z, idx = 0;
    static const codes[][] = {"cityhall_door","hospital","shop247_door","binco_door","dealer","lspd_door","casino_door","fishing","autoschool_door","job_loader","job_janitor","job_bus","job_taxi","job_trucker","job_miner","bank","gas1","gas2","recycle","impound","fish_market","treasure_shop","job_courier","job_garbage"};
    static const icons[] = {56, 22, 17, 45, 55, 30, 25, 9, 36, 51, 51, 51, 51, 51, 51, 52, 27, 27, 35, 53, 9, 18, 29, 51};
    for (new i = 0; i < sizeof icons; i++)
    {
        if (!Loc_Get(codes[i], x, y, z)) continue;
        SetPlayerMapIcon(playerid, idx++, x, y, z, icons[i], 0, MAPICON_LOCAL);
    }
}
