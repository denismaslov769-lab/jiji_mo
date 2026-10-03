// =====================================================================
//  Работы: грузчик, дворник, водитель автобуса, таксист, дальнобойщик, шахтёр
// =====================================================================
enum { JOB_NONE, JOB_LOADER, JOB_JANITOR, JOB_BUS, JOB_TAXI, JOB_TRUCKER, JOB_MINER }
static const gJobName[][] = {"Безработный", "Грузчик", "Дворник", "Водитель автобуса", "Таксист", "Дальнобойщик", "Шахтёр"};
static const gJobKey[][] = {"none", "loader", "janitor", "bus", "taxi", "trucker", "miner"};
static const gJobLevel[] = {0, 1, 1, 2, 2, 3, 1};
static const gJobLic[] = {0, 0, 0, 1, 1, 1, 0};
static const gJobShift[] = {0, 10, 10, 1, 3, 1, 5};
static const gJobLoc[][] = {"", "job_loader", "job_janitor", "job_bus", "job_taxi", "job_trucker", "job_miner"};

enum { CP_NONE, CP_JOB, CP_GPS }
enum { RCP_NONE, RCP_BUS, RCP_TAXI, RCP_TRUCK, RCP_EXAM }

static const Float:gBusRoute[][3] = {
    {1803.0,-1838.0,13.4},{1955.0,-1758.0,13.4},{2085.0,-1752.0,13.4},{2213.0,-1650.0,15.2},{2108.0,-1380.0,23.8},
    {1868.0,-1374.0,13.4},{1560.0,-1415.0,13.4},{1427.0,-1580.0,13.4},{1530.0,-1730.0,13.4},{1690.0,-1735.0,13.4},{1797.5,-1898.0,13.4}
};
static const Float:gTruckDest[][3] = {{247.0,-245.0,1.58},{660.0,-560.0,16.3},{1374.0,272.0,19.56},{2290.0,30.0,26.5},{-90.0,-1180.0,2.0}};
static const Float:gTaxiPts[][3] = {
    {1480.0,-1740.0,13.4},{1170.0,-1340.0,13.6},{2230.0,-1160.0,25.7},{2490.0,-1670.0,13.3},{1020.0,-1130.0,23.8},
    {830.0,-1800.0,13.4},{1290.0,-1840.0,13.4},{2130.0,-1140.0,25.2},{1940.0,-1770.0,13.4},{2780.0,-2430.0,13.6}
};

stock Job_Name(job) { new t[32]; strcpy(t, gJobName[job]); return t; }

stock Jobs_Init()
{
    new lbl[128];
    for (new j = 1; j <= 6; j++)
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
        if (!P[playerid][pOnJob]) Menu_Add(2, "Начать смену", "", -1, "play");
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
    if (job < 1 || job > 6) return 1;
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
        case JOB_BUS, JOB_TAXI, JOB_TRUCKER:
        {
            new model = (job == JOB_BUS) ? 431 : (job == JOB_TAXI) ? 420 : 403;
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
                new d = random(sizeof gTruckDest);
                SetPVarInt(playerid, "truckDest", d);
                SetPlayerRaceCheckpoint(playerid, 1, gTruckDest[d][0], gTruckDest[d][1], gTruckDest[d][2], 0.0, 0.0, 0.0, 8.0);
                P[playerid][pRaceCP] = RCP_TRUCK;
                Info(playerid, "Доставьте груз в отмеченную точку. Не теряйте прицеп!");
            }
            else if (job == JOB_BUS)
            {
                Bus_NextCP(playerid);
                Info(playerid, "Проезжайте остановки маршрута. $15 за остановку, $100 за круг.");
            }
            else
            {
                Taxi_NewOrder(playerid);
                Info(playerid, "Выполняйте заказы диспетчера. Игроки могут вызвать такси: /taxi.");
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
    if (P[playerid][pJobVeh] != INVALID_VEHICLE_ID) { DestroyVehicle(P[playerid][pJobVeh]); P[playerid][pJobVeh] = INVALID_VEHICLE_ID; }
    new tr = GetPVarInt(playerid, "trailer");
    if (tr) { DestroyVehicle(tr); DeletePVar(playerid, "trailer"); }
    if (P[playerid][pCarry]) { RemovePlayerAttachedObject(playerid, 5); SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE); P[playerid][pCarry] = false; }
    Inv_Remove(playerid, Item_ByKey("ore"), Inv_Count(playerid, Item_ByKey("ore")));
    if (P[playerid][pJobTimer] != -1) { KillTimer(P[playerid][pJobTimer]); P[playerid][pJobTimer] = -1; }
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
    new Float:x, Float:y, Float:z; Loc_Get("job_janitor", x, y, z);
    x += float(random(50) - 25); y += float(random(50) - 25);
    SetPlayerCheckpoint(playerid, x, y, z, 1.5); P[playerid][pCP] = CP_JOB;
    P[playerid][pJobStep] = 0;
}

forward Job_ActionDone(playerid);
public Job_ActionDone(playerid)
{
    P[playerid][pJobTimer] = -1;
    if (!P[playerid][pOnJob]) return;
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
    new Float:x, Float:y, Float:z; Loc_Get("job_miner", x, y, z);
    x += float(random(40) - 20); y += float(random(40) - 20);
    SetPlayerCheckpoint(playerid, x, y, z, 2.0); P[playerid][pCP] = CP_JOB; P[playerid][pJobStep] = 0;
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
stock Bus_NextCP(playerid)
{
    new s = P[playerid][pJobStep];
    new n = (s + 1 < sizeof gBusRoute) ? s + 1 : s;
    SetPlayerRaceCheckpoint(playerid, (s == sizeof(gBusRoute) - 1) ? 1 : 0, gBusRoute[s][0], gBusRoute[s][1], gBusRoute[s][2], gBusRoute[n][0], gBusRoute[n][1], gBusRoute[n][2], 6.0);
    P[playerid][pRaceCP] = RCP_BUS;
}

// ---- такси ----
stock Taxi_NewOrder(playerid)
{
    new a = random(sizeof gTaxiPts);
    SetPVarInt(playerid, "taxiFrom", a);
    new b = random(sizeof gTaxiPts); if (b == a) b = (a + 1) % sizeof gTaxiPts;
    SetPVarInt(playerid, "taxiTo", b);
    P[playerid][pJobStep] = 0;
    SetPlayerRaceCheckpoint(playerid, 1, gTaxiPts[a][0], gTaxiPts[a][1], gTaxiPts[a][2], 0.0, 0.0, 0.0, 5.0);
    P[playerid][pRaceCP] = RCP_TAXI;
    Info(playerid, "Диспетчер: заберите клиента по адресу (отмечено).");
}

stock Jobs_OnEnterRaceCP(playerid)
{
    new v = GetPlayerVehicleID(playerid);
    switch (P[playerid][pRaceCP])
    {
        case RCP_BUS:
        {
            if (v != P[playerid][pJobVeh]) return Err(playerid, "Нужен рабочий автобус.");
            Job_Count2(playerid, 15);
            P[playerid][pJobStep]++;
            if (P[playerid][pJobStep] >= sizeof gBusRoute)
            {
                P[playerid][pJobStep] = 0;
                Money_Give(playerid, 100, "bus route");
                P[playerid][pJobCount]++;
                Ok(playerid, "Круг завершён: +$100.");
                if (P[playerid][pJobCount] == gJobShift[JOB_BUS]) { Lvl_AddExp(playerid, 1); Quest_Event(playerid, "job_shift", "bus"); }
            }
            Bus_NextCP(playerid);
        }
        case RCP_TAXI:
        {
            if (v != P[playerid][pJobVeh]) return Err(playerid, "Нужен рабочий автомобиль такси.");
            if (P[playerid][pJobStep] == 0)
            {
                new b = GetPVarInt(playerid, "taxiTo");
                P[playerid][pJobStep] = 1;
                SetPlayerRaceCheckpoint(playerid, 1, gTaxiPts[b][0], gTaxiPts[b][1], gTaxiPts[b][2], 0.0, 0.0, 0.0, 5.0);
                Info(playerid, "Клиент сел. Отвезите его по адресу.");
            }
            else
            {
                new a = GetPVarInt(playerid, "taxiFrom"), b = GetPVarInt(playerid, "taxiTo");
                new pay = 40 + floatround(DistBetween(gTaxiPts[a][0], gTaxiPts[a][1], 0.0, gTaxiPts[b][0], gTaxiPts[b][1], 0.0) / 12.0);
                Job_Count(playerid, pay);
                Taxi_NewOrder(playerid);
            }
        }
        case RCP_TRUCK:
        {
            if (v != P[playerid][pJobVeh]) return Err(playerid, "Нужен рабочий тягач.");
            new tr = GetPVarInt(playerid, "trailer");
            if (!tr || GetVehicleTrailer(v) != tr) return Err(playerid, "Прицеп не прицеплен!");
            Job_Count(playerid, 450 + random(250));
            new d = random(sizeof gTruckDest);
            SetPlayerRaceCheckpoint(playerid, 1, gTruckDest[d][0], gTruckDest[d][1], gTruckDest[d][2], 0.0, 0.0, 0.0, 8.0);
            Info(playerid, "Новый заказ получен.");
        }
        case RCP_EXAM: School_OnRaceCP(playerid);
    }
    return 1;
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
