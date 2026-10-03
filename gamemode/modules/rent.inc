// =====================================================================
//  Аренда мопедов (Faggio): у вокзала Юнити и у ночлежки под эстакадой
//  Садитесь на свободный мопед -> подтверждение -> $50 за 30 минут.
//  /unrent — вернуть мопед досрочно.
// =====================================================================
#define RENT_PRICE   50
#define RENT_MINUTES 30
#define MAX_RENT_VEH 8

static const Float:gRentPos[MAX_RENT_VEH][4] = {
    {1772.3, -1910.6, 13.15, 270.0}, {1772.3, -1912.4, 13.15, 270.0}, {1772.3, -1914.2, 13.15, 270.0},
    {1772.3, -1916.0, 13.15, 270.0}, {1772.3, -1917.8, 13.15, 270.0},
    {1661.2, -1885.6, 13.15, 0.0}, {1662.8, -1885.6, 13.15, 0.0}, {1664.4, -1885.6, 13.15, 0.0}
};
static const gRentColor[MAX_RENT_VEH] = {3, 1, 6, 3, 1, 6, 3, 1};
new gRentVeh[MAX_RENT_VEH], gRentBy[MAX_RENT_VEH], gRentLeft[MAX_RENT_VEH];
new gRentOffer[MAX_PLAYERS] = {-1, ...};

stock Rent_Init()
{
    for (new i = 0; i < MAX_RENT_VEH; i++)
    {
        gRentVeh[i] = CreateVehicle(462, gRentPos[i][0], gRentPos[i][1], gRentPos[i][2], gRentPos[i][3], gRentColor[i], 1, -1);
        gRentBy[i] = -1; gRentLeft[i] = 0;
        Veh_SetFuel(gRentVeh[i], 100);
        SetVehicleNumberPlate(gRentVeh[i], "RENT");
    }
    new lbl[160];
    format(lbl, sizeof lbl, H_GOLD "Аренда мопедов\n" H_GREY "Садитесь на свободный мопед\n$%d за %d минут  •  /unrent — вернуть", RENT_PRICE, RENT_MINUTES);
    Point_AddLoc(PT_RENT, 0, "rent1", lbl, 1239);
    Point_AddLoc(PT_RENT, 1, "rent2", lbl, 1239);
}

stock Rent_Index(vehicleid)
{
    for (new i = 0; i < MAX_RENT_VEH; i++) if (gRentVeh[i] == vehicleid) return i;
    return -1;
}

stock Rent_Of(playerid)
{
    for (new i = 0; i < MAX_RENT_VEH; i++) if (gRentBy[i] == playerid) return i;
    return -1;
}

stock Rent_Desk(playerid)
{
    new r = Rent_Of(playerid), s[128];
    if (r != -1) format(s, sizeof s, "Ваш мопед арендован ещё на %d мин. Вернуть досрочно: /unrent", gRentLeft[r]);
    else format(s, sizeof s, "Садитесь на любой свободный мопед: $%d за %d минут. Права не нужны.", RENT_PRICE, RENT_MINUTES);
    Info(playerid, s);
    return 1;
}

static Rent_Engine(v, on)
{
    new e, l, al, d, bn, bt, o;
    GetVehicleParamsEx(v, e, l, al, d, bn, bt, o);
    SetVehicleParamsEx(v, on, l, al, d, bn, bt, o);
}

// вызывается после Veh_OnStateDriver
stock Rent_OnStateDriver(playerid, v)
{
    new r = Rent_Index(v);
    if (r == -1 || gRentBy[r] == playerid) return;
    Rent_Engine(v, 0);
    if (gRentBy[r] != -1) { RemovePlayerFromVehicle(playerid); Err(playerid, "Этот мопед уже кто-то арендовал."); return; }
    if (Rent_Of(playerid) != -1) { RemovePlayerFromVehicle(playerid); Err(playerid, "У вас уже есть арендованный мопед. /unrent — вернуть его."); return; }
    gRentOffer[playerid] = r;
    new t[160];
    format(t, sizeof t, "Мопед Faggio на %d минут за $%d. Оплата наличными или картой. Права не нужны.", RENT_MINUTES, RENT_PRICE);
    UI_Prompt(playerid, "rent", "Аренда мопеда", t, false, "", "Арендовать");
}

PROMPT:rent(playerid, response, const text[])
{
    UI_Close(playerid);
    new r = gRentOffer[playerid];
    gRentOffer[playerid] = -1;
    if (r == -1) return 1;
    new v = gRentVeh[r];
    new bool:inside = GetPlayerVehicleID(playerid) == v && GetPlayerState(playerid) == PLAYER_STATE_DRIVER;
    if (!response) { if (inside) RemovePlayerFromVehicle(playerid); return 1; }
    if (!inside) return Err(playerid, "Сначала сядьте на мопед.");
    if (gRentBy[r] != -1) { RemovePlayerFromVehicle(playerid); return Err(playerid, "Мопед уже арендован."); }
    if (!Pay(playerid, RENT_PRICE, "rent moped")) { RemovePlayerFromVehicle(playerid); return 1; }
    gRentBy[r] = playerid; gRentLeft[r] = RENT_MINUTES;
    Veh_SetFuel(v, 100);
    Rent_Engine(v, 1);
    new s[128]; format(s, sizeof s, "Мопед ваш на %d минут. Вернуть досрочно: /unrent", RENT_MINUTES);
    Ok(playerid, s);
    return 1;
}

stock Rent_Release(r, const reason[] = "")
{
    if (r < 0 || r >= MAX_RENT_VEH) return;
    new pid = gRentBy[r];
    gRentBy[r] = -1; gRentLeft[r] = 0;
    if (pid != -1 && IsPlayerConnected(pid))
    {
        if (GetPlayerVehicleID(pid) == gRentVeh[r]) RemovePlayerFromVehicle(pid);
        if (reason[0]) Info(pid, reason);
    }
    SetTimerEx("Rent_Respawn", pid != -1 && IsPlayerConnected(pid) ? 1500 : 100, false, "i", r);
}

forward Rent_Respawn(r);
public Rent_Respawn(r)
{
    if (gRentBy[r] != -1) return;
    SetVehicleToRespawn(gRentVeh[r]);
}

stock Rent_Minute()
{
    for (new r = 0; r < MAX_RENT_VEH; r++)
    {
        if (gRentBy[r] == -1) continue;
        if (!IsPlayerConnected(gRentBy[r])) { Rent_Release(r); continue; }
        gRentLeft[r]--;
        if (gRentLeft[r] == 5) Info(gRentBy[r], "Аренда мопеда закончится через 5 минут.");
        if (gRentLeft[r] <= 0) Rent_Release(r, "Время аренды вышло — мопед вернулся на стоянку.");
    }
}

stock Rent_OnDisconnect(playerid)
{
    gRentOffer[playerid] = -1;
    new r = Rent_Of(playerid);
    if (r != -1) Rent_Release(r);
}

stock bool:Rent_OnVehicleDeath(vehicleid)
{
    new r = Rent_Index(vehicleid);
    if (r == -1) return false;
    if (gRentBy[r] != -1) Rent_Release(r, "Арендованный мопед разбит.");
    else SetTimerEx("Rent_Respawn", 5000, false, "i", r);
    return true;
}

stock Rent_OnVehicleSpawn(vehicleid)
{
    new r = Rent_Index(vehicleid);
    if (r == -1) return;
    Veh_SetFuel(vehicleid, 100);
    Rent_Engine(vehicleid, 0);
}

CMD:unrent(playerid, const params[])
{
    new r = Rent_Of(playerid);
    if (r == -1) return Err(playerid, "У вас нет арендованного мопеда.");
    Rent_Release(r, "Вы вернули мопед. Спасибо, что выбрали прокат Godjo!");
    return 1;
}
