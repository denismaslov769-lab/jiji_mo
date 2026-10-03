// =====================================================================
//  Дома и квартиры: покупка, аренда, шкаф, замок, налог на имущество
// =====================================================================
forward bool:AdmCheck(playerid, level);
#define MAX_HOUSES 64
enum E_HOUSE { hID, hOwner, hPrice, hRent, Float:hX, Float:hY, Float:hZ, hClass, bool:hLocked, hDebt, bool:hForRent, hPoint, hExitPoint, hInt }
new gH[MAX_HOUSES][E_HOUSE], gHCount;
// Пулы родных интерьеров SA по классам: {x, y, z, угол, interior, класс}.
// Каждый дом получает свой интерьер (по порядку внутри класса), админ может
// переназначить: /hint [id дома] [номер] (столбец houses.interior, -1 = авто).
static const Float:gHouseInt[][6] = {
    // класс 0 — комнаты под аренду
    {2233.69, -1115.26, 1050.88, 0.0, 5.0, 0.0},
    {2218.40, -1076.18, 1050.48, 90.0, 1.0, 0.0},
    {2237.59, -1081.64, 1049.02, 0.0, 2.0, 0.0},
    {2308.77, -1212.94, 1049.02, 0.0, 6.0, 0.0},
    {2259.38, -1135.77, 1050.64, 270.0, 10.0, 0.0},
    {2282.91, -1140.29, 1050.90, 0.0, 11.0, 0.0},
    {2251.85, -1138.16, 1050.63, 270.0, 9.0, 0.0},
    {2196.85, -1204.25, 1049.02, 90.0, 6.0, 0.0},
    // класс 1 — небольшие дома
    {2468.84, -1698.22, 1013.51, 90.0, 2.0, 1.0},
    {2495.98, -1692.08, 1014.74, 180.0, 3.0, 1.0},
    {2365.30, -1135.60, 1050.88, 0.0, 8.0, 1.0},
    {2324.53, -1149.54, 1050.71, 0.0, 12.0, 1.0},
    {2807.48, -1174.76, 1025.57, 0.0, 8.0, 1.0},
    {-68.81, 1351.21, 1080.21, 0.0, 6.0, 1.0},
    // класс 2 — просторные дома
    {223.04, 1287.26, 1082.14, 0.0, 1.0, 2.0},
    {2317.89, -1026.76, 1050.22, 0.0, 9.0, 2.0},
    {235.34, 1186.68, 1080.26, 0.0, 3.0, 2.0},
    {226.30, 1114.24, 1080.99, 270.0, 5.0, 2.0},
    {140.17, 1366.07, 1083.86, 0.0, 5.0, 2.0},
    // класс 3 — особняки
    {1260.64, -785.37, 1091.91, 270.0, 5.0, 3.0},
    {225.68, 1021.45, 1084.02, 0.0, 7.0, 3.0},
    {234.19, 1063.73, 1084.21, 0.0, 6.0, 3.0}
};

// автоматический выбор: n-й дом своего класса -> n-й интерьер класса
stock House_AutoInt(k)
{
    new c = gH[k][hClass], n = 0, cnt = 0, first = -1;
    for (new j = 0; j < k; j++) if (gH[j][hClass] == c) n++;
    for (new i = 0; i < sizeof gHouseInt; i++) if (floatround(gHouseInt[i][5]) == c) { if (first == -1) first = i; cnt++; }
    if (cnt == 0) return 0;
    n %= cnt;
    for (new i = 0; i < sizeof gHouseInt; i++) if (floatround(gHouseInt[i][5]) == c) { if (n == 0) return i; n--; }
    return first;
}

stock House_PlaceExit(k)
{
    if (gH[k][hExitPoint] != -1) Point_Remove(gH[k][hExitPoint]);
    new t = gH[k][hInt];
    gH[k][hExitPoint] = Point_Add(PT_HOUSE_EXIT, k, gHouseInt[t][0], gHouseInt[t][1], gHouseInt[t][2], VW_HOUSE_BASE + gH[k][hID], "[ALT] Выйти\n" H_GREY "/house — меню дома", 19132);
}

// случайная дверь дома (для курьерских заказов)
stock House_RandomDoor(&Float:x, &Float:y, &Float:z)
{
    if (gHCount <= 0) return 0;
    new k = random(gHCount);
    x = gH[k][hX]; y = gH[k][hY]; z = gH[k][hZ];
    return 1;
}

stock Houses_Load()
{
    gHCount = 0;
    new DBResult:r = DB_Query("SELECT * FROM houses ORDER BY id");
    if (!r) return;
    for (new i = 0, n = db_num_rows(r); i < n && gHCount < MAX_HOUSES; i++)
    {
        new k = gHCount++;
        gH[k][hID] = DB_GetInt(r, "id"); gH[k][hOwner] = DB_GetInt(r, "owner");
        gH[k][hPrice] = DB_GetInt(r, "price"); gH[k][hRent] = DB_GetInt(r, "rent");
        gH[k][hX] = DB_GetFloat(r, "x"); gH[k][hY] = DB_GetFloat(r, "y"); gH[k][hZ] = DB_GetFloat(r, "z");
        gH[k][hClass] = DB_GetInt(r, "class"); gH[k][hLocked] = DB_GetInt(r, "locked") != 0;
        gH[k][hDebt] = DB_GetInt(r, "debt"); gH[k][hForRent] = DB_GetInt(r, "for_rent") != 0;
        gH[k][hPoint] = Point_Add(PT_HOUSE, k, gH[k][hX], gH[k][hY], gH[k][hZ], 0, "", 1273);
        if (gH[k][hClass] < 0 || gH[k][hClass] > 3) gH[k][hClass] = 0;
        new t = DB_GetInt(r, "interior");
        gH[k][hInt] = (t >= 0 && t < sizeof gHouseInt) ? t : House_AutoInt(k);
        gH[k][hExitPoint] = -1;
        House_PlaceExit(k);
        House_UpdateLabel(k);
        db_next_row(r);
    }
    db_free_result(r);
    printf("[HOUSE] Загружено домов: %d", gHCount);
}

stock House_Idx(id)
{
    for (new k = 0; k < gHCount; k++) if (gH[k][hID] == id) return k;
    return -1;
}

stock House_Renter(k)
{
    new q[96]; format(q, sizeof q, "SELECT id FROM accounts WHERE rent_house=%d LIMIT 1", gH[k][hID]);
    return DB_ScalarInt(q, 0);
}

stock House_UpdateLabel(k)
{
    new s[200];
    if (gH[k][hForRent])
    {
        if (House_Renter(k)) format(s, sizeof s, H_GOLD "Комната №%d\n" H_WHITE "Сдана\n" H_GREY "[ALT] Войти", gH[k][hID]);
        else format(s, sizeof s, H_GOLD "Комната №%d\n" H_GREEN "Сдаётся: %s / час\n" H_GREY "[ALT] Подробнее", gH[k][hID], FormatMoney(gH[k][hRent]));
    }
    else if (gH[k][hOwner]) format(s, sizeof s, H_GOLD "Дом №%d\n" H_WHITE "Владелец: %s\n" H_GREY "[ALT] Войти", gH[k][hID], AccName(gH[k][hOwner]));
    else format(s, sizeof s, H_GOLD "Дом №%d\n" H_GREEN "Продаётся: %s\n" H_GREY "[ALT] Подробнее", gH[k][hID], FormatMoney(gH[k][hPrice]));
    Point_SetLabel(gH[k][hPoint], s);
}

stock House_Save(k)
{
    new q[200];
    format(q, sizeof q, "UPDATE houses SET owner=%d,locked=%d,debt=%d WHERE id=%d", gH[k][hOwner], gH[k][hLocked] ? 1 : 0, gH[k][hDebt], gH[k][hID]);
    DB_Exec(q);
}

stock House_RentPrice(id) { new k = House_Idx(id); return k == -1 ? 0 : gH[k][hRent]; }
stock House_PayRent(id, amount)
{
    new k = House_Idx(id);
    if (k != -1 && gH[k][hOwner])
    {
        new o = FindPlayerByAcc(gH[k][hOwner]);
        if (o != INVALID_PLAYER_ID) P[o][pBank] += amount;
        else { new q[96]; format(q, sizeof q, "UPDATE accounts SET bank=bank+%d WHERE id=%d", amount, gH[k][hOwner]); DB_Exec(q); }
    }
    else gTreasury += amount;
}

stock bool:House_Access(playerid, k)
{
    if (gH[k][hOwner] && gH[k][hOwner] == P[playerid][pID]) return true;
    if (P[playerid][pRentHouse] == gH[k][hID]) return true;
    return !gH[k][hLocked];
}

stock House_Door(playerid, k)
{
    new t[48], d[96];
    format(t, sizeof t, gH[k][hForRent] ? ("Комната №%d") : ("Дом №%d"), gH[k][hID]);
    format(d, sizeof d, "Класс: %d | Налог: %s", gH[k][hClass], FormatMoney(gH[k][hDebt]));
    Menu_Begin(t, d);
    Menu_Add(1, "Войти", gH[k][hLocked] ? ("Закрыто") : ("Открыто"), -1, "door");
    if (!gH[k][hForRent] && !gH[k][hOwner]) Menu_Add(2, "Купить дом", "", gH[k][hPrice], "key");
    if (gH[k][hForRent] && !House_Renter(k)) Menu_Add(3, "Арендовать", "Оплата каждый PayDay со счёта", gH[k][hRent], "key");
    if (P[playerid][pRentHouse] == gH[k][hID]) Menu_Add(4, "Расторгнуть аренду", "", -1, "x");
    SetPVarInt(playerid, "houseK", k);
    Menu_Send(playerid, "house");
    return 1;
}

MENU:house(playerid, item, const extra[])
{
    new k = GetPVarInt(playerid, "houseK");
    if (k < 0 || k >= gHCount) return 1;
    UI_Close(playerid);
    switch (item)
    {
        case 1:
        {
            if (!House_Access(playerid, k)) return Err(playerid, "Дверь закрыта.");
            new t = gH[k][hInt];
            Int_GuardBegin(playerid, gHouseInt[t][2]);
            SetPlayerInterior(playerid, floatround(gHouseInt[t][4]));
            SetPlayerVirtualWorld(playerid, VW_HOUSE_BASE + gH[k][hID]);
            SetPlayerPos(playerid, gHouseInt[t][0], gHouseInt[t][1], gHouseInt[t][2]);
            SetPlayerFacingAngle(playerid, gHouseInt[t][3]); SetCameraBehindPlayer(playerid);
            P[playerid][pHouseIn] = gH[k][hID];
            TogglePlayerControllable(playerid, false); SetTimerEx("Unfreeze", 1200, false, "i", playerid);
        }
        case 2:
        {
            if (gH[k][hOwner] || gH[k][hForRent]) return 1;
            if (P[playerid][pLevel] < 3) return Err(playerid, "Покупка домов доступна с 3 уровня.");
            new q[96]; format(q, sizeof q, "SELECT COUNT(*) FROM houses WHERE owner=%d", P[playerid][pID]);
            if (DB_ScalarInt(q) > 0) return Err(playerid, "У вас уже есть дом.");
            if (!Pay(playerid, gH[k][hPrice], "house buy")) return 1;
            gH[k][hOwner] = P[playerid][pID]; gH[k][hDebt] = 0; gH[k][hLocked] = true;
            House_Save(k); House_UpdateLabel(k);
            Ok(playerid, "Поздравляем с покупкой дома! /house — управление.");
            Ach_Give(playerid, "home", "Крыша над головой");
            Quest_Event(playerid, "rent", "buy");
        }
        case 3:
        {
            if (!gH[k][hForRent] || House_Renter(k)) return 1;
            if (P[playerid][pRentHouse]) return Err(playerid, "Вы уже снимаете жильё.");
            if (!Pay(playerid, gH[k][hRent], "rent")) return 1;
            P[playerid][pRentHouse] = gH[k][hID];
            P_SetInt(playerid, "rent_house", gH[k][hID]);
            House_UpdateLabel(k);
            Ok(playerid, "Вы арендовали комнату. Оплата снимается со счёта каждый PayDay.");
            Ach_Give(playerid, "home", "Крыша над головой");
            Quest_Event(playerid, "rent", "rent");
        }
        case 4:
        {
            P[playerid][pRentHouse] = 0; P_SetInt(playerid, "rent_house", 0);
            House_UpdateLabel(k);
            Ok(playerid, "Аренда расторгнута.");
        }
    }
    return 1;
}

stock House_Exit(playerid, k)
{
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerPos(playerid, gH[k][hX], gH[k][hY], gH[k][hZ]);
    P[playerid][pHouseIn] = 0;
}

CMD:house(playerid, const params[])
{
    new k = House_Idx(P[playerid][pHouseIn]);
    if (k == -1) return Err(playerid, "Вы не в доме.");
    new bool:own = gH[k][hOwner] == P[playerid][pID], bool:renter = P[playerid][pRentHouse] == gH[k][hID];
    if (!own && !renter) return Err(playerid, "Это не ваш дом.");
    Menu_Begin("Мой дом", "Управление");
    Menu_Add(1, gH[k][hLocked] ? ("Открыть дверь") : ("Закрыть дверь"), "", -1, "lock");
    Menu_Add(2, "Шкаф", "Хранение вещей", -1, "box");
    Menu_Add(5, "Лечь спать", "Восстановить бодрость", -1, "bed");
    if (own)
    {
        Menu_Add(3, "Оплатить налог", "", gH[k][hDebt], "tax");
        Menu_Add(4, "Продать государству", "60% от цены", gH[k][hPrice] * 6 / 10, "x");
    }
    SetPVarInt(playerid, "houseK", k);
    Menu_Send(playerid, "myhouse");
    return 1;
}

MENU:myhouse(playerid, item, const extra[])
{
    new k = GetPVarInt(playerid, "houseK");
    if (k < 0 || k >= gHCount) return 1;
    UI_Close(playerid);
    switch (item)
    {
        case 1: { gH[k][hLocked] = !gH[k][hLocked]; House_Save(k); Ok(playerid, gH[k][hLocked] ? ("Дверь закрыта.") : ("Дверь открыта.")); }
        case 2: Storage_Open(playerid, ST_HOUSE, gH[k][hID], "Шкаф");
        case 3:
        {
            if (gH[k][hDebt] <= 0) return Info(playerid, "Долгов нет.");
            if (!Pay(playerid, gH[k][hDebt], "house tax")) return 1;
            gTreasury += gH[k][hDebt]; gH[k][hDebt] = 0; House_Save(k); Ok(playerid, "Налог оплачен.");
        }
        case 4:
        {
            if (gH[k][hOwner] != P[playerid][pID]) return 1;
            Money_Give(playerid, gH[k][hPrice] * 6 / 10, "house sell");
            gH[k][hOwner] = 0; gH[k][hDebt] = 0; House_Save(k); House_UpdateLabel(k);
            Ok(playerid, "Дом продан государству.");
        }
        case 5: cmd_sleep(playerid, "\1");
    }
    return 1;
}

stock Houses_TaxTick()
{
    for (new k = 0; k < gHCount; k++)
    {
        if (!gH[k][hOwner] || gH[k][hForRent]) continue;
        gH[k][hDebt] += (gH[k][hPrice] * TAX_PROPERTY / 100) / 24 + 1;
        if (gH[k][hDebt] > gH[k][hPrice] / 5)
        {
            new o = FindPlayerByAcc(gH[k][hOwner]);
            if (o != INVALID_PLAYER_ID) Err(o, "Ваш дом изъят государством за долги по налогу.");
            gH[k][hOwner] = 0; gH[k][hDebt] = 0;
            House_UpdateLabel(k);
        }
        House_Save(k);
    }
}

// /hint [id дома] [номер интерьера | -1 = авто] — сменить интерьер дома
CMD:hint(playerid, const params[])
{
    if (!AdmCheck(playerid, 4)) return 1;
    new id = GetParamInt(params, 0, 0), t = GetParamInt(params, 1, -2), k = House_Idx(id);
    if (k == -1 || t < -1 || t >= sizeof gHouseInt)
    {
        new s[128]; format(s, sizeof s, "/hint [id дома] [0-%d | -1 авто]. Классы: 0 — 0-7, 1 — 8-13, 2 — 14-18, 3 — 19-21.", sizeof gHouseInt - 1);
        return Err(playerid, s);
    }
    new q[96]; format(q, sizeof q, "UPDATE houses SET interior=%d WHERE id=%d", t, id); DB_Exec(q);
    gH[k][hInt] = t == -1 ? House_AutoInt(k) : t;
    House_PlaceExit(k);
    new s[96]; format(s, sizeof s, "Дом %d: интерьер #%d (int %d).", id, gH[k][hInt], floatround(gHouseInt[gH[k][hInt]][4]));
    return Ok(playerid, s);
}
