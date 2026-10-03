// =====================================================================
//  Поезда и междугородние автобусы: Лос-Сантос - Сан-Фиерро -
//  Лас-Вентурас - Диллимор (деревня). Кассир на каждом вокзале.
//  Без денег — льготный билет бесплатно (новичкам и малоимущим).
// =====================================================================
#define TRAVEL_PRICE 25
static const gStCode[][] = {"ls_station", "sf_station", "lv_station", "countryside_stop"};
static const gStName[][] = {"Лос-Сантос (вокзал Юнити)", "Сан-Фиерро (вокзал Крэнберри)", "Лас-Вентурас (вокзал Линден)", "Диллимор (автостанция)"};
static const gStCity[][] = {"ls", "sf", "lv", "country"};

stock Travel_Init()
{
    new lbl[128];
    for (new i = 0; i < sizeof gStCode; i++)
    {
        new Float:x, Float:y, Float:z, Float:a;
        if (!Loc_Get(gStCode[i], x, y, z, a)) continue;
        format(lbl, sizeof lbl, H_GOLD "%s\n" H_GREY "[ALT] Купить билет", i == 3 ? ("Автостанция") : ("Касса вокзала"));
        Point_Add(PT_TRAIN, i, x, y, z, 0, lbl, 1239);
        new act = CreateActor(i == 3 ? 161 : 171, x - 1.2 * floatsin(-a, degrees), y - 1.2 * floatcos(-a, degrees), z, a + 180.0);
        if (act != INVALID_ACTOR_ID) SetActorInvulnerable(act, true);
    }
}

stock Travel_Desk(playerid, from)
{
    new bool:free = (P[playerid][pCash] + P[playerid][pBank] < 100) || P[playerid][pLevel] <= 2;
    Menu_Begin(from == 3 ? ("Автостанция") : ("Касса вокзала"), free ? ("Вам положен льготный билет — бесплатно") : ("Куда едем?"));
    for (new i = 0; i < sizeof gStCode; i++)
    {
        if (i == from) continue;
        Menu_Add(i, gStName[i], i == 3 || from == 3 ? ("Междугородний автобус · ~1 мин") : ("Поезд · ~1 мин"), free ? 0 : TRAVEL_PRICE, i == 3 || from == 3 ? ("car") : ("ticket"));
    }
    SetPVarInt(playerid, "trFrom", from);
    Menu_Send(playerid, "travel");
    return 1;
}

MENU:travel(playerid, item, const extra[])
{
    UI_Close(playerid);
    new from = GetPVarInt(playerid, "trFrom");
    if (item < 0 || item >= sizeof gStCode || item == from) return 1;
    if (!IsPlayerAtLoc(playerid, gStCode[from], 4.0)) return Err(playerid, "Вы отошли от кассы.");
    if (P[playerid][pCuffed] || P[playerid][pWanted] >= 3) return Err(playerid, "Кассир: «Вас разыскивает полиция. Билет не продам».");
    new bool:free = (P[playerid][pCash] + P[playerid][pBank] < 100) || P[playerid][pLevel] <= 2;
    if (!free && !Pay(playerid, TRAVEL_PRICE, "ticket")) return 1;
    if (P[playerid][pOnJob]) Job_End(playerid, true);
    TogglePlayerControllable(playerid, false);
    UI_Data(playerid, "fade", "{\"ms\":5000,\"text\":\"В пути...\"}");
    new s[128]; format(s, sizeof s, "Вы купили билет: %s. Поездка займёт около минуты по игровому времени.", gStName[item]);
    Info(playerid, s);
    SetPVarInt(playerid, "trTo", item);
    SetTimerEx("Travel_Arrive", 4500, false, "i", playerid);
    return 1;
}

forward Travel_Arrive(playerid);
public Travel_Arrive(playerid)
{
    if (!IsLogged(playerid)) return;
    new to = GetPVarInt(playerid, "trTo");
    Loc_Teleport(playerid, gStCode[to]);
    TogglePlayerControllable(playerid, true);
    new s[96]; format(s, sizeof s, "Добро пожаловать: %s.", gStName[to]);
    Ok(playerid, s);
    Quest_Event(playerid, "travel", gStCity[to]);
}

stock Travel_Nearest(playerid)
{
    for (new i = 0; i < sizeof gStCode; i++) if (IsPlayerAtLoc(playerid, gStCode[i], 60.0)) return i;
    return -1;
}
