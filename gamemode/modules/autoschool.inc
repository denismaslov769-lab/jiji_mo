// =====================================================================
//  Автошкола: теория (тест на CEF) и практика по чекпоинтам
// =====================================================================
#define THEORY_PRICE 200
#define DRIVE_PRICE  300
#define THEORY_Q 10
enum E_QQ { qqText[128], qqA[64], qqB[64], qqC[64], qqRight }
static const gQQ[][E_QQ] = {
    {"Какая максимальная скорость в черте города?", "60 км/ч", "90 км/ч", "110 км/ч", 0},
    {"Что означает мигающий жёлтый сигнал светофора?", "Движение запрещено", "Нерегулируемый перекрёсток, будьте внимательны", "Можно ехать без остановки", 1},
    {"Кто имеет преимущество на нерегулируемом перекрёстке равнозначных дорог?", "Тот, кто едет быстрее", "Помеха справа", "Помеха слева", 1},
    {"При каком сигнале спецмашины нужно уступить дорогу?", "Включены маячки и сирена", "Только при включённых фарах", "Не нужно уступать", 0},
    {"Разрешено ли управлять транспортом в состоянии опьянения?", "Да, если немного", "Нет, никогда", "Только ночью", 1},
    {"Что нужно сделать при ДТП с пострадавшими?", "Уехать", "Остановиться, вызвать скорую (911) и полицию", "Сфотографировать и уехать", 1},
    {"Как обгонять транспорт?", "Справа", "Слева, убедившись в безопасности", "По обочине", 1},
    {"Где запрещена стоянка?", "На пешеходном переходе", "На парковке", "У своего дома", 0},
    {"Что означает сплошная линия разметки?", "Можно пересекать", "Пересекать запрещено", "Только для автобусов", 1},
    {"Обязан ли водитель пропустить пешехода на «зебре»?", "Да", "Нет", "Только детей", 0},
    {"Какое расстояние до впереди идущей машины безопасно?", "Половина корпуса", "Не менее 2 секунд хода", "Вплотную", 1},
    {"Когда включают ближний свет?", "В тёмное время суток и в тоннелях", "Только в дождь", "Никогда", 0},
    {"Что делать при требовании полицейского остановиться?", "Скрыться", "Остановиться у края дороги", "Игнорировать", 1},
    {"Можно ли разговаривать по телефону за рулём без гарнитуры?", "Да", "Нет", "Только на трассе", 1},
    {"С какой стороны дороги в Сан-Андреасе правостороннее движение?", "Справа", "Слева", "Посередине", 0},
    {"Что означает знак «STOP»?", "Остановка обязательна перед стоп-линией", "Можно не останавливаться", "Стоянка запрещена", 0},
    {"Ремень безопасности должен быть пристёгнут:", "Только у водителя", "У всех в салоне", "Только на трассе", 1},
    {"Разворот на мосту:", "Разрешён", "Запрещён", "Разрешён ночью", 1},
    {"Что делать, если отказали тормоза?", "Выпрыгнуть", "Тормозить двигателем и ручником, включить аварийку", "Разогнаться", 1},
    {"Красный сигнал светофора означает:", "Движение запрещено", "Приготовиться", "Ехать быстрее", 0}
};
new gTheoryPick[MAX_PLAYERS][THEORY_Q];

// маршрут экзамена — gExamRoute в gen_routes.inc (точки на реальных дорогах вокруг автошколы)

stock School_Desk(playerid)
{
    Menu_Begin("Автошкола", "Категория B");
    Menu_Add(1, "Теоретический экзамен", P[playerid][pTheory] ? "Сдан" : "10 вопросов, нужно 8 правильных", THEORY_PRICE, "book");
    Menu_Add(2, "Практический экзамен", P[playerid][pLicDrive] ? "Права уже получены" : "Маршрут по городу, без повреждений", DRIVE_PRICE, "car");
    Menu_Send(playerid, "school");
    return 1;
}

MENU:school(playerid, item, const extra[])
{
    UI_Close(playerid);
    if (!HasPassport(playerid)) return Err(playerid, "Инструктор: «Без паспорта не записываем».");
    if (!P[playerid][pMedcard]) return Err(playerid, "Инструктор: «Нужна медкарта. Больница All Saints».");
    if (item == 1)
    {
        if (P[playerid][pTheory]) return Info(playerid, "Теория уже сдана.");
        if (!Debt_PayOrLoan(playerid, THEORY_PRICE, "theory")) return 1;
        // выбрать 10 случайных вопросов без повторов
        new used[sizeof gQQ], n = 0;
        while (n < THEORY_Q)
        {
            new r = random(sizeof gQQ);
            if (used[r]) continue;
            used[r] = 1; gTheoryPick[playerid][n++] = r;
        }
        new j[4096], t[400];
        strcpy(j, "{\"questions\":[");
        for (new i = 0; i < THEORY_Q; i++)
        {
            new q = gTheoryPick[playerid][i];
            format(t, sizeof t, "%s{\"q\":\"%s\",\"a\":[\"%s\",\"%s\",\"%s\"]}", i ? "," : "", gQQ[q][qqText], gQQ[q][qqA], gQQ[q][qqB], gQQ[q][qqC]);
            strcat(j, t);
        }
        strcat(j, "],\"need\":8,\"time\":300}");
        UI_Open(playerid, "theory", j);
        SetPVarInt(playerid, "theoryStart", gettime());
    }
    else if (item == 2)
    {
        if (P[playerid][pLicDrive]) return Info(playerid, "Права уже получены.");
        if (!P[playerid][pTheory]) return Err(playerid, "Сначала сдайте теорию.");
        if (P[playerid][pExamVeh] != INVALID_VEHICLE_ID) return 1;
        if (!Debt_PayOrLoan(playerid, DRIVE_PRICE, "drive exam")) return 1;
        new Float:x, Float:y, Float:z, Float:a;
        Loc_Get("autoschool_car", x, y, z, a);
        new v = CreateVehicle(426, x, y, z + 0.5, a, 1, 1, -1);
        P[playerid][pExamVeh] = v;
        Veh_SetFuel(v, 100);
        SetPlayerInterior(playerid, 0); SetPlayerVirtualWorld(playerid, 0);
        PutPlayerInVehicle(playerid, v, 0);
        P[playerid][pExamStep] = 0;
        School_NextCP(playerid);
        SetPVarInt(playerid, "examStart", gettime());
        SCM(playerid, C_YELLOW, "Инструктор: «Пристегнись. Едем по маршруту, не гоним, машину не бьём. Поехали».");
    }
    return 1;
}

UI:theory(playerid, const a[])
{
    if (P[playerid][pTheory] || !GetPVarInt(playerid, "theoryStart")) return 1;
    new elapsed = gettime() - GetPVarInt(playerid, "theoryStart");
    DeletePVar(playerid, "theoryStart");
    new right = 0;
    for (new i = 0; i < THEORY_Q; i++)
    {
        new ans = CefFieldInt(a, 1 + i);
        if (ans == gQQ[gTheoryPick[playerid][i]][qqRight]) right++;
    }
    new j[96];
    if (elapsed > 330) right = 0;
    format(j, sizeof j, "{\"right\":%d,\"total\":%d,\"passed\":%s}", right, THEORY_Q, right >= 8 ? ("true") : ("false"));
    UI_Data(playerid, "theory_result", j);
    if (right >= 8)
    {
        P[playerid][pTheory] = 1; P_SetInt(playerid, "theory", 1);
        Ok(playerid, "Теория сдана! Теперь практический экзамен.");
        Quest_Event(playerid, "theory", "*");
    }
    else Err(playerid, "Экзамен не сдан. Можно пересдать.");
    return 1;
}

stock School_NextCP(playerid)
{
    new s = P[playerid][pExamStep], n = (s + 1 < sizeof gExamRoute) ? s + 1 : s;
    SetPlayerRaceCheckpoint(playerid, s == sizeof(gExamRoute) - 1 ? 1 : 0, gExamRoute[s][0], gExamRoute[s][1], gExamRoute[s][2], gExamRoute[n][0], gExamRoute[n][1], gExamRoute[n][2], 5.0);
    P[playerid][pRaceCP] = RCP_EXAM;
}

stock School_OnRaceCP(playerid)
{
    new v = GetPlayerVehicleID(playerid);
    if (v != P[playerid][pExamVeh]) return School_Fail(playerid, "Вы не в учебной машине.");
    new Float:hp; GetVehicleHealth(v, hp);
    if (hp < 900.0) return School_Fail(playerid, "Машина повреждена.");
    P[playerid][pExamStep]++;
    if (P[playerid][pExamStep] >= sizeof gExamRoute)
    {
        DisablePlayerRaceCheckpoint(playerid); P[playerid][pRaceCP] = RCP_NONE;
        if (gettime() - GetPVarInt(playerid, "examStart") > 420) return School_Fail(playerid, "Не уложились во время.");
        DestroyVehicle(P[playerid][pExamVeh]); P[playerid][pExamVeh] = INVALID_VEHICLE_ID;
        P[playerid][pLicDrive] = 1; P_SetInt(playerid, "lic_drive", 1);
        SCM(playerid, C_GREEN, "Инструктор: «Неплохо! Держи права. Не позорь автошколу».");
        Ach_Give(playerid, "driver", "За рулём");
        Quest_Event(playerid, "driving", "*");
        Loc_Teleport(playerid, "autoschool_door");
        return 1;
    }
    School_NextCP(playerid);
    return 1;
}

stock School_Fail(playerid, const reason[])
{
    if (P[playerid][pExamVeh] != INVALID_VEHICLE_ID) { DestroyVehicle(P[playerid][pExamVeh]); P[playerid][pExamVeh] = INVALID_VEHICLE_ID; }
    if (P[playerid][pRaceCP] == RCP_EXAM) { DisablePlayerRaceCheckpoint(playerid); P[playerid][pRaceCP] = RCP_NONE; }
    new s[128]; format(s, sizeof s, "Экзамен провален: %s", reason);
    Err(playerid, s);
    return 1;
}

stock School_OnExitVehicle(playerid, vehicleid)
{
    if (vehicleid == P[playerid][pExamVeh]) School_Fail(playerid, "Вы покинули учебную машину.");
}
