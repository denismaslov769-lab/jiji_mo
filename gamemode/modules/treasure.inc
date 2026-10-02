// =====================================================================
//  Кладоискатели: металлоискатель, лопата, лавка старьёвщика
// =====================================================================
// зоны поиска (minX, minY, maxX, maxY): пляжи и пустыри
new const Float:gTreasureZones[][4] =
{
    {150.0, -1900.0, 900.0, -1780.0},     // пляж Санта-Мария
    {-2950.0, -800.0, -2800.0, 400.0},    // западное побережье SF
    {2000.0, -1700.0, 2300.0, -1500.0},   // пустыри Ганг-района (для теста)
    {-400.0, 1450.0, 200.0, 1800.0}       // пустыня у Big Ear
};
new Float:gTrX[MAX_PLAYERS], Float:gTrY[MAX_PLAYERS];

stock Treasure_Zone(playerid)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    for (new i = 0; i < sizeof gTreasureZones; i++)
        if (x >= gTreasureZones[i][0] && x <= gTreasureZones[i][2] && y >= gTreasureZones[i][1] && y <= gTreasureZones[i][3]) return i;
    return -1;
}

stock Treasure_NewTarget(playerid)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new z0 = Treasure_Zone(playerid);
    if (z0 == -1) { P[playerid][pTreasureTarget] = 0; return; }
    new Float:ang = float(random(360)), Float:dist = 15.0 + float(random(30));
    gTrX[playerid] = x + dist * floatcos(ang, degrees);
    gTrY[playerid] = y + dist * floatsin(ang, degrees);
    if (gTrX[playerid] < gTreasureZones[z0][0]) gTrX[playerid] = gTreasureZones[z0][0] + 5.0;
    if (gTrX[playerid] > gTreasureZones[z0][2]) gTrX[playerid] = gTreasureZones[z0][2] - 5.0;
    if (gTrY[playerid] < gTreasureZones[z0][1]) gTrY[playerid] = gTreasureZones[z0][1] + 5.0;
    if (gTrY[playerid] > gTreasureZones[z0][3]) gTrY[playerid] = gTreasureZones[z0][3] - 5.0;
    P[playerid][pTreasureTarget] = 1;
}

stock Treasure_Toggle(playerid)
{
    if (P[playerid][pDetector] > 0 || P[playerid][pTreasureTarget] == -1)
    {
        P[playerid][pDetector] = 0; P[playerid][pTreasureTarget] = 0;
        RemovePlayerAttachedObject(playerid, 8);
        Info(playerid, "Металлоискатель выключен.");
        UI_Hud(playerid);
        return 1;
    }
    if (Treasure_Zone(playerid) == -1) return Err(playerid, "Здесь нечего искать. Попробуйте пляж Санта-Мария.");
    Treasure_NewTarget(playerid);
    P[playerid][pDetector] = 1;
    SetPlayerAttachedObject(playerid, 8, 2045, 6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    SendMe(playerid, "включил металлоискатель");
    Info(playerid, "Металлоискатель включён. Следите за сигналом на HUD, у максимума — копайте лопатой.");
    return 1;
}

// вызывается каждую секунду
stock Treasure_Tick(playerid)
{
    if (P[playerid][pDetector] <= 0) return;
    if (IsPlayerInAnyVehicle(playerid) || Treasure_Zone(playerid) == -1)
    {
        P[playerid][pDetector] = 1;
        return;
    }
    if (!P[playerid][pTreasureTarget]) Treasure_NewTarget(playerid);
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new Float:d = floatsqroot((x - gTrX[playerid]) * (x - gTrX[playerid]) + (y - gTrY[playerid]) * (y - gTrY[playerid]));
    new sig = 100 - floatround(d * 2.0);
    if (sig < 1) sig = 1;
    P[playerid][pDetector] = sig;
    if (sig >= 95) PlayerPlaySound(playerid, 1056, 0.0, 0.0, 0.0);
    else if (sig >= 70) PlayerPlaySound(playerid, 1052, 0.0, 0.0, 0.0);
}

stock Treasure_Dig(playerid)
{
    if (IsPlayerInAnyVehicle(playerid)) return Err(playerid, "Выйдите из транспорта.");
    if (P[playerid][pDetector] <= 0) return Err(playerid, "Сначала включите металлоискатель.");
    if (P[playerid][pDetector] < 95) return Err(playerid, "Сигнал слабый — здесь ничего нет.");
    if (gettime() - P[playerid][pAntiSpam] < 5) return 1;
    P[playerid][pAntiSpam] = gettime();
    ApplyAnimation(playerid, "BOMBER", "BOM_Plant_Loop", 4.0, 1, 0, 0, 0, 0);
    SetPlayerAttachedObject(playerid, 7, 337, 6);
    TogglePlayerControllable(playerid, false);
    SendMe(playerid, "копает лопатой");
    if (P[playerid][pCef]) { UI_Data(playerid, "progress", "{\"text\":\"Копаем...\",\"ms\":5000}"); }
    SetTimerEx("Treasure_Found", 5000, false, "i", playerid);
    return 1;
}

forward Treasure_Found(playerid);
public Treasure_Found(playerid)
{
    if (!IsPlayerConnected(playerid)) return;
    ClearAnimations(playerid);
    RemovePlayerAttachedObject(playerid, 7);
    TogglePlayerControllable(playerid, true);
    new x = random(100), item, amount = 1;
    if (x < 45) { item = Item_ByKey("junk"); amount = 1 + random(2); }
    else if (x < 85) { item = Item_ByKey("coin"); amount = 1 + random(3); }
    else if (x < 97) item = Item_ByKey("ring");
    else { item = Item_ByKey("coin"); amount = 15; }
    if (Inv_Add(playerid, item, amount) > 0)
    {
        new s[96]; format(s, sizeof s, "Находка: %s x%d", gItems[item][iName], amount); Ok(playerid, s);
        if (amount >= 15 || item == Item_ByKey("ring")) Ach_Give(playerid, "treasure", "Охотник за сокровищами");
        Quest_Event(playerid, "dig", gItems[item][iKey]);
    }
    Treasure_NewTarget(playerid);
}

stock Junk_Open(playerid)
{
    Menu_Begin("Лавка старьёвщика", "Снаряжение кладоискателя и скупка");
    Menu_Add(1, "Металлоискатель", "Показывает сигнал на HUD", 1500, "detector");
    Menu_Add(2, "Лопата", "Чтобы выкопать находку", 300, "shovel");
    new d[48];
    format(d, sizeof d, "Монеты, кольца, хлам (у вас %d шт.)", Inv_Count(playerid, Item_ByKey("coin")) + Inv_Count(playerid, Item_ByKey("ring")) + Inv_Count(playerid, Item_ByKey("junk")));
    Menu_Add(3, "Продать все находки", d, -1, "cash");
    Menu_Send(playerid, "junk");
    return 1;
}

MENU:junk(playerid, item, const extra[])
{
    switch (item)
    {
        case 1, 2:
        {
            new it = Item_ByKey(item == 1 ? ("detector") : ("shovel")), price = item == 1 ? 1500 : 300;
            if (Inv_Count(playerid, it) > 0) return Err(playerid, "У вас уже есть этот инструмент.");
            if (!Pay(playerid, price, "junk_shop")) return Err(playerid, "Недостаточно денег.");
            if (Inv_Add(playerid, it, 1) <= 0) { Money_Give(playerid, price, "refund"); return 1; }
            Ok(playerid, "Покупка совершена.");
        }
        case 3:
        {
            new keys[][] = {"coin", "ring", "junk"};
            new total = Sell_All(playerid, keys, sizeof keys);
            if (total <= 0) return Err(playerid, "Нечего продавать.");
            Money_Give(playerid, total, "treasure_sell");
            new s[64]; format(s, sizeof s, "Старьёвщик заплатил %s", FormatMoney(total)); Ok(playerid, s);
        }
    }
    return 1;
}
