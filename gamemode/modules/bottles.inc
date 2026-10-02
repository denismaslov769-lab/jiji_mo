// =====================================================================
//  Сбор бутылок (подработка для новичков)
// =====================================================================
#define MAX_BOTTLES 15
new gBottlePickup[MAX_BOTTLES] = {-1, ...};
new Float:gBottlePos[MAX_BOTTLES][3];

stock Bottles_Init()
{
    new code[12], Float:a, it, vw;
    for (new i = 0; i < MAX_BOTTLES; i++)
    {
        format(code, sizeof code, "bottle%d", i + 1);
        Loc_Get(code, gBottlePos[i][0], gBottlePos[i][1], gBottlePos[i][2], a, it, vw);
        Bottle_Spawn(i);
    }
}

forward Bottle_Spawn(i);
public Bottle_Spawn(i)
{
    if (gBottlePickup[i] != -1) return;
    gBottlePickup[i] = CreatePickup(1486, 1, gBottlePos[i][0] + (random(20) - 10) / 10.0, gBottlePos[i][1] + (random(20) - 10) / 10.0, gBottlePos[i][2], 0);
}

stock Bottles_OnPickup(playerid, pickupid)
{
    for (new i = 0; i < MAX_BOTTLES; i++)
    {
        if (gBottlePickup[i] != pickupid) continue;
        if (IsPlayerInAnyVehicle(playerid)) return 1;
        if (Inv_Add(playerid, Item_ByKey("bottle"), 1) <= 0) return 1;
        DestroyPickup(gBottlePickup[i]);
        gBottlePickup[i] = -1;
        ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.0, 0, 0, 0, 0, 0);
        new s[64]; format(s, sizeof s, "Бутылок в сумке: %d", Inv_Count(playerid, Item_ByKey("bottle")));
        Info(playerid, s);
        SetTimerEx("Bottle_Spawn", 60000 + random(60000), false, "i", i);
        return 1;
    }
    return 0;
}

stock Recycle_Use(playerid)
{
    new it = Item_ByKey("bottle"), n = Inv_Count(playerid, it);
    if (n <= 0) return Err(playerid, "Приёмщик: «Пустой пришёл? Бутылки неси!»");
    Inv_Remove(playerid, it, n);
    new pay = n * gItems[it][iSell];
    Money_Give(playerid, pay, "recycle");
    new s[96]; format(s, sizeof s, "Вы сдали %d бутылок и получили %s.", n, FormatMoney(pay));
    Ok(playerid, s);
    SendMe(playerid, "сдал стеклотару в окошко приёмщика");
    Quest_Event(playerid, "recycle", "*", n);
    return 1;
}
