// =====================================================================
//  Документы: очередь в мэрии, анкета на паспорт, медкарта, лицензии
// =====================================================================
new gQueueCounter = 100;

stock bool:HasPassport(playerid)
{
    new q[80]; format(q, sizeof q, "SELECT COUNT(*) FROM passports WHERE acc=%d", P[playerid][pID]);
    return DB_ScalarInt(q) > 0;
}

// =====================================================================
//  Живая очередь в мэрии: NPC-посетители берут талоны, стоят в линию,
//  по одному подходят к окну, их обслуживают, они уходят к выходу —
//  очередь двигается. Игроки встают в ту же очередь.
//  Паспорт: анкета -> фото -> отпечатки пальцев -> печать (30 с) -> подпись.
// =====================================================================
#define CH_VW 1
#define LINE_MAX 12
#define QA_MAX 10
static const gQueueSkins[] = {9,10,11,12,14,15,17,20,24,25,31,37,40,44,47,55,57,58,59,60,69,72,76,88,98,141,150,170,184,187,190,193,211,216,223,240,250};
static const gClerkLines[][] = {"Следующий!", "Талон, пожалуйста.", "Распишитесь вот здесь.", "Фото есть? Сейчас сделаем.", "Ожидайте, документ готовится.", "Всего доброго!"};
enum E_QA { bool:qaUsed, qaActor, Float:qaX, Float:qaY, Float:qaTX, Float:qaTY, qaState, qaTicket } // state: 0 стоит, 1 идёт, 2 у окна, 3 уходит
new gQA[QA_MAX][E_QA];
new gLineType[LINE_MAX], gLineId[LINE_MAX], gLineTicket[LINE_MAX], gLineLen;
new gWinState, gWinWho, gWinUntil; // 0 свободно, 1 NPC у окна, 2 вызван игрок
new gClerk = INVALID_ACTOR_ID, gNextArrival;

stock CityHall_Init()
{
    gClerk = CreateActor(150, 358.3, 173.6, 1008.38, 270.0);
    if (gClerk != INVALID_ACTOR_ID) { SetActorVirtualWorld(gClerk, CH_VW); SetActorInvulnerable(gClerk, true); }
    Create3DTextLabel(H_GOLD "Паспортный стол · окно №1\n" H_GREY "Табло: ожидайте вызова", -1, 359.0, 173.6, 1009.9, 10.0, CH_VW, 1);
    for (new i = 0; i < 3; i++) QA_Arrive(true);
    SetTimer("CH_Move", 150, true);
}

stock Float:Line_SlotX(slot) return 361.2 + float(slot) * 1.05;

stock QA_Arrive(bool:instant = false)
{
    if (gLineLen >= LINE_MAX - 1) return;
    new k = -1;
    for (new i = 0; i < QA_MAX; i++) if (!gQA[i][qaUsed]) { k = i; break; }
    if (k == -1) return;
    new slot = Line_NpcCount();
    new Float:sx = instant ? Line_SlotX(slot) : 388.8, Float:sy = 173.7;
    new a = CreateActor(gQueueSkins[random(sizeof gQueueSkins)], sx, sy, 1008.38, 90.0);
    if (a == INVALID_ACTOR_ID) return;
    SetActorVirtualWorld(a, CH_VW); SetActorInvulnerable(a, true);
    gQA[k][qaUsed] = true; gQA[k][qaActor] = a; gQA[k][qaX] = sx; gQA[k][qaY] = sy; gQA[k][qaState] = 0;
    gQA[k][qaTicket] = ++gQueueCounter;
    Line_Push(0, k, gQA[k][qaTicket]);
    QA_Goto(k, Line_SlotX(slot), 173.7);
}

stock Line_NpcCount()
{
    new c = 0;
    for (new i = 0; i < gLineLen; i++) if (gLineType[i] == 0) c++;
    return c;
}

stock Line_Push(type, id, ticket)
{
    if (gLineLen >= LINE_MAX) return;
    gLineType[gLineLen] = type; gLineId[gLineLen] = id; gLineTicket[gLineLen] = ticket; gLineLen++;
}

stock Line_RemoveAt(i)
{
    for (new j = i; j < gLineLen - 1; j++) { gLineType[j] = gLineType[j + 1]; gLineId[j] = gLineId[j + 1]; gLineTicket[j] = gLineTicket[j + 1]; }
    if (gLineLen > 0) gLineLen--;
}

stock Line_FindPlayer(playerid)
{
    for (new i = 0; i < gLineLen; i++) if (gLineType[i] == 1 && gLineId[i] == playerid) return i;
    return -1;
}

stock QA_Goto(k, Float:x, Float:y)
{
    gQA[k][qaTX] = x; gQA[k][qaTY] = y;
    if (floatabs(gQA[k][qaX] - x) + floatabs(gQA[k][qaY] - y) < 0.05) return;
    new st = gQA[k][qaState];
    gQA[k][qaState] = (st == 3 || st == 4) ? st : 1;
    new Float:ang = -atan2(x - gQA[k][qaX], y - gQA[k][qaY]); if (ang < 0.0) ang += 360.0;
    SetActorFacingAngle(gQA[k][qaActor], ang);
    ApplyActorAnimation(gQA[k][qaActor], "PED", "WALK_civi", 4.1, 1, 1, 1, 0, 0);
}

// плавное движение NPC очереди (150 мс)
forward CH_Move();
public CH_Move()
{
    for (new k = 0; k < QA_MAX; k++)
    {
        if (!gQA[k][qaUsed] || (gQA[k][qaState] != 1 && gQA[k][qaState] != 3 && gQA[k][qaState] != 4)) continue;
        new Float:dx = gQA[k][qaTX] - gQA[k][qaX], Float:dy = gQA[k][qaTY] - gQA[k][qaY];
        new Float:d = floatsqroot(dx * dx + dy * dy);
        if (d <= 0.21)
        {
            gQA[k][qaX] = gQA[k][qaTX]; gQA[k][qaY] = gQA[k][qaTY];
            SetActorPos(gQA[k][qaActor], gQA[k][qaX], gQA[k][qaY], 1008.38);
            if (gQA[k][qaState] == 3) { QA_Leave(k); continue; }
            if (gQA[k][qaState] == 4) { gQA[k][qaState] = 3; QA_Goto(k, 388.8, 171.8); continue; }
            gQA[k][qaState] = 0;
            SetActorFacingAngle(gQA[k][qaActor], 90.0);
            ClearActorAnimations(gQA[k][qaActor]);
            if (random(3) == 0) ApplyActorAnimation(gQA[k][qaActor], "COP_AMBIENT", "Coplook_loop", 4.1, 1, 0, 0, 0, 0);
            continue;
        }
        gQA[k][qaX] += dx / d * 0.2; gQA[k][qaY] += dy / d * 0.2;
        SetActorPos(gQA[k][qaActor], gQA[k][qaX], gQA[k][qaY], 1008.38);
    }
}

stock QA_Leave(k)
{
    if (IsValidActor(gQA[k][qaActor])) DestroyActor(gQA[k][qaActor]);
    gQA[k][qaUsed] = false;
}

// NPC, стоящие в линии, подтягиваются к своим местам
stock Line_Relayout()
{
    new slot = 0;
    for (new i = 0; i < gLineLen; i++)
    {
        if (gLineType[i] != 0) continue;
        new k = gLineId[i];
        if (gQA[k][qaUsed] && gQA[k][qaState] != 3 && gQA[k][qaState] != 2) QA_Goto(k, Line_SlotX(slot), 173.7);
        slot++;
    }
}

stock Board_Announce(ticket)
{
    new s[96]; format(s, sizeof s, "{FFC94D}[Табло]{FFFFFF} Талон А-%d — окно №1 (паспортный стол).", ticket);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsLogged(i) && GetPlayerInterior(i) == 3 && GetPlayerVirtualWorld(i) == CH_VW)
        {
            SCM(i, C_WHITE, s); PlayerPlaySound(i, 1139, 0.0, 0.0, 0.0);
            if (IsPlayerInRangeOfPoint(i, 12.0, 359.7, 173.6, 1008.38)) { new c[96]; format(c, sizeof c, "Сотрудница паспортного стола: «%s»", gClerkLines[random(sizeof gClerkLines)]); SCM(i, 0xE6E6E6FF, c); }
        }
}

stock Queue_Notify()
{
    for (new i = 0; i < gLineLen; i++)
    {
        if (gLineType[i] != 1) continue;
        new j[64]; format(j, sizeof j, "{\"no\":%d,\"ahead\":%d}", gLineTicket[i], i + (gWinState ? 1 : 0));
        UI_Data(gLineId[i], "queue", j);
    }
}

// раз в секунду: окно обслуживает следующего
stock CityHall_Tick()
{
    new now = gettime();
    if (gWinState == 1 && now >= gWinUntil)
    {   // NPC обслужен — уходит к выходу
        new k = gWinWho;
        if (gQA[k][qaUsed]) { if (gClerk != INVALID_ACTOR_ID) ApplyActorAnimation(gClerk, "PED", "IDLE_chat", 4.1, 0, 1, 1, 0, 0); gQA[k][qaState] = 4; QA_Goto(k, 360.6, 171.8); }
        gWinState = 0;
    }
    if (gWinState == 2 && now >= gWinUntil)
    {
        new pl = gWinWho;
        if (IsPlayerConnected(pl) && GetPVarInt(pl, "passStage") > 0) gWinUntil = now + 60; // игрок уже оформляется
        else
        {
            if (IsPlayerConnected(pl) && P[pl][pQueueNo]) { SCM(pl, C_GREY, "Вы не подошли к окну — талон аннулирован. Возьмите новый в терминале."); P[pl][pQueueNo] = 0; P[pl][pQueueCalled] = false; UI_Data(pl, "queue", "{\"done\":1}"); }
            gWinState = 0;
        }
    }
    if (gWinState == 0 && gLineLen > 0)
    {
        new type = gLineType[0], id = gLineId[0], ticket = gLineTicket[0];
        Line_RemoveAt(0);
        Board_Announce(ticket);
        if (type == 0)
        {
            if (gQA[id][qaUsed])
            {
                gWinState = 1; gWinWho = id; gWinUntil = now + 7 + random(6);
                QA_Goto(id, 359.9, 173.6); gQA[id][qaState] = 1;
                SetTimerEx("QA_AtWindow", 2500, false, "i", id);
            }
        }
        else if (IsPlayerConnected(id) && P[id][pQueueNo])
        {
            gWinState = 2; gWinWho = id; gWinUntil = now + 90;
            Queue_Call(id);
        }
        Line_Relayout();
        Queue_Notify();
    }
    if (now >= gNextArrival)
    {
        gNextArrival = now + 18 + random(25);
        if (Line_NpcCount() < 5) QA_Arrive();
    }
}

forward QA_AtWindow(k);
public QA_AtWindow(k)
{
    if (!gQA[k][qaUsed]) return;
    gQA[k][qaState] = 2;
    SetActorFacingAngle(gQA[k][qaActor], 90.0);
    ApplyActorAnimation(gQA[k][qaActor], "INT_SHOP", "shop_pay", 4.1, 1, 0, 0, 0, 0);
    if (gClerk != INVALID_ACTOR_ID) ApplyActorAnimation(gClerk, "INT_SHOP", "shop_cashier", 4.1, 1, 0, 0, 0, 0);
}

stock Queue_Take(playerid)
{
    if (HasPassport(playerid)) return Info(playerid, "У вас уже есть паспорт. Окно паспортного стола вам не нужно.");
    if (P[playerid][pQueueNo])
    {
        new i = Line_FindPlayer(playerid);
        new s[96]; format(s, sizeof s, "Ваш талон: А-%d. Перед вами: %d чел.", P[playerid][pQueueNo], i == -1 ? 0 : i + (gWinState ? 1 : 0));
        return Info(playerid, s);
    }
    P[playerid][pQueueNo] = ++gQueueCounter;
    P[playerid][pQueueCalled] = false;
    Line_Push(1, playerid, P[playerid][pQueueNo]);
    new ahead = gLineLen - 1 + (gWinState ? 1 : 0);
    new s[144];
    format(s, sizeof s, "Терминал выдал талон А-%d. Перед вами в очереди: %d чел. Следите за табло — вас вызовут к окну №1.", P[playerid][pQueueNo], ahead);
    SCM(playerid, C_WHITE, s);
    ApplyAnimation(playerid, "INT_SHOP", "shop_pay", 4.0, 0, 0, 0, 0, 0);
    new j[64]; format(j, sizeof j, "{\"no\":%d,\"ahead\":%d}", P[playerid][pQueueNo], ahead);
    UI_Data(playerid, "queue", j);
    return 1;
}

stock Queue_Leave(playerid)
{
    new i = Line_FindPlayer(playerid);
    if (i != -1) Line_RemoveAt(i);
    if (gWinState == 2 && gWinWho == playerid) gWinState = 0;
    P[playerid][pQueueNo] = 0; P[playerid][pQueueCalled] = false;
    DeletePVar(playerid, "passStage");
}

forward Queue_Call(playerid);
public Queue_Call(playerid)
{
    if (!IsPlayerConnected(playerid) || !P[playerid][pQueueNo]) return;
    if (!IsPlayerAtLoc(playerid, "cityhall_in", 45.0))
    {
        SCM(playerid, C_GREY, "Вы ушли из мэрии — ваш талон аннулирован.");
        Queue_Leave(playerid);
        return;
    }
    P[playerid][pQueueCalled] = true;
    UI_Data(playerid, "queue_call", "{\"window\":1}");
    new cs[112]; format(cs, sizeof cs, "Сотрудница паспортного стола: «Талон А-%d, проходите к окну №1!»", P[playerid][pQueueNo]); SCM(playerid, C_YELLOW, cs);
    Quest_Event(playerid, "queue_done", "*");
}

stock Window_Use(playerid)
{
    if (HasPassport(playerid)) return Info(playerid, "Ваш паспорт уже оформлен.");
    if (!P[playerid][pQueueNo]) return Err(playerid, "Сначала возьмите талон в терминале электронной очереди.");
    if (!P[playerid][pQueueCalled]) return Err(playerid, "Ваш номер ещё не вызвали. Следите за табло.");
    switch (GetPVarInt(playerid, "passStage"))
    {
        case 0:
        {
            new j[200];
            format(j, sizeof j, "{\"name\":\"%s\",\"age\":%d,\"sex\":%d,\"ticket\":%d}", PName(playerid), P[playerid][pAge], P[playerid][pSex], P[playerid][pQueueNo]);
            UI_Open(playerid, "passport_form", j);
        }
        case 1:
        {
            SCM(playerid, C_WHITE, "Сотрудница: «Смотрим в камеру, не моргаем... Поймайте момент, когда вы в рамке».");
            Mini_Start(playerid, "photo", 2, "pass:photo", "Фото на паспорт", "Нажмите, когда лицо точно в рамке");
        }
        case 2:
        {
            SCM(playerid, C_WHITE, "Сотрудница: «Приложите палец к сканеру и держите, пока полоска не станет зелёной».");
            Mini_Start(playerid, "hold", 2, "pass:finger", "Отпечатки пальцев", "Удерживайте кнопку мыши, отпустите в зелёной зоне");
        }
        case 3: Info(playerid, "Сотрудница: «Паспорт ещё печатается, подождите немного».");
        case 4:
        {
            SCM(playerid, C_WHITE, "Сотрудница: «Готово! Проверьте данные и распишитесь в получении».");
            Mini_Start(playerid, "sign", 1, "pass:sign", "Подпись в журнале", "Распишитесь мышью в поле (зажмите и ведите)");
        }
    }
    return 1;
}

UI:passport(playerid, const a[])
{
    new act[8];
    CefField(a, 1, act);
    if (!StrEq(act, "submit")) return 1;
    if (HasPassport(playerid) || !P[playerid][pQueueCalled] || GetPVarInt(playerid, "passStage") != 0) return 1;
    if (!IsPlayerAtLoc(playerid, "cityhall_window", 5.0)) return Err(playerid, "Вы отошли от окна.");
    new city[32], marital[24], sign[4];
    CefField(a, 2, city); CefField(a, 3, marital); CefField(a, 4, sign);
    if (!city[0] || !marital[0]) return UI_Data(playerid, "form_error", "{\"code\":\"fields\"}");
    if (!StrEq(sign, "1")) return UI_Data(playerid, "form_error", "{\"code\":\"sign\"}");
    SetPVarString(playerid, "passCity", city);
    SetPVarString(playerid, "passMar", marital);
    SetPVarInt(playerid, "passStage", 1);
    UI_Close(playerid, "passport_form");
    SCM(playerid, C_WHITE, "Сотрудница: «Анкета принята. Теперь сфотографируемся — нажмите ALT у окна».");
    SendMe(playerid, "передал анкету в окно паспортного стола");
    return 1;
}

stock Pass_OnMini(playerid, const what[], bool:ok)
{
    new st = GetPVarInt(playerid, "passStage");
    if (!ok)
    {
        if (StrEq(what, "photo")) return SCM(playerid, C_GREY, "Сотрудница: «Вы моргнули. Ещё раз — ALT у окна».");
        if (StrEq(what, "finger")) return SCM(playerid, C_GREY, "Сотрудница: «Отпечаток смазался. Попробуем снова — ALT».");
        return SCM(playerid, C_GREY, "Сотрудница: «Подпись должна быть разборчивой. Ещё раз — ALT».");
    }
    if (StrEq(what, "photo") && st == 1)
    {
        SetPVarInt(playerid, "passStage", 2);
        SCM(playerid, C_WHITE, "Сотрудница: «Отличное фото! Теперь отпечатки пальцев — ALT у окна».");
    }
    else if (StrEq(what, "finger") && st == 2)
    {
        SetPVarInt(playerid, "passStage", 3);
        SCM(playerid, C_WHITE, "Сотрудница: «Всё, документы ушли в печать. Около 30 секунд — можете присесть».");
        UI_Data(playerid, "progress", "{\"text\":\"Паспорт печатается...\",\"ms\":30000}");
        P[playerid][pQueueTimer] = SetTimerEx("Pass_Printed", 30000, false, "i", playerid);
    }
    else if (StrEq(what, "sign") && st == 4) Pass_Issue(playerid);
    return 1;
}

forward Pass_Printed(playerid);
public Pass_Printed(playerid)
{
    P[playerid][pQueueTimer] = -1;
    if (!IsPlayerConnected(playerid) || GetPVarInt(playerid, "passStage") != 3) return;
    SetPVarInt(playerid, "passStage", 4);
    SCM(playerid, C_YELLOW, "Сотрудница паспортного стола: «Паспорт готов! Подойдите к окну, нужна ваша подпись».");
    PlayerPlaySound(playerid, 1139, 0.0, 0.0, 0.0);
}

stock Pass_Issue(playerid)
{
    new city[32], marital[24], number[16], q[300];
    GetPVarString(playerid, "passCity", city, sizeof city);
    GetPVarString(playerid, "passMar", marital, sizeof marital);
    format(number, sizeof number, "%02d %02d %06d", 10 + random(89), 10 + random(89), 100000 + P[playerid][pID] * 13 % 899999);
    format(q, sizeof q, "INSERT OR REPLACE INTO passports VALUES (%d,'%s','%s','%s','%s')", P[playerid][pID], number, SqlEsc(city), SqlEsc(marital), DateStr());
    DB_Exec(q);
    DeletePVar(playerid, "passStage"); DeletePVar(playerid, "passCity"); DeletePVar(playerid, "passMar");
    P[playerid][pQueueNo] = 0;
    P[playerid][pQueueCalled] = false;
    if (gWinState == 2 && gWinWho == playerid) gWinState = 0;
    UI_Data(playerid, "passport_done", "{}");
    new s[128]; format(s, sizeof s, "Сотрудница паспортного стола выдала вам паспорт № %s.", number);
    SCM(playerid, C_GREEN, s);
    SendMe(playerid, "получил паспорт и убрал его во внутренний карман");
    Ach_Give(playerid, "passport", "Гражданин");
    Quest_Event(playerid, "passport", "*");
}

stock CityHall_OnDisconnect(playerid)
{
    if (P[playerid][pQueueNo]) Queue_Leave(playerid);
}

// больница и медкомиссия — hospital.inc

// ---------- показ документов ----------
stock Docs_Show(playerid, target, const type[])
{
    new j[600], q[128];
    if (StrEq(type, "passport"))
    {
        format(q, sizeof q, "SELECT * FROM passports WHERE acc=%d", P[playerid][pID]);
        new DBResult:r = DB_Query(q);
        if (!r || !db_num_rows(r)) { if (r) db_free_result(r); return Err(playerid, "У вас нет паспорта."); }
        new number[16], city[32], marital[24], issued[24];
        DB_GetStr(r, "number", number); DB_GetStr(r, "birth_city", city); DB_GetStr(r, "marital", marital); DB_GetStr(r, "issued", issued);
        db_free_result(r);
        format(j, sizeof j, "{\"type\":\"passport\",\"name\":\"%s\",\"age\":%d,\"sex\":%d,\"number\":\"%s\",\"city\":\"%s\",\"marital\":\"%s\",\"issued\":\"%s\",\"skin\":%d,\"org\":\"%s\",\"lvl\":%d}",
            PName(playerid), P[playerid][pAge], P[playerid][pSex], number, JsonEsc(city), JsonEsc(marital), issued, P[playerid][pSkin], P[playerid][pOrg] ? Org_Name(P[playerid][pOrg]) : ("—"), P[playerid][pLevel]);
    }
    else if (StrEq(type, "med"))
    {
        if (!P[playerid][pMedcard]) return Err(playerid, "У вас нет медкарты.");
        format(j, sizeof j, "{\"type\":\"med\",\"name\":\"%s\",\"age\":%d,\"status\":\"Здоров\"}", PName(playerid), P[playerid][pAge]);
    }
    else
    {
        format(j, sizeof j, "{\"type\":\"lic\",\"name\":\"%s\",\"drive\":%d,\"theory\":%d}", PName(playerid), P[playerid][pLicDrive], P[playerid][pTheory]);
    }
    UI_Open(target, "document", j);
    if (target != playerid)
    {
        new s[96]; format(s, sizeof s, "показал документы %s", PName(target)); SendMe(playerid, s);
    }
    return 1;
}

CMD:pass(playerid, const params[])
{
    new t = isnull(params) ? playerid : GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || !P[t][pLogged]) return Err(playerid, "Игрок не найден. /pass [id]");
    if (t != playerid && GetNearestPlayer(playerid, 4.0) != t) return Err(playerid, "Игрок слишком далеко.");
    return Docs_Show(playerid, t, "passport");
}
CMD:med(playerid, const params[])
{
    new t = isnull(params) ? playerid : GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || !P[t][pLogged]) return Err(playerid, "/med [id]");
    if (t != playerid && GetNearestPlayer(playerid, 4.0) != t) return Err(playerid, "Игрок слишком далеко.");
    return Docs_Show(playerid, t, "med");
}
CMD:lic(playerid, const params[])
{
    new t = isnull(params) ? playerid : GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || !P[t][pLogged]) return Err(playerid, "/lic [id]");
    if (t != playerid && GetNearestPlayer(playerid, 4.0) != t) return Err(playerid, "Игрок слишком далеко.");
    return Docs_Show(playerid, t, "lic");
}
