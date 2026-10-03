// =====================================================================
//  Больница: состояние «ранен», вызов скорой, лечение
// =====================================================================
#define WOUND_TIME 120
new Float:gDeathPos[MAX_PLAYERS][4], gDeathInt[MAX_PLAYERS], gDeathVW[MAX_PLAYERS];
new Text3D:gWoundLabel[MAX_PLAYERS] = {Text3D:INVALID_3DTEXT_ID, ...};

stock Hospital_OnDeath(playerid, killerid)
{
    GetPlayerPos(playerid, gDeathPos[playerid][0], gDeathPos[playerid][1], gDeathPos[playerid][2]);
    GetPlayerFacingAngle(playerid, gDeathPos[playerid][3]);
    gDeathInt[playerid] = GetPlayerInterior(playerid);
    gDeathVW[playerid] = GetPlayerVirtualWorld(playerid);
    if (P[playerid][pJail] > 0) { P[playerid][pInjured] = 0; Spawn_Prepare(playerid); return; }
    P[playerid][pInjured] = 2; // ранен, ждёт скорую
    if (killerid != INVALID_PLAYER_ID && IsLogged(killerid))
    {
        new bool:legal = (IsCop(killerid) && P[killerid][pDuty] && P[playerid][pWanted] > 0);
        if (!legal && !(Org_Type(P[killerid][pOrg]) >= 2 && Org_Type(P[playerid][pOrg]) >= 2)) Police_AddWanted(killerid, 2, "нападение");
    }
    SetSpawnInfo(playerid, 0, GetPlayerSkin(playerid), gDeathPos[playerid][0], gDeathPos[playerid][1], gDeathPos[playerid][2], gDeathPos[playerid][3], 0, 0, 0, 0, 0, 0);
}

stock Hospital_OnSpawnWounded(playerid)
{
    SetPlayerInterior(playerid, gDeathInt[playerid]);
    SetPlayerVirtualWorld(playerid, gDeathVW[playerid]);
    SetPlayerPos(playerid, gDeathPos[playerid][0], gDeathPos[playerid][1], gDeathPos[playerid][2]);
    SetPlayerHealth(playerid, 15.0);
    SetPlayerSkin(playerid, P[playerid][pDuty] && P[playerid][pOrgSkin] ? P[playerid][pOrgSkin] : P[playerid][pSkin]);
    TogglePlayerControllable(playerid, false);
    ApplyAnimation(playerid, "PED", "KO_skid_back", 4.1, 0, 1, 1, 1, 0, 1);
    if (gWoundLabel[playerid] != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(gWoundLabel[playerid]);
    gWoundLabel[playerid] = Create3DTextLabel("{FF5A5A}РАНЕН\n{AFAFAF}нужна медицинская помощь", -1, 0.0, 0.0, 0.0, 15.0, gDeathVW[playerid], 1);
    Attach3DTextLabelToPlayer(gWoundLabel[playerid], playerid, 0.0, 0.0, 0.3);
    new j[96]; format(j, sizeof j, "{\"time\":%d}", WOUND_TIME);
    UI_Data(playerid, "wounded", j);
    SCM(playerid, C_RED, "Вы тяжело ранены. Вызовите скорую: /call911 (или дождитесь — через 2 минуты вас доставят в больницу).");
    if (P[playerid][pInjuredTimer] != -1) KillTimer(P[playerid][pInjuredTimer]);
    P[playerid][pInjuredTimer] = SetTimerEx("Hospital_Timeout", WOUND_TIME * 1000, false, "i", playerid);
    Hospital_Alert(playerid, "раненый");
}

stock Hospital_Alert(playerid, const what[])
{
    new s[128], zone[32]; GetPlayerZoneName(playerid, zone);
    format(s, sizeof s, "[Диспетчер 911] %s: %s [%d], район %s. /accept911 %d", what, PName(playerid), playerid, zone, playerid);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && IsMedic(i)) SCM(i, 0xFF7FA8FF, s);
}

forward Hospital_Timeout(playerid);
public Hospital_Timeout(playerid)
{
    P[playerid][pInjuredTimer] = -1;
    if (!IsPlayerConnected(playerid) || P[playerid][pInjured] != 2) return;
    Hospital_Clear(playerid);
    new fee = 200;
    if (P[playerid][pBank] >= fee) P[playerid][pBank] -= fee; else if (P[playerid][pCash] >= fee) Money_Take(playerid, fee); else fee = 0;
    gO[ORG_EMS][oBank] += fee;
    P[playerid][pInjured] = 1;
    Loc_Teleport(playerid, "hospital_spawn");
    SetPlayerHealth(playerid, 60.0);
    P[playerid][pInjured] = 0;
    new s[96]; format(s, sizeof s, "Вас доставили в больницу. Счёт за лечение: %s.", FormatMoney(fee));
    Info(playerid, s);
}

stock Hospital_Clear(playerid)
{
    if (gWoundLabel[playerid] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(gWoundLabel[playerid]); gWoundLabel[playerid] = Text3D:INVALID_3DTEXT_ID; }
    TogglePlayerControllable(playerid, true);
    ClearAnimations(playerid);
    UI_Data(playerid, "wounded", "{\"time\":0}");
}

CMD:call911(playerid, const params[])
{
    if (Inv_Count(playerid, Item_ByKey("phone")) <= 0 && P[playerid][pInjured] != 2) return Err(playerid, "Нужен телефон.");
    Hospital_Alert(playerid, P[playerid][pInjured] == 2 ? ("раненый") : ("вызов скорой"));
    new s[96]; format(s, sizeof s, "[911] Вызов от %s [%d]. /su при необходимости.", PName(playerid), playerid);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && IsCop(i) && P[i][pDuty]) SCM(i, C_BLUE, s);
    return Ok(playerid, "Вызов принят. Ожидайте.");
}

CMD:accept911(playerid, const params[])
{
    if (!IsMedic(playerid)) return Err(playerid, "Только для медиков.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID) return Err(playerid, "/accept911 [id]");
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z);
    SetPlayerCheckpoint(playerid, x, y, z, 3.0); P[playerid][pCP] = CP_GPS;
    new s[96]; format(s, sizeof s, "Бригада скорой (%s) выехала к вам.", PName(playerid)); Ok(t, s);
    return Ok(playerid, "Место вызова отмечено на карте.");
}

CMD:heal(playerid, const params[])
{
    if (!IsMedic(playerid)) return Err(playerid, "Только для медиков.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || t == playerid) return Err(playerid, "/heal [id]");
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z);
    if (!IsPlayerInRangeOfPoint(playerid, 3.0, x, y, z)) return Err(playerid, "Подойдите ближе.");
    if (P[t][pInjured] == 2)
    {
        if (P[t][pInjuredTimer] != -1) { KillTimer(P[t][pInjuredTimer]); P[t][pInjuredTimer] = -1; }
        P[t][pInjured] = 0;
        Hospital_Clear(t);
        SetPlayerHealth(t, 50.0);
        ApplyAnimation(playerid, "MEDIC", "CPR", 4.0, 0, 0, 0, 0, 0);
        new s[96]; format(s, sizeof s, "оказал первую помощь %s", PName(t)); SendMe(playerid, s);
        Money_Give(playerid, 150, "reanimation");
        Lvl_AddExp(playerid, 0);
        return 1;
    }
    SetPlayerHealth(t, 100.0);
    new s[96]; format(s, sizeof s, "вылечил %s", PName(t)); SendMe(playerid, s);
    Money_Give(playerid, 50, "heal");
    return 1;
}

// =====================================================================
//  Больница All Saints изнутри: регистратура, терапевт, окулист,
//  лаборатория, палата. Собственный интерьер (gen_interiors.inc,
//  генератор tools/interiors/gen_interiors.py), вирт. мир HOSP_VW.
//  Медкомиссия: регистратура -> терапевт (давление) -> окулист (таблица)
//  -> лаборатория (кровь) -> ожидание анализов -> терапевт -> медкарта.
//  Без денег — бесплатно по полису ОМС.
// =====================================================================
#define HOSP_INT GENINT_HOSP_INT
#define MED_PRICE 100
enum { MEDP_RECEPTION, MEDP_THER, MEDP_EYE, MEDP_LAB }
new gMedTimer[MAX_PLAYERS] = {-1, ...};

stock bool:Med_IsPoor(playerid) return (P[playerid][pCash] + P[playerid][pBank] < 150) || P[playerid][pLevel] <= 2;

stock Hospital_Build()
{
    GenInt_Hospital(HOSP_VW);   // стены, мебель, таблички, персонал
    Point_AddLoc(PT_MED, MEDP_RECEPTION, "med_reception", H_GOLD "Регистратура\n" H_GREY "[ALT] Медкомиссия / лечение", 1240);
    Point_AddLoc(PT_MED, MEDP_THER, "med_ther", H_GOLD "Кабинет терапевта\n" H_GREY "[ALT] На приём", -1);
    Point_AddLoc(PT_MED, MEDP_EYE, "med_eye", H_GOLD "Кабинет окулиста\n" H_GREY "[ALT] Проверить зрение", -1);
    Point_AddLoc(PT_MED, MEDP_LAB, "med_lab", H_GOLD "Лаборатория\n" H_GREY "[ALT] Сдать анализы", -1);
}

stock Med_Point(playerid, which)
{
    switch (which)
    {
        case MEDP_RECEPTION: return Hospital_Desk(playerid);
        case MEDP_THER:
        {
            new st = GetPVarInt(playerid, "medStage");
            if (st == 1)
            {
                SCM(playerid, C_WHITE, "Терапевт Смирнова: «Присаживайтесь. Сначала давление — накачайте манжету и отпустите, когда стрелка в зелёной зоне».");
                return Mini_Start(playerid, "hold", 2, "med:bp", "Измерение давления", "Зажмите кнопку мыши, отпустите в зелёной зоне");
            }
            if (st == 5)
            {
                Med_Done(playerid);
                return 1;
            }
            if (st >= 2 && st <= 4) return Info(playerid, "Терапевт: «Сначала пройдите окулиста и лабораторию по маршрутному листу».");
            return Info(playerid, "Терапевт: «Запишитесь в регистратуре, я принимаю по маршрутному листу».");
        }
        case MEDP_EYE:
        {
            if (GetPVarInt(playerid, "medStage") != 2) return Info(playerid, "Окулист: «Вы не по записи. Сначала регистратура и терапевт».");
            SCM(playerid, C_WHITE, "Окулист Ким: «Закройте левый глаз. Какую букву я показываю указкой?»");
            return Mini_Start(playerid, "eye", 2, "med:eye", "Проверка зрения", "Назовите букву, на которую показывает указка");
        }
        case MEDP_LAB:
        {
            new st = GetPVarInt(playerid, "medStage");
            if (st == 4) return Info(playerid, "Лаборант: «Анализы ещё готовятся, подождите в коридоре».");
            if (st != 3) return Info(playerid, "Лаборант: «По маршрутному листу: терапевт, окулист, потом ко мне».");
            SCM(playerid, C_WHITE, "Лаборант Гарсия: «Работаем кулачком... Сейчас найдём вену».");
            return Mini_Start(playerid, "timing", 2, "med:lab", "Забор крови", "Нажмите ПРОБЕЛ, когда игла над веной (зелёная зона)");
        }
    }
    return 1;
}

stock Med_OnMini(playerid, const what[], bool:ok)
{
    if (!ok)
    {
        if (StrEq(what, "bp")) return SCM(playerid, C_GREY, "Терапевт: «Сбилось. Давайте ещё раз — ALT у кабинета».");
        if (StrEq(what, "eye")) return SCM(playerid, C_GREY, "Окулист: «Не торопитесь. Попробуем другую строку — ALT ещё раз».");
        return SCM(playerid, C_GREY, "Лаборант: «Ой, мимо. Сейчас ещё раз — ALT у лаборатории».");
    }
    new st = GetPVarInt(playerid, "medStage");
    if (StrEq(what, "bp") && st == 1)
    {
        SetPVarInt(playerid, "medStage", 2);
        SCM(playerid, C_WHITE, "Терапевт: «120 на 80, как у космонавта. Теперь к окулисту — кабинет 2, по соседству».");
        Med_Guide(playerid, "med_eye");
    }
    else if (StrEq(what, "eye") && st == 2)
    {
        SetPVarInt(playerid, "medStage", 3);
        SCM(playerid, C_WHITE, "Окулист: «Единица! Зрение отличное. Следующий — лаборатория».");
        Med_Guide(playerid, "med_lab");
    }
    else if (StrEq(what, "lab") && st == 3)
    {
        SetPVarInt(playerid, "medStage", 4);
        SCM(playerid, C_WHITE, "Лаборант: «Готово, держите ватку. Результаты будут через 20 секунд».");
        UI_Data(playerid, "progress", "{\"text\":\"Анализы в лаборатории...\",\"ms\":20000}");
        if (gMedTimer[playerid] != -1) KillTimer(gMedTimer[playerid]);
        gMedTimer[playerid] = SetTimerEx("Med_LabReady", 20000, false, "i", playerid);
    }
    return 1;
}

stock Med_Guide(playerid, const code[])
{
    new Float:x, Float:y, Float:z;
    if (!Loc_Get(code, x, y, z)) return;
    if (P[playerid][pCP] == CP_NONE || P[playerid][pCP] == CP_GPS) { SetPlayerCheckpoint(playerid, x, y, z, 1.2); P[playerid][pCP] = CP_GPS; }
}

forward Med_LabReady(playerid);
public Med_LabReady(playerid)
{
    gMedTimer[playerid] = -1;
    if (!IsPlayerConnected(playerid) || GetPVarInt(playerid, "medStage") != 4) return;
    SetPVarInt(playerid, "medStage", 5);
    SCM(playerid, C_YELLOW, "[Больница] Ваши анализы готовы. Вернитесь к терапевту за заключением.");
    PlayerPlaySound(playerid, 1139, 0.0, 0.0, 0.0);
    Med_Guide(playerid, "med_ther");
}

stock Hospital_Desk(playerid)
{
    new bool:poor = Med_IsPoor(playerid);
    Menu_Begin("Больница All Saints", poor ? "Регистратура · вам доступно обслуживание по полису ОМС" : "Регистратура");
    if (P[playerid][pMedcard]) Menu_Add(1, "Медкомиссия", "Медкарта уже оформлена", -1, "medic");
    else if (GetPVarInt(playerid, "medStage")) Menu_Add(1, "Маршрутный лист", "Вы уже проходите медкомиссию — показать, куда идти", -1, "medic");
    else Menu_Add(1, "Пройти медкомиссию", poor ? "Терапевт, окулист, анализы · бесплатно по ОМС" : "Терапевт, окулист, анализы -> медкарта", poor ? 0 : MED_PRICE, "medic");
    Menu_Add(2, "Лечение", poor ? "Восстановить здоровье · бесплатно по ОМС" : "Восстановить здоровье до 100", poor ? 0 : 50, "heart");
    Menu_Add(3, "Купить аптечку", "Восстанавливает 50 HP", 120, "medkit");
    Menu_Send(playerid, "hospital");
    return 1;
}

MENU:hospital(playerid, item, const extra[])
{
    UI_Close(playerid);
    new bool:poor = Med_IsPoor(playerid);
    switch (item)
    {
        case 1:
        {
            if (P[playerid][pMedcard]) return Info(playerid, "Ваша медкарта уже оформлена.");
            if (!HasPassport(playerid)) return Err(playerid, "Медсестра: «Без паспорта не оформляем. Сначала в мэрию».");
            new st = GetPVarInt(playerid, "medStage");
            if (st)
            {
                static const where[][] = {"", "к терапевту (кабинет 1)", "к окулисту (кабинет 2)", "в лабораторию (кабинет 3)", "подождать анализы в коридоре", "к терапевту за заключением"};
                new s[128]; format(s, sizeof s, "Медсестра: «По маршрутному листу вам сейчас %s».", where[st]);
                return SCM(playerid, C_WHITE, s);
            }
            if (poor) SCM(playerid, C_WHITE, "Медсестра: «Денег нет? Ничего, оформим по полису ОМС — бесплатно».");
            else if (!Pay(playerid, MED_PRICE, "medcard")) return 1;
            SetPVarInt(playerid, "medStage", 1);
            SCM(playerid, C_WHITE, "Медсестра: «Вот маршрутный лист: терапевт -> окулист -> лаборатория -> снова терапевт».");
            SendMe(playerid, "взял маршрутный лист в регистратуре");
            Med_Guide(playerid, "med_ther");
        }
        case 2:
        {
            new Float:hp; GetPlayerHealth(playerid, hp);
            if (hp >= 99.0) return Info(playerid, "Вы здоровы.");
            if (!poor && !Pay(playerid, 50, "heal")) return 1;
            SetPlayerHealth(playerid, 100.0);
            Ok(playerid, poor ? ("Врач оказал вам помощь бесплатно по полису ОМС.") : ("Врач оказал вам помощь."));
        }
        case 3:
        {
            if (!Pay(playerid, 120, "medkit")) return 1;
            Inv_Add(playerid, Item_ByKey("medkit"), 1);
        }
    }
    return 1;
}

stock Med_Done(playerid)
{
    DeletePVar(playerid, "medStage");
    P[playerid][pMedcard] = 1;
    P_SetInt(playerid, "medcard", 1);
    SCM(playerid, C_GREEN, "Терапевт: «Годен. Немного истощён — ешьте вовремя. Вот ваша медкарта».");
    SendMe(playerid, "получил медкарту и убрал её к паспорту");
    Quest_Event(playerid, "medcard", "*");
    return 1;
}

stock Hospital_Reset(playerid)
{
    if (gMedTimer[playerid] != -1) { KillTimer(gMedTimer[playerid]); gMedTimer[playerid] = -1; }
}
