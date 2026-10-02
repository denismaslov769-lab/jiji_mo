// =====================================================================
//  Рыбалка: /fish на пирсе, мини-игра в CEF, рыбный рынок
// =====================================================================
enum { FISH_NONE, FISH_WAIT, FISH_BITE }
new gFishBiteAt[MAX_PLAYERS];

stock Fish_Level(playerid) return P[playerid][pFishSkill] / 25 + 1;

CMD:fish(playerid, const params[])
{
    if (P[playerid][pFishState] != FISH_NONE) return Err(playerid, "Вы уже рыбачите. /stopfish — смотать удочку.");
    if (IsPlayerInAnyVehicle(playerid)) return Err(playerid, "Выйдите из транспорта.");
    if (!IsPlayerAtLoc(playerid, "fishing", 35.0)) return Err(playerid, "Рыбачить можно только на рыболовном пирсе (метка на карте).");
    if (Inv_Count(playerid, Item_ByKey("rod")) <= 0) return Err(playerid, "Нужна удочка (магазин 24/7).");
    if (Inv_Count(playerid, Item_ByKey("bait")) <= 0) return Err(playerid, "Нужна наживка (магазин 24/7).");
    Inv_Remove(playerid, Item_ByKey("bait"), 1);
    P[playerid][pFishState] = FISH_WAIT;
    TogglePlayerControllable(playerid, false);
    SetPlayerAttachedObject(playerid, 9, 18632, 6, 0.08, 0.02, 0.0, 180.0, 0.0, 0.0);
    ApplyAnimation(playerid, "SWORD", "sword_IDLE", 4.0, 1, 0, 0, 1, 0);
    SendMe(playerid, "закинул удочку");
    Info(playerid, "Ждите поклёвку...");
    P[playerid][pFishTimer] = SetTimerEx("Fish_Bite", (8 + random(14)) * 1000, false, "i", playerid);
    return 1;
}

CMD:stopfish(playerid, const params[])
{
    if (P[playerid][pFishState] == FISH_NONE) return 1;
    Fish_Stop(playerid);
    Info(playerid, "Вы смотали удочку.");
    return 1;
}

stock Fish_Stop(playerid)
{
    if (P[playerid][pFishTimer] > 0) KillTimer(P[playerid][pFishTimer]);
    P[playerid][pFishTimer] = 0;
    P[playerid][pFishState] = FISH_NONE;
    if (IsPlayerConnected(playerid))
    {
        RemovePlayerAttachedObject(playerid, 9);
        ClearAnimations(playerid);
        TogglePlayerControllable(playerid, true);
    }
}

forward Fish_Bite(playerid);
public Fish_Bite(playerid)
{
    P[playerid][pFishTimer] = 0;
    if (P[playerid][pFishState] != FISH_WAIT) return;
    P[playerid][pFishState] = FISH_BITE;
    gFishBiteAt[playerid] = gettime();
    // сложность: чем выше навык, тем шире зона
    new lvl = Fish_Level(playerid), zone = 14 + lvl * 3, speed = 9 - lvl;
    if (zone > 34) zone = 34;
    if (speed < 3) speed = 3;
    new j[96];
    format(j, sizeof j, "{\"zone\":%d,\"speed\":%d,\"time\":10,\"lvl\":%d}", zone, speed, lvl);
    if (!UI_Open(playerid, "fishing", j))
    {
        // без CEF — упрощённо: шанс 50%%
        Fish_Result(playerid, random(2));
    }
    else GameTextForPlayer(playerid, "~y~KLYOT!", 1500, 4);
}

stock Fish_Result(playerid, success)
{
    if (P[playerid][pFishState] != FISH_BITE) return;
    if (gettime() - gFishBiteAt[playerid] > 14) success = 0;
    Fish_Stop(playerid);
    if (!success) { Err(playerid, "Рыба сорвалась с крючка!"); SendMe(playerid, "вытащил пустой крючок"); return; }
    new lvl = Fish_Level(playerid), x = random(100), item;
    if (x < 8 - lvl) item = Item_ByKey("junk");
    else if (x < 60 - lvl * 4) item = Item_ByKey("fish_s");
    else if (x < 90 - lvl * 2) item = Item_ByKey("fish_m");
    else item = Item_ByKey("fish_b");
    if (Inv_Add(playerid, item, 1) <= 0) return;
    P[playerid][pFishSkill]++;
    new s[96]; format(s, sizeof s, "вытащил %s", gItems[item][iName]); SendMe(playerid, s);
    if (item == Item_ByKey("fish_b")) Ach_Give(playerid, "fisher", "Король пирса");
    Quest_Event(playerid, "fish", gItems[item][iKey]);
}

UI:fish(playerid, const a[])
{
    new act[12]; CefField(a, 1, act);
    if (StrEq(act, "result"))
    {
        UI_Close(playerid, "fishing");
        Fish_Result(playerid, CefFieldInt(a, 2) == 1);
    }
    else if (StrEq(act, "cancel")) { UI_Close(playerid, "fishing"); Fish_Result(playerid, 0); }
    return 1;
}

stock FishMarket_Open(playerid)
{
    Menu_Begin("Рыбный рынок", "Скупка улова и находок");
    new keys[][] = {"fish_s", "fish_m", "fish_b", "junk"};
    for (new i = 0; i < sizeof keys; i++)
    {
        new it = Item_ByKey(keys[i]), d[48];
        format(d, sizeof d, "У вас: %d шт.", Inv_Count(playerid, it));
        Menu_Add(it, gItems[it][iName], d, gItems[it][iSell], "fish");
    }
    Menu_Add(0, "Продать весь улов", "Карась, окунь, щука", -1, "cash");
    Menu_Send(playerid, "fishmarket");
    return 1;
}

stock Sell_All(playerid, const keys[][], count)
{
    new total = 0;
    for (new i = 0; i < count; i++)
    {
        new it = Item_ByKey(keys[i]), n = Inv_Count(playerid, it);
        if (n <= 0) continue;
        Inv_Remove(playerid, it, n);
        total += n * gItems[it][iSell];
    }
    return total;
}

MENU:fishmarket(playerid, item, const extra[])
{
    new total = 0;
    if (item == 0)
    {
        new keys[][] = {"fish_s", "fish_m", "fish_b"};
        total = Sell_All(playerid, keys, sizeof keys);
    }
    else if (item > 0 && item < sizeof gItems)
    {
        new n = Inv_Count(playerid, item);
        if (n > 0) { Inv_Remove(playerid, item, n); total = n * gItems[item][iSell]; }
    }
    if (total <= 0) return Err(playerid, "Нечего продавать.");
    Money_Give(playerid, total, "fish_sell");
    new s[64]; format(s, sizeof s, "Продано на %s", FormatMoney(total)); Ok(playerid, s);
    FishMarket_Open(playerid);
    return 1;
}
