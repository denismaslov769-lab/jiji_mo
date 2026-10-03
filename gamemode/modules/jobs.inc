// =====================================================================
//  Работы: грузчик, дворник, водитель автобуса, таксист, дальнобойщик, шахтёр,
//  курьер, мусорщик, фермер, электрик. Точки маршрутов — gen_routes.inc
//  (сняты с путевых узлов GTA SA: дороги и тротуары).
// =====================================================================
enum { JOB_NONE, JOB_LOADER, JOB_JANITOR, JOB_BUS, JOB_TAXI, JOB_TRUCKER, JOB_MINER, JOB_COURIER, JOB_GARBAGE, JOB_FARMER, JOB_ELECTRIC }
#define JOB_COUNT 10
#define GARBAGE_STOPS 8
static const gJobName[][] = {"Безработный", "Грузчик", "Дворник", "Водитель автобуса", "Таксист", "Дальнобойщик", "Шахтёр", "Курьер Godjo Express", "Водитель мусоровоза", "Фермер", "Электрик (Лас-Вентурас)"};
static const gJobKey[][] = {"none", "loader", "janitor", "bus", "taxi", "trucker", "miner", "courier", "garbage", "farmer", "electric"};
static const gJobLevel[] = {0, 1, 1, 2, 2, 3, 1, 1, 2, 1, 2};
static const gJobLic[] = {0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1};
static const gJobShift[] = {0, 10, 10, 1, 3, 1, 5, 6, 8, 8, 5};
static const gJobLoc[][] = {"", "job_loader", "job_janitor", "job_bus", "job_taxi", "job_trucker", "job_miner", "job_courier", "job_garbage", "job_farmer", "job_electric"};

enum { RCP_NONE, RCP_BUS, RCP_TAXI, RCP_TRUCK, RCP_EXAM, RCP_GARBAGE, RCP_ELECTRIC }

// NPC-пассажиры (такси и автобус)
static const gPaxSkins[] = {7,9,10,11,12,13,14,15,17,20,22,24,25,26,31,37,40,41,44,47,55,56,57,58,59,60,69,72,76,88,89,93,98,101,141,150,169,170,184,185,187,190,191,193,211,216,223,226,240,250};
static const gTaxiHello[][] = {"Добрый день! Мне нужно в %s, побыстрее, если можно.", "Здравствуйте. Отвезите в %s, пожалуйста.", "Шеф, в %s! Плачу хорошо, если без приключений.", "Привет. %s, только аккуратно — у меня торт.", "Здрасьте... В %s. Только не гоните, меня укачивает."};
static const gTaxiTalk[][] = {"Пассажир: «Давно таксуете? Я вот только переехал в город».", "Пассажир: «Слышали, говорят, в Вегасе электриков не хватает».", "Пассажир: «Ой, я забыл выключить утюг... шучу».", "Пассажир: «А вы музыку погромче можете? Обожаю эту песню».", "Пассажир: «Мэрия опять очередь устроила, полдня потерял»."};
new gPaxActor[MAX_PLAYERS][4];
new gPaxCount[MAX_PLAYERS];
stock Job_Name(job) { new t[32]; strcpy(t, gJobName[job]); return t; }

stock Jobs_Init()
{
    new lbl[128];
    for (new j = 1; j <= JOB_COUNT; j++)
    {
        format(lbl, sizeof lbl, H_GOLD "Работа: %s\n" H_GREY "[ALT] Отдел кадров", gJobName[j]);
        Point_AddLoc(PT_JOB, j, gJobLoc[j], lbl, 1275);
    }
}

stock Job_Desk(playerid, job)
{
    new sub[96];
    format(sub, sizeof sub, "Требуется: %d ур.%s", gJobLevel[job], gJobLic[job] ? (", права") : (""));
    Menu_Begin(gJobName[job], sub);
    if (P[playerid][pJob] != job) Menu_Add(1, "Устроиться на работу", "Нужен паспорт", -1, "ok");
    else
    {
        if (!P[playerid][pOnJob])
        {
            if (job == JOB_BUS)
            {
                Menu_Add(5, "Городской маршрут", "9 остановок по Лос-Сантосу · ~$250 за круг", -1, "play");
                Menu_Add(6, "Пригородный маршрут", "Диллимор, Блуберри, Монтгомери, Паломино · ~$600", -1, "play");
            }
            else Menu_Add(2, "Начать смену", "", -1, "play");
        }
        else Menu_Add(3, "Закончить смену", "", -1, "stop");
        Menu_Add(4, "Уволиться", "", -1, "x");
    }
    Menu_Send(playerid, "job");
    SetPVarInt(playerid, "jobDesk", job);
    return 1;
}

MENU:job(playerid, item, const extra[])
{
    new job = GetPVarInt(playerid, "jobDesk");
    UI_Close(playerid);
    if (job < 1 || job > JOB_COUNT) return 1;
    switch (item)
    {
        case 1:
        {
            if (P[playerid][pJob] && P[playerid][pJob] != job) return Err(playerid, "Сначала уволитесь с текущей работы.");
            if (!HasPassport(playerid)) return Err(playerid, "Без паспорта на работу не берут.");
            if (P[playerid][pLevel] < gJobLevel[job]) return Err(playerid, "Недостаточный уровень.");
            if (gJobLic[job] && !P[playerid][pLicDrive]) return Err(playerid, "Нужны водительские права.");
            if (P[playerid][pOrg] && Org_Type(P[playerid][pOrg]) == 1) return Err(playerid, "Госслужащим нельзя совмещать работу.");
            P[playerid][pJob] = job;
            P_SetInt(playerid, "job", job);
            new s[96]; format(s, sizeof s, "Вы устроились: %s. Нажмите ALT снова, чтобы начать смену.", gJobName[job]);
            Ok(playerid, s);
            Quest_Event(playerid, "job_join", gJobKey[job]);
        }
        case 2: Job_Start(playerid);
        case 5, 6: { SetPVarInt(playerid, "busRoute", item == 6); Job_Start(playerid); }
        case 3: Job_End(playerid, true);
        case 4:
        {
            Job_End(playerid, false);
            P[playerid][pJob] = 0; P_SetInt(playerid, "job", 0);
            Ok(playerid, "Вы уволились.");
        }
    }
    return 1;
}

stock Job_Start(playerid)
{
    new job = P[playerid][pJob];
    if (!job || P[playerid][pOnJob]) return 1;
    P[playerid][pOnJob] = true; P[playerid][pJobStep] = 0; P[playerid][pJobCount] = 0; P[playerid][pCarry] = false;
    new Float:x, Float:y, Float:z, Float:a;
    switch (job)
    {
        case JOB_LOADER:
        {
            Loc_Get("loader_pick", x, y, z);
            SetPlayerCheckpoint(playerid, x, y, z, 1.5); P[playerid][pCP] = CP_JOB;
            Info(playerid, "Берите ящики на складе (ALT) и относите к погрузке. $25 за ящик.");
        }
        case JOB_JANITOR:
        {
            Janitor_Next(playerid);
            Info(playerid, "Подметайте отмеченные места. $20 за точку.");
        }
        case JOB_COURIER, JOB_GARBAGE:
        {
            new courier = (job == JOB_COURIER);
            Loc_Get(courier ? ("courier_spawn") : ("garbage_spawn"), x, y, z, a);
            new v = CreateVehicle(courier ? 448 : 408, x, y, z + 0.5, a, courier ? 3 : 1, courier ? 1 : 1, -1);
            P[playerid][pJobVeh] = v;
            Veh_SetFuel(v, 100);
            PutPlayerInVehicle(playerid, v, 0);
            if (courier)
            {
                Courier_NewOrder(playerid);
                Info(playerid, "Развозите заказы по адресам на мопеде. Оплата зависит от расстояния, чаевые — за скорость.");
            }
            else
            {
                Garbage_Next(playerid);
                Info(playerid, "Объезжайте мусорные точки: остановитесь в метке, рабочие загрузят баки. 8 точек — и назад на базу.");
            }
        }
        case JOB_FARMER:
        {
            Farmer_Next(playerid);
            Info(playerid, "Собирайте урожай в отмеченных грядках (жмите ПРОБЕЛ в мини-игре) и сносите ящики в амбар. $35 за ящик.");
        }
        case JOB_ELECTRIC:
        {
            Loc_Get("electric_spawn", x, y, z, a);
            new v = CreateVehicle(552, x, y, z + 0.5, a, 1, 1, -1);
            P[playerid][pJobVeh] = v;
            Veh_SetFuel(v, 100);
            PutPlayerInVehicle(playerid, v, 0);
            Electric_Next(playerid);
            Info(playerid, "Езжайте к щитку, выйдите из фургона и почините проводку (соедините провода по цветам).");
        }
        case JOB_BUS, JOB_TAXI, JOB_TRUCKER:
        {
            new model = (job == JOB_BUS) ? (GetPVarInt(playerid, "busRoute") ? 437 : 431) : (job == JOB_TAXI) ? 420 : 403;
            Loc_Get(job == JOB_BUS ? ("bus_spawn") : job == JOB_TAXI ? ("taxi_spawn") : ("trucker_spawn"), x, y, z, a);
            new v = CreateVehicle(model, x, y, z + 0.5, a, 1, 1, -1);
            P[playerid][pJobVeh] = v;
            Veh_SetFuel(v, 100);
            PutPlayerInVehicle(playerid, v, 0);
            if (job == JOB_TRUCKER)
            {
                new tr = CreateVehicle(435, x - 8.0 * floatsin(-a, degrees), y - 8.0 * floatcos(-a, degrees), z + 0.5, a, 1, 1, -1);
                SetPVarInt(playerid, "trailer", tr);
                AttachTrailerToVehicle(tr, v);
                Truck_NewOrder(playerid);
            }
            else if (job == JOB_BUS)
            {
                P[playerid][pJobStep] = 0;
                Bus_NextCP(playerid);
                Info(playerid, GetPVarInt(playerid, "busRoute") ? ("Пригородный маршрут: Диллимор, Блуберри, Монтгомери, Паломино. На остановках забирайте пассажиров.") : ("Городской маршрут. Останавливайтесь на остановках — пассажиры платят за проезд."));
            }
            else
            {
                Taxi_NewOrder(playerid);
                Info(playerid, "Клиенты голосуют на тротуарах — подъезжайте к метке, они сядут сами. Игроки тоже могут вызвать такси: /taxi.");
            }
        }
        case JOB_MINER:
        {
            Miner_Next(playerid);
            Info(playerid, "Добывайте руду в отмеченных местах и сдавайте на приёмку. $40 за кусок.");
        }
    }
    return 1;
}

stock Job_End(playerid, bool:msg)
{
    if (!P[playerid][pOnJob]) return;
    P[playerid][pOnJob] = false;
    if (P[playerid][pCP] == CP_JOB) { DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE; }
    if (P[playerid][pRaceCP] != RCP_EXAM && P[playerid][pRaceCP] != RCP_NONE) { DisablePlayerRaceCheckpoint(playerid); P[playerid][pRaceCP] = RCP_NONE; }
    Pax_Clear(playerid);
    DeletePVar(playerid, "taxiPax");
    if (P[playerid][pJobVeh] != INVALID_VEHICLE_ID) { DestroyVehicle(P[playerid][pJobVeh]); P[playerid][pJobVeh] = INVALID_VEHICLE_ID; }
    new tr = GetPVarInt(playerid, "trailer");
    if (tr) { DestroyVehicle(tr); DeletePVar(playerid, "trailer"); }
    if (P[playerid][pCarry]) { RemovePlayerAttachedObject(playerid, 5); SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE); P[playerid][pCarry] = false; }
    Inv_Remove(playerid, Item_ByKey("ore"), Inv_Count(playerid, Item_ByKey("ore")));
    if (P[playerid][pJobTimer] != -1) { KillTimer(P[playerid][pJobTimer]); P[playerid][pJobTimer] = -1; TogglePlayerControllable(playerid, true); }
    if (msg) Info(playerid, "Смена окончена.");
}

stock Job_Count(playerid, pay)
{
    Money_Give(playerid, pay, "job");
    P[playerid][pJobCount]++;
    new job = P[playerid][pJob];
    new s[96]; format(s, sizeof s, "+%s | Выполнено: %d/%d", FormatMoney(pay), P[playerid][pJobCount], gJobShift[job]);
    Info(playerid, s);
    if (P[playerid][pJobCount] == gJobShift[job])
    {
        Ok(playerid, "Норма смены выполнена! Можно продолжать работать.");
        Money_Give(playerid, 50, "shift bonus");
        Lvl_AddExp(playerid, 1);
        Quest_Event(playerid, "job_shift", gJobKey[job]);
    }
}

// ---- грузчик ----
stock Loader_Pick(playerid)
{
    if (P[playerid][pJob] != JOB_LOADER || !P[playerid][pOnJob]) return Info(playerid, "Это рабочее место грузчиков.");
    if (P[playerid][pCarry]) return Err(playerid, "У вас уже ящик в руках.");
    P[playerid][pCarry] = true;
    SetPlayerAttachedObject(playerid, 5, 1271, 5, 0.1, 0.2, 0.25, -100.0, 0.0, 0.0, 0.5, 0.5, 0.5);
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_CARRY);
    new Float:x, Float:y, Float:z; Loc_Get("loader_drop", x, y, z);
    SetPlayerCheckpoint(playerid, x, y, z, 1.5); P[playerid][pCP] = CP_JOB;
    return 1;
}

stock Loader_Drop(playerid)
{
    if (P[playerid][pJob] != JOB_LOADER || !P[playerid][pOnJob] || !P[playerid][pCarry]) return 1;
    P[playerid][pCarry] = false;
    RemovePlayerAttachedObject(playerid, 5);
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
    ApplyAnimation(playerid, "CARRY", "putdwn", 4.0, 0, 0, 0, 0, 0);
    Job_Count(playerid, 25);
    new Float:x, Float:y, Float:z; Loc_Get("loader_pick", x, y, z);
    SetPlayerCheckpoint(playerid, x, y, z, 1.5); P[playerid][pCP] = CP_JOB;
    return 1;
}

// ---- дворник ----
stock Janitor_Next(playerid)
{
    new last = GetPVarInt(playerid, "jnLast"), k;
    do k = random(sizeof gJanitorSpots); while (k == last && sizeof gJanitorSpots > 1);
    SetPVarInt(playerid, "jnLast", k);
    SetPlayerCheckpoint(playerid, gJanitorSpots[k][0], gJanitorSpots[k][1], gJanitorSpots[k][2], 1.5); P[playerid][pCP] = CP_JOB;
    P[playerid][pJobStep] = 0;
}

forward Job_ActionDone(playerid);
public Job_ActionDone(playerid)
{
    P[playerid][pJobTimer] = -1;
    if (!P[playerid][pOnJob]) return;
    if (P[playerid][pJob] == JOB_GARBAGE) { TogglePlayerControllable(playerid, true); Garbage_Loaded(playerid); return; }
    TogglePlayerControllable(playerid, true);
    ClearAnimations(playerid);
    if (P[playerid][pJob] == JOB_JANITOR) { Job_Count(playerid, 20); Janitor_Next(playerid); }
    else if (P[playerid][pJob] == JOB_MINER)
    {
        if (Inv_Add(playerid, Item_ByKey("ore"), 1) > 0) Info(playerid, "Руда добыта. Отнесите на приёмку или добудьте ещё (макс. 10).");
        if (Inv_Count(playerid, Item_ByKey("ore")) >= 3)
        {
            new Float:x, Float:y, Float:z; Loc_Get("mine_drop", x, y, z);
            SetPlayerCheckpoint(playerid, x, y, z, 2.0); P[playerid][pCP] = CP_JOB; P[playerid][pJobStep] = 1;
        }
        else Miner_Next(playerid);
    }
}

// ---- шахтёр ----
stock Miner_Next(playerid)
{
    new k = random(sizeof gMinerSpots);
    SetPlayerCheckpoint(playerid, gMinerSpots[k][0], gMinerSpots[k][1], gMinerSpots[k][2], 2.0); P[playerid][pCP] = CP_JOB; P[playerid][pJobStep] = 0;
}

// ---- фермер (Блуберри) ----
stock Farmer_Next(playerid)
{
    new last = GetPVarInt(playerid, "fmLast"), k;
    do k = random(sizeof gFarmSpots); while (k == last && sizeof gFarmSpots > 1);
    SetPVarInt(playerid, "fmLast", k);
    SetPlayerCheckpoint(playerid, gFarmSpots[k][0], gFarmSpots[k][1], gFarmSpots[k][2], 2.0); P[playerid][pCP] = CP_JOB; P[playerid][pJobStep] = 0;
}

// ---- электрик (Лас-Вентурас) ----
stock Electric_Next(playerid)
{
    new k = random(sizeof gElecSpots);
    SetPVarInt(playerid, "elSpot", k);
    SetPVarInt(playerid, "elStart", gettime());
    SetPlayerCheckpoint(playerid, gElecSpots[k][0], gElecSpots[k][1], gElecSpots[k][2], 2.5); P[playerid][pCP] = CP_JOB; P[playerid][pJobStep] = 0;
    new zone[32], s[128]; GetPointZoneName(gElecSpots[k][0], gElecSpots[k][1], zone);
    format(s, sizeof s, "Диспетчер: «Авария на линии — %s. Щиток отмечен на карте».", zone);
    SCM(playerid, C_YELLOW, s);
}

stock Miner_Drop(playerid)
{
    if (P[playerid][pJob] != JOB_MINER || !P[playerid][pOnJob]) return 1;
    new it = Item_ByKey("ore"), n = Inv_Count(playerid, it);
    if (!n) return Err(playerid, "У вас нет руды.");
    Inv_Remove(playerid, it, n);
    for (new i = 0; i < n; i++) Job_Count(playerid, 40);
    Miner_Next(playerid);
    return 1;
}

stock Jobs_OnEnterCP(playerid)
{
    if (!P[playerid][pOnJob]) return 0;
    switch (P[playerid][pJob])
    {
        case JOB_LOADER: return 1; // взаимодействие по ALT
        case JOB_COURIER:
        {
            if (P[playerid][pCP] != CP_JOB) return 0;
            if (IsPlayerInAnyVehicle(playerid) && GetPlayerVehicleID(playerid) != P[playerid][pJobVeh]) return Err(playerid, "Заказы развозят на рабочем мопеде.");
            DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE;
            Courier_Delivered(playerid);
            return 1;
        }
        case JOB_FARMER:
        {
            if (P[playerid][pCP] != CP_JOB || IsPlayerInAnyVehicle(playerid)) return 1;
            if (P[playerid][pJobStep] == 1)
            {   // донёс ящик до амбара
                DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE;
                P[playerid][pCarry] = false;
                RemovePlayerAttachedObject(playerid, 5);
                SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
                ApplyAnimation(playerid, "CARRY", "putdwn", 4.0, 0, 0, 0, 0, 0);
                Job_Count(playerid, 35);
                Farmer_Next(playerid);
                return 1;
            }
            DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE;
            ApplyAnimation(playerid, "BOMBER", "BOM_Plant_Loop", 4.0, 1, 0, 0, 0, 0);
            Mini_Start(playerid, "mash", 1 + P[playerid][pJobCount] / 4, "job:farm", "Сбор урожая", "Быстро жмите ПРОБЕЛ, чтобы набить ящик овощами");
            return 1;
        }
        case JOB_ELECTRIC:
        {
            if (P[playerid][pCP] != CP_JOB) return 1;
            if (IsPlayerInAnyVehicle(playerid)) return Info(playerid, "Выйдите из фургона, чтобы открыть щиток.");
            DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE;
            ApplyAnimation(playerid, "COP_AMBIENT", "Copbrowse_loop", 4.0, 1, 0, 0, 0, 0);
            Mini_Start(playerid, "wires", 1 + P[playerid][pJobCount] / 3, "job:elec", "Ремонт щитка", "Соедините провода одинакового цвета: клик слева, затем справа");
            return 1;
        }
        case JOB_JANITOR, JOB_MINER:
        {
            if (P[playerid][pJob] == JOB_MINER && P[playerid][pJobStep] == 1) return 1;
            if (IsPlayerInAnyVehicle(playerid) || P[playerid][pJobTimer] != -1) return 1;
            DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE;
            TogglePlayerControllable(playerid, false);
            if (P[playerid][pJob] == JOB_JANITOR) ApplyAnimation(playerid, "BD_FIRE", "wash_up", 4.0, 1, 0, 0, 0, 0);
            else ApplyAnimation(playerid, "BASEBALL", "Bat_4", 4.0, 1, 0, 0, 0, 0);
            UI_Data(playerid, "progress", P[playerid][pJob] == JOB_JANITOR ? ("{\"text\":\"Подметаете...\",\"ms\":5000}") : ("{\"text\":\"Добываете руду...\",\"ms\":8000}"));
            P[playerid][pJobTimer] = SetTimerEx("Job_ActionDone", P[playerid][pJob] == JOB_JANITOR ? 5000 : 8000, false, "i", playerid);
            return 1;
        }
    }
    return 0;
}

// ---- автобус ----
stock Bus_Len(playerid) return GetPVarInt(playerid, "busRoute") ? sizeof gBusSuburb : sizeof gBusCity;
stock Bus_Pt(playerid, i, &Float:x, &Float:y, &Float:z)
{
    if (GetPVarInt(playerid, "busRoute")) { x = gBusSuburb[i][0]; y = gBusSuburb[i][1]; z = gBusSuburb[i][2]; return floatround(gBusSuburb[i][3]); }
    x = gBusCity[i][0]; y = gBusCity[i][1]; z = gBusCity[i][2]; return floatround(gBusCity[i][3]);
}
stock Bus_StopName(playerid, st)
{
    new t[32];
    if (GetPVarInt(playerid, "busRoute")) strcpy(t, gBusSuburbStopName[st]); else strcpy(t, gBusCityStopName[st]);
    return t;
}
stock Bus_StopPos(playerid, st, &Float:x, &Float:y, &Float:z, &Float:a)
{
    if (GetPVarInt(playerid, "busRoute")) { x = gBusSuburbStops[st][0]; y = gBusSuburbStops[st][1]; z = gBusSuburbStops[st][2]; a = gBusSuburbStops[st][3]; }
    else { x = gBusCityStops[st][0]; y = gBusCityStops[st][1]; z = gBusCityStops[st][2]; a = gBusCityStops[st][3]; }
}

stock Bus_NextCP(playerid)
{
    new s = P[playerid][pJobStep], len = Bus_Len(playerid);
    new n = (s + 1 < len) ? s + 1 : s;
    new Float:x, Float:y, Float:z, Float:nx, Float:ny, Float:nz;
    new st = Bus_Pt(playerid, s, x, y, z);
    Bus_Pt(playerid, n, nx, ny, nz);
    SetPlayerRaceCheckpoint(playerid, (s == len - 1 || st >= 0) ? 1 : 0, x, y, z, nx, ny, nz, st >= 0 ? 5.0 : 7.0);
    P[playerid][pRaceCP] = RCP_BUS;
    if (st >= 0)
    {   // на остановке ждут пассажиры
        Pax_Clear(playerid);
        new Float:sx, Float:sy, Float:sz, Float:sa;
        Bus_StopPos(playerid, st, sx, sy, sz, sa);
        new cnt = 1 + random(3);
        for (new i = 0; i < cnt; i++)
        {
            new Float:ox = float(i) * 1.1 - 1.1;
            new act = CreateActor(gPaxSkins[random(sizeof gPaxSkins)], sx + ox * floatcos(sa, degrees), sy + ox * floatsin(sa, degrees), sz, sa);
            if (act == INVALID_ACTOR_ID) continue;
            ApplyActorAnimation(act, "PED", i == 0 ? ("IDLE_tired") : ("XPRESSscratch"), 4.1, 1, 0, 0, 0, 0);
            gPaxActor[playerid][gPaxCount[playerid]++] = act;
        }
        new t[96]; format(t, sizeof t, "Следующая остановка: «%s». Пассажиров ждёт: %d.", Bus_StopName(playerid, st), cnt);
        Info(playerid, t);
    }
}

stock Pax_Clear(playerid)
{
    for (new i = 0; i < gPaxCount[playerid]; i++) if (IsValidActor(gPaxActor[playerid][i])) DestroyActor(gPaxActor[playerid][i]);
    gPaxCount[playerid] = 0;
}

forward Bus_Depart(playerid);
public Bus_Depart(playerid)
{
    P[playerid][pJobTimer] = -1;
    if (!P[playerid][pOnJob] || P[playerid][pJob] != JOB_BUS) return;
    TogglePlayerControllable(playerid, true);
    new n = gPaxCount[playerid];
    Pax_Clear(playerid);
    new pay = 20 + n * (GetPVarInt(playerid, "busRoute") ? 12 : 7);
    Money_Give(playerid, pay, "bus stop");
    new s[96]; format(s, sizeof s, "Двери закрываются. Пассажиров: %d · +%s", n, FormatMoney(pay));
    Info(playerid, s);
    P[playerid][pJobStep]++;
    if (P[playerid][pJobStep] >= Bus_Len(playerid)) Bus_Lap(playerid);
    Bus_NextCP(playerid);
}

stock Bus_Lap(playerid)
{
    P[playerid][pJobStep] = 0;
    new bonus = GetPVarInt(playerid, "busRoute") ? 350 : 150;
    Money_Give(playerid, bonus, "bus route");
    P[playerid][pJobCount]++;
    new s[64]; format(s, sizeof s, "Круг завершён: +%s.", FormatMoney(bonus)); Ok(playerid, s);
    if (P[playerid][pJobCount] == gJobShift[JOB_BUS]) { Lvl_AddExp(playerid, 1); Quest_Event(playerid, "job_shift", "bus"); }
    Quest_Event(playerid, "bus_lap", GetPVarInt(playerid, "busRoute") ? ("suburb") : ("city"));
}

// ---- такси ----
stock Taxi_NewOrder(playerid)
{
    Pax_Clear(playerid);
    new last = GetPVarInt(playerid, "taxiTo"), a, tries = 0;
    new Float:px, Float:py, Float:pz; GetPlayerPos(playerid, px, py, pz);
    // ближайшие клиенты в приоритете: ищем точку в 150–900 м от машины
    do { a = random(sizeof gStreetSpots); tries++; }
    while ((a == last || DistBetween(px, py, 0.0, gStreetSpots[a][0], gStreetSpots[a][1], 0.0) > 900.0 || DistBetween(px, py, 0.0, gStreetSpots[a][0], gStreetSpots[a][1], 0.0) < 150.0) && tries < 40);
    SetPVarInt(playerid, "taxiFrom", a);
    P[playerid][pJobStep] = 0;
    new act = CreateActor(gPaxSkins[random(sizeof gPaxSkins)], gStreetSpots[a][0], gStreetSpots[a][1], gStreetSpots[a][2], gStreetSpots[a][3]);
    if (act != INVALID_ACTOR_ID)
    {
        SetActorInvulnerable(act, true);
        ApplyActorAnimation(act, "ON_LOOKERS", "wave_loop", 4.1, 1, 0, 0, 0, 0);
        gPaxActor[playerid][0] = act; gPaxCount[playerid] = 1;
    }
    SetPVarInt(playerid, "taxiMood", random(4)); // 0 спокойный, 1 торопится, 2 болтун, 3 вредный
    SetPlayerRaceCheckpoint(playerid, 1, gStreetSpots[a][4], gStreetSpots[a][5], gStreetSpots[a][6], 0.0, 0.0, 0.0, 5.0);
    P[playerid][pRaceCP] = RCP_TAXI;
    new zone[32], s[128]; GetPointZoneName(gStreetSpots[a][0], gStreetSpots[a][1], zone);
    format(s, sizeof s, "Диспетчер: клиент голосует на тротуаре, район %s. Подъезжайте и остановитесь у метки.", zone);
    Info(playerid, s);
}

stock Taxi_PickDest(from)
{
    new b, tries = 0;
    do { b = random(sizeof gStreetSpots); tries++; }
    while ((b == from || DistBetween(gStreetSpots[from][0], gStreetSpots[from][1], 0.0, gStreetSpots[b][0], gStreetSpots[b][1], 0.0) < 400.0 || DistBetween(gStreetSpots[from][0], gStreetSpots[from][1], 0.0, gStreetSpots[b][0], gStreetSpots[b][1], 0.0) > 1900.0) && tries < 60);
    return b;
}

forward Taxi_Chat(playerid);
public Taxi_Chat(playerid)
{
    if (!IsPlayerConnected(playerid) || !P[playerid][pOnJob] || P[playerid][pJob] != JOB_TAXI || P[playerid][pJobStep] != 1) return;
    if (GetPVarInt(playerid, "taxiMood") == 2) SCM(playerid, 0xE6E6E6FF, gTaxiTalk[random(sizeof gTaxiTalk)]);
    else if (GetPVarInt(playerid, "taxiMood") == 1 && gettime() - GetPVarInt(playerid, "taxiStart") > GetPVarInt(playerid, "taxiLimit") / 2) SCM(playerid, 0xE6E6E6FF, "Пассажир: «Можно побыстрее? Я опаздываю!»");
    SetTimerEx("Taxi_Chat", 25000 + random(15000), false, "i", playerid);
}

forward Taxi_PaxLeave(actorid);
public Taxi_PaxLeave(actorid) if (IsValidActor(actorid)) DestroyActor(actorid);

stock Jobs_OnEnterRaceCP(playerid)
{
    new v = GetPlayerVehicleID(playerid);
    switch (P[playerid][pRaceCP])
    {
        case RCP_BUS:
        {
            if (v != P[playerid][pJobVeh]) return Err(playerid, "Нужен рабочий автобус.");
            if (P[playerid][pJobTimer] != -1) return 1;
            new Float:x, Float:y, Float:z;
            new st = Bus_Pt(playerid, P[playerid][pJobStep], x, y, z);
            if (st >= 0)
            {   // остановка: посадка пассажиров
                DisablePlayerRaceCheckpoint(playerid);
                TogglePlayerControllable(playerid, false);
                new t[96]; format(t, sizeof t, "Остановка «%s». Посадка пассажиров...", Bus_StopName(playerid, st));
                new j[160]; format(j, sizeof j, "{\"text\":\"%s\",\"ms\":5000}", JsonEsc(t));
                UI_Data(playerid, "progress", j);
                for (new i = 0; i < gPaxCount[playerid]; i++) ApplyActorAnimation(gPaxActor[playerid][i], "PED", "WALK_civi", 4.1, 1, 1, 1, 0, 0);
                P[playerid][pJobTimer] = SetTimerEx("Bus_Depart", 5000, false, "i", playerid);
                return 1;
            }
            P[playerid][pJobStep]++;
            if (P[playerid][pJobStep] >= Bus_Len(playerid)) Bus_Lap(playerid);
            Bus_NextCP(playerid);
        }
        case RCP_TAXI:
        {
            if (v != P[playerid][pJobVeh]) return Err(playerid, "Нужен рабочий автомобиль такси.");
            new a = GetPVarInt(playerid, "taxiFrom");
            if (P[playerid][pJobStep] == 0)
            {
                new b = Taxi_PickDest(a);
                SetPVarInt(playerid, "taxiTo", b);
                Pax_Clear(playerid);
                P[playerid][pJobStep] = 1;
                new Float:hp; GetVehicleHealth(v, hp); SetPVarFloat(playerid, "taxiHp", hp);
                new dist = floatround(DistBetween(gStreetSpots[a][0], gStreetSpots[a][1], 0.0, gStreetSpots[b][0], gStreetSpots[b][1], 0.0));
                SetPVarInt(playerid, "taxiStart", gettime());
                SetPVarInt(playerid, "taxiLimit", 30 + dist / 11);
                SetPlayerRaceCheckpoint(playerid, 1, gStreetSpots[b][4], gStreetSpots[b][5], gStreetSpots[b][6], 0.0, 0.0, 0.0, 5.0);
                new zone[32], t[160]; GetPointZoneName(gStreetSpots[b][0], gStreetSpots[b][1], zone);
                format(t, sizeof t, gTaxiHello[random(sizeof gTaxiHello)], zone);
                new line[180]; format(line, sizeof line, "Пассажир: «%s»", t);
                SCM(playerid, 0xE6E6E6FF, line);
                format(t, sizeof t, "Клиент сел. Адрес: %s, ~%d м. Счётчик включён.", zone, dist);
                Info(playerid, t);
                SetTimerEx("Taxi_Chat", 20000, false, "i", playerid);
            }
            else
            {
                new b = GetPVarInt(playerid, "taxiTo"), mood = GetPVarInt(playerid, "taxiMood");
                new pay = 40 + floatround(DistBetween(gStreetSpots[a][0], gStreetSpots[a][1], 0.0, gStreetSpots[b][0], gStreetSpots[b][1], 0.0) / 10.0);
                new Float:hp; GetVehicleHealth(v, hp);
                new took = gettime() - GetPVarInt(playerid, "taxiStart");
                if (GetPVarFloat(playerid, "taxiHp") - hp > 80.0)
                {
                    pay = pay * (mood == 3 ? 50 : 70) / 100;
                    SCM(playerid, 0xE6E6E6FF, mood == 3 ? ("Пассажир: «Ты водить вообще умеешь?! Пожалуюсь в таксопарк!»") : ("Пассажир: «Ну и поездочка... Держите, но без чаевых»."));
                }
                else if (took <= GetPVarInt(playerid, "taxiLimit"))
                {
                    new tip = (mood == 1) ? 25 + random(30) : 5 + random(15);
                    pay += tip;
                    new t[96]; format(t, sizeof t, "Пассажир: «Спасибо, быстро доехали! Сдачи не надо» (+%s чаевых).", FormatMoney(tip));
                    SCM(playerid, 0xE6E6E6FF, t);
                }
                else SCM(playerid, 0xE6E6E6FF, "Пассажир: «Спасибо, до свидания».");
                // пассажир выходит и уходит по тротуару
                new act = CreateActor(gPaxSkins[random(sizeof gPaxSkins)], gStreetSpots[b][0], gStreetSpots[b][1], gStreetSpots[b][2], gStreetSpots[b][3] + 180.0);
                if (act != INVALID_ACTOR_ID) { ApplyActorAnimation(act, "PED", "WALK_civi", 4.1, 1, 1, 1, 0, 0); SetTimerEx("Taxi_PaxLeave", 6000, false, "i", act); }
                Job_Count(playerid, pay);
                Quest_Event(playerid, "taxi_fare", "*");
                SetPVarInt(playerid, "taxiTo", b);
                Taxi_NewOrder(playerid);
            }
        }
        case RCP_TRUCK:
        {
            if (v != P[playerid][pJobVeh]) return Err(playerid, "Нужен рабочий тягач.");
            new tr = GetPVarInt(playerid, "trailer");
            if (!tr || GetVehicleTrailer(v) != tr) return Err(playerid, "Прицеп не прицеплен!");
            Job_Count(playerid, GetPVarInt(playerid, "truckPay"));
            Quest_Event(playerid, "truck", gTruckDestName[GetPVarInt(playerid, "truckDest")]);
            Truck_NewOrder(playerid);
        }
        case RCP_GARBAGE:
        {
            if (v != P[playerid][pJobVeh]) return Err(playerid, "Нужен рабочий мусоровоз.");
            if (P[playerid][pJobTimer] != -1) return 1;
            DisablePlayerRaceCheckpoint(playerid);
            if (P[playerid][pJobStep] >= GARBAGE_STOPS)
            {   // вернулся на базу
                P[playerid][pRaceCP] = RCP_NONE;
                Money_Give(playerid, 150, "garbage route");
                Ok(playerid, "Мусор сдан на базу: +$150. Новый маршрут уже в навигаторе.");
                P[playerid][pJobStep] = 0;
                Garbage_Next(playerid);
                return 1;
            }
            TogglePlayerControllable(playerid, false);
            UI_Data(playerid, "progress", "{\"text\":\"Рабочие загружают баки...\",\"ms\":4000}");
            P[playerid][pJobTimer] = SetTimerEx("Job_ActionDone", 4000, false, "i", playerid);
        }
        case RCP_EXAM: School_OnRaceCP(playerid);
        case RCP_ELECTRIC: return 1;
    }
    return 1;
}

// ---- курьер Godjo Express ----
stock Truck_NewOrder(playerid)
{
    new Float:x, Float:y, Float:z; GetPlayerPos(playerid, x, y, z);
    new last = GetPVarInt(playerid, "truckDest"), d;
    do d = random(sizeof gTruckDest); while (d == last && sizeof gTruckDest > 1);
    SetPVarInt(playerid, "truckDest", d);
    new dist = floatround(DistBetween(x, y, 0.0, gTruckDest[d][0], gTruckDest[d][1], 0.0));
    SetPVarInt(playerid, "truckPay", 250 + dist / 4 + random(100));
    SetPlayerRaceCheckpoint(playerid, 1, gTruckDest[d][0], gTruckDest[d][1], gTruckDest[d][2], 0.0, 0.0, 0.0, 8.0);
    P[playerid][pRaceCP] = RCP_TRUCK;
    new s[144]; format(s, sizeof s, "Заказ: груз в %s, ~%d м. Оплата %s. Не теряйте прицеп!", gTruckDestName[d], dist, FormatMoney(GetPVarInt(playerid, "truckPay")));
    Info(playerid, s);
}

stock Courier_NewOrder(playerid)
{
    new Float:x, Float:y, Float:z, Float:bx, Float:by, Float:bz;
    Loc_Get("job_courier", bx, by, bz);
    // адрес: случайный дом штата или тротуар у дороги
    if (!random(2) || !House_RandomDoor(x, y, z)) { new k = random(sizeof gStreetSpots); x = gStreetSpots[k][0]; y = gStreetSpots[k][1]; z = gStreetSpots[k][2]; }
    new dist = floatround(DistBetween(bx, by, 0.0, x, y, 0.0));
    SetPVarInt(playerid, "crPay", 35 + dist / 14);
    SetPVarInt(playerid, "crStart", gettime());
    SetPVarInt(playerid, "crLimit", 40 + dist / 9);
    SetPlayerCheckpoint(playerid, x, y, z, 3.0); P[playerid][pCP] = CP_JOB;
    new zone[32], s[144]; GetPointZoneName(x, y, zone);
    format(s, sizeof s, "Новый заказ: %s, ~%d м. Успеете за %d сек — получите чаевые.", zone, dist, GetPVarInt(playerid, "crLimit"));
    Info(playerid, s);
}

stock Courier_Delivered(playerid)
{
    new pay = GetPVarInt(playerid, "crPay"), took = gettime() - GetPVarInt(playerid, "crStart");
    if (took <= GetPVarInt(playerid, "crLimit")) { pay += 15 + random(16); Info(playerid, "Клиент доволен скоростью и оставил чаевые!"); }
    ApplyAnimation(playerid, "DEALER", "DEALER_DEAL", 4.0, 0, 0, 0, 0, 0);
    Job_Count(playerid, pay);
    Courier_NewOrder(playerid);
}

// ---- мусоровоз ----
stock Garbage_Next(playerid)
{
    new Float:x, Float:y, Float:z;
    if (P[playerid][pJobStep] >= GARBAGE_STOPS)
    {
        Loc_Get("garbage_spawn", x, y, z);
        SetPlayerRaceCheckpoint(playerid, 1, x, y, z, 0.0, 0.0, 0.0, 8.0);
        Info(playerid, "Кузов полон — возвращайтесь на базу.");
    }
    else
    {
        // мусорные точки — у тротуаров на улицах города, подряд не повторяются
        new last = GetPVarInt(playerid, "gbLast"), k;
        do k = random(sizeof gStreetSpots); while (k == last);
        SetPVarInt(playerid, "gbLast", k);
        SetPlayerRaceCheckpoint(playerid, 1, gStreetSpots[k][4], gStreetSpots[k][5], gStreetSpots[k][6], 0.0, 0.0, 0.0, 7.0);
    }
    P[playerid][pRaceCP] = RCP_GARBAGE;
}

stock Garbage_Loaded(playerid)
{
    P[playerid][pJobStep]++;
    Job_Count(playerid, 30);
    Garbage_Next(playerid);
}

stock GetPointZoneName(Float:x, Float:y, out[], size = sizeof out)
{
    if (x > 2200.0 && y > -1800.0 && y < -1600.0) strcpy(out, "Гантон", size);
    else if (x > 1700.0 && x < 1900.0 && y < -1800.0 && y > -2000.0) strcpy(out, "Юнити", size);
    else if (x > 1300.0 && x < 1600.0 && y < -1550.0 && y > -1850.0) strcpy(out, "Першинг-сквер", size);
    else if (y > -1300.0 && x > 700.0 && x < 1500.0) strcpy(out, "Вайнвуд", size);
    else if (y > -1300.0 && x >= 1500.0) strcpy(out, "Джефферсон / Малхолланд", size);
    else if (x > 2600.0 && y < -2000.0) strcpy(out, "Порт", size);
    else if (x < 1000.0 && y < -1600.0) strcpy(out, "Санта-Мария", size);
    else strcpy(out, "Лос-Сантос", size);
}

// /jobs — где найти работу
CMD:jobs(playerid, const params[])
{
    Menu_Begin("Работы штата", "Выберите — метка отдела кадров появится на карте");
    new sub[64];
    for (new j = 1; j <= JOB_COUNT; j++)
    {
        format(sub, sizeof sub, "С %d ур.%s%s", gJobLevel[j], gJobLic[j] ? (" · нужны права") : (""), P[playerid][pJob] == j ? (" · ваша работа") : (""));
        Menu_Add(j, gJobName[j], sub, -1, "job");
    }
    Menu_Send(playerid, "jobs");
    return 1;
}
MENU:jobs(playerid, item, const extra[])
{
    UI_Close(playerid);
    if (item < 1 || item > JOB_COUNT) return 1;
    if (P[playerid][pCP] == CP_JOB) return Err(playerid, "Сначала закончите рабочее задание.");
    new Float:x, Float:y, Float:z;
    if (!Loc_Get(gJobLoc[item], x, y, z)) return 1;
    SetPlayerCheckpoint(playerid, x, y, z, 4.0); P[playerid][pCP] = CP_GPS;
    return Ok(playerid, "Отдел кадров отмечен на карте.");
}

stock Job_Count2(playerid, pay)
{
    Money_Give(playerid, pay, "");
    new s[48]; format(s, sizeof s, "Остановка: +%s", FormatMoney(pay));
    Info(playerid, s);
}

stock Jobs_OnExitVehicle(playerid, vehicleid)
{
    if (P[playerid][pOnJob] && vehicleid == P[playerid][pJobVeh])
    {
        Info(playerid, "Вернитесь в рабочий транспорт в течение 60 секунд, иначе смена закончится.");
        if (P[playerid][pJobTimer] != -1) KillTimer(P[playerid][pJobTimer]);
        P[playerid][pJobTimer] = SetTimerEx("Job_VehTimeout", 60000, false, "i", playerid);
    }
}

stock Jobs_OnEnterVehicle(playerid, vehicleid)
{
    if (P[playerid][pOnJob] && vehicleid == P[playerid][pJobVeh] && P[playerid][pJobTimer] != -1)
    {
        KillTimer(P[playerid][pJobTimer]); P[playerid][pJobTimer] = -1;
    }
}

forward Job_VehTimeout(playerid);
public Job_VehTimeout(playerid)
{
    P[playerid][pJobTimer] = -1;
    if (P[playerid][pOnJob] && GetPlayerVehicleID(playerid) != P[playerid][pJobVeh]) Job_End(playerid, true);
}

// /taxi — вызвать такси (уведомление всем таксистам на смене)
CMD:taxi(playerid, const params[])
{
    new cnt = 0, s[128], zone[32];
    GetPlayerZoneName(playerid, zone);
    format(s, sizeof s, "[Диспетчер] Вызов такси: %s (ID %d), район %s. Принять: /accepttaxi %d", PName(playerid), playerid, zone, playerid);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsLogged(i) && P[i][pJob] == JOB_TAXI && P[i][pOnJob]) { SCM(i, C_YELLOW, s); cnt++; }
    if (!cnt) return Err(playerid, "Свободных такси нет.");
    return Ok(playerid, "Вызов отправлен таксистам.");
}

CMD:accepttaxi(playerid, const params[])
{
    if (P[playerid][pJob] != JOB_TAXI || !P[playerid][pOnJob]) return Err(playerid, "Вы не таксист на смене.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID) return Err(playerid, "/accepttaxi [id]");
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z);
    SetPlayerCheckpoint(playerid, x, y, z, 4.0); P[playerid][pCP] = CP_GPS;
    new s[96]; format(s, sizeof s, "Таксист %s принял ваш вызов и уже едет.", PName(playerid)); Ok(t, s);
    return Ok(playerid, "Клиент отмечен на карте. Плату за проезд назначьте: /fare [id] [сумма]");
}

CMD:fare(playerid, const params[])
{
    if (P[playerid][pJob] != JOB_TAXI || !P[playerid][pOnJob]) return Err(playerid, "Вы не таксист на смене.");
    new t = GetParamPlayer(params, 0), sum = GetParamInt(params, 1, 0);
    if (t == INVALID_PLAYER_ID || sum < 10 || sum > 1000) return Err(playerid, "/fare [id] [10-1000]");
    if (GetPlayerVehicleID(t) != P[playerid][pJobVeh]) return Err(playerid, "Клиент не в вашем такси.");
    P[t][pTaxiFare] = sum; P[t][pTaxiDriver] = playerid;
    new s[128]; format(s, sizeof s, "Таксист предлагает поездку за %s. Согласиться: /payfare", FormatMoney(sum)); Info(t, s);
    return 1;
}

CMD:payfare(playerid, const params[])
{
    new d = P[playerid][pTaxiDriver], sum = P[playerid][pTaxiFare];
    if (d == INVALID_PLAYER_ID || !sum || !IsPlayerConnected(d)) return Err(playerid, "Нет предложения.");
    if (!Money_Take(playerid, sum)) return Err(playerid, "Недостаточно наличных.");
    Money_Give(d, sum, "fare");
    P[playerid][pTaxiFare] = 0; P[playerid][pTaxiDriver] = INVALID_PLAYER_ID;
    Ok(d, "Клиент оплатил поездку.");
    return Ok(playerid, "Поездка оплачена.");
}

CMD:jobend(playerid, const params[]) { Job_End(playerid, true); return 1; }

stock GetPlayerZoneName(playerid, out[], size = sizeof out)
{
    new Float:x, Float:y, Float:z; GetPlayerPos(playerid, x, y, z);
    if (x > 2200.0 && y > -1800.0 && y < -1600.0) strcpy(out, "Гантон", size);
    else if (x > 1700.0 && x < 1900.0 && y < -1800.0 && y > -2000.0) strcpy(out, "Юнити", size);
    else if (x > 1300.0 && x < 1600.0 && y < -1550.0 && y > -1850.0) strcpy(out, "Першинг-сквер", size);
    else if (y > -1300.0 && x > 700.0 && x < 1500.0) strcpy(out, "Вайнвуд", size);
    else if (x > 2600.0 && y < -2000.0) strcpy(out, "Порт", size);
    else if (x < 1000.0 && y < -1600.0) strcpy(out, "Санта-Мария", size);
    else strcpy(out, "Лос-Сантос", size);
}
