// =====================================================================
//  Дома и квартиры: покупка, аренда, шкаф, замок, налог на имущество
// =====================================================================
#define MAX_HOUSES 64
enum E_HOUSE { hID, hOwner, hPrice, hRent, Float:hX, Float:hY, Float:hZ, hClass, bool:hLocked, hDebt, bool:hForRent, hPoint, hExitPoint }
new gH[MAX_HOUSES][E_HOUSE], gHCount;
static const Float:gHouseInt[][4] = {
    {223.04, 1287.26, 1082.14, 1.0},
    {2468.84, -1698.22, 1013.51, 2.0},
    {2365.30, -1135.60, 1050.88, 8.0},
    {2233.69, -1115.26, 1050.88, 5.0}
};

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
        new c = gH[k][hClass]; if (c < 0 || c > 3) c = 0;
        gH[k][hExitPoint] = Point_Add(PT_HOUSE_EXIT, k, gHouseInt[c][0], gHouseInt[c][1], gHouseInt[c][2], VW_HOUSE_BASE + gH[k][hID], "[ALT] Выйти\n" H_GREY "/house — меню дома", 19132);
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
            new c = gH[k][hClass];
            SetPlayerInterior(playerid, floatround(gHouseInt[c][3]));
            SetPlayerVirtualWorld(playerid, VW_HOUSE_BASE + gH[k][hID]);
            SetPlayerPos(playerid, gHouseInt[c][0], gHouseInt[c][1], gHouseInt[c][2]);
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
