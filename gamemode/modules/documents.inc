// =====================================================================
//  Документы: очередь в мэрии, анкета на паспорт, медкарта, лицензии
// =====================================================================
new gQueueCounter = 100;
new gQueueServing = 100;

stock bool:HasPassport(playerid)
{
    new q[80]; format(q, sizeof q, "SELECT COUNT(*) FROM passports WHERE acc=%d", P[playerid][pID]);
    return DB_ScalarInt(q) > 0;
}

stock Queue_Take(playerid)
{
    if (HasPassport(playerid)) return Info(playerid, "У вас уже есть паспорт. Окно паспортного стола вам не нужно.");
    if (P[playerid][pQueueNo]) 
    {
        new s[96]; format(s, sizeof s, "Ваш талон: А-%d. Ожидайте вызова.", P[playerid][pQueueNo]);
        return Info(playerid, s);
    }
    gQueueCounter++;
    P[playerid][pQueueNo] = gQueueCounter;
    P[playerid][pQueueCalled] = false;
    new ahead = 1 + random(3);
    new s[128];
    format(s, sizeof s, "Терминал выдал талон А-%d. Перед вами в очереди: %d чел. Ожидайте вызова.", gQueueCounter, ahead);
    SCM(playerid, C_WHITE, s);
    ApplyAnimation(playerid, "INT_SHOP", "shop_pay", 4.0, 0, 0, 0, 0, 0);
    P[playerid][pQueueTimer] = SetTimerEx("Queue_Call", (15 + ahead * 10) * 1000, false, "i", playerid);
    new j[64]; format(j, sizeof j, "{\"no\":%d,\"ahead\":%d}", gQueueCounter, ahead);
    UI_Data(playerid, "queue", j);
    return 1;
}

forward Queue_Call(playerid);
public Queue_Call(playerid)
{
    P[playerid][pQueueTimer] = -1;
    if (!IsPlayerConnected(playerid) || !P[playerid][pQueueNo]) return;
    if (!IsPlayerAtLoc(playerid, "cityhall_in", 40.0))
    {
        SCM(playerid, C_GREY, "Вы ушли из мэрии — ваш талон аннулирован.");
        P[playerid][pQueueNo] = 0;
        return;
    }
    gQueueServing = P[playerid][pQueueNo];
    P[playerid][pQueueCalled] = true;
    new s[96]; format(s, sizeof s, "{FFC94D}[Табло]{FFFFFF} Талон А-%d — окно №1 (паспортный стол).", gQueueServing);
    SCM(playerid, C_WHITE, s);
    PlayerPlaySound(playerid, 1139, 0.0, 0.0, 0.0);
    UI_Data(playerid, "queue_call", "{\"window\":1}");
    Quest_Event(playerid, "queue_done", "*");
}

stock Window_Use(playerid)
{
    if (HasPassport(playerid)) return Info(playerid, "Ваш паспорт уже оформлен.");
    if (!P[playerid][pQueueNo]) return Err(playerid, "Сначала возьмите талон в терминале электронной очереди.");
    if (!P[playerid][pQueueCalled]) return Err(playerid, "Ваш номер ещё не вызвали. Подождите.");
    new j[200];
    format(j, sizeof j, "{\"name\":\"%s\",\"age\":%d,\"sex\":%d,\"ticket\":%d}", PName(playerid), P[playerid][pAge], P[playerid][pSex], P[playerid][pQueueNo]);
    UI_Open(playerid, "passport_form", j);
    return 1;
}

UI:passport(playerid, const a[])
{
    new act[8];
    CefField(a, 1, act);
    if (!StrEq(act, "submit")) return 1;
    if (HasPassport(playerid) || !P[playerid][pQueueCalled]) return 1;
    if (!IsPlayerAtLoc(playerid, "cityhall_window", 5.0)) return Err(playerid, "Вы отошли от окна.");
    new city[32], marital[24], sign[4];
    CefField(a, 2, city); CefField(a, 3, marital); CefField(a, 4, sign);
    if (!city[0] || !marital[0]) return UI_Data(playerid, "form_error", "{\"code\":\"fields\"}");
    if (!StrEq(sign, "1")) return UI_Data(playerid, "form_error", "{\"code\":\"sign\"}");
    new number[16], q[300];
    format(number, sizeof number, "%02d %02d %06d", 10 + random(89), 10 + random(89), 100000 + P[playerid][pID] * 13 % 899999);
    format(q, sizeof q, "INSERT OR REPLACE INTO passports VALUES (%d,'%s','%s','%s','%s')", P[playerid][pID], number, SqlEsc(city), SqlEsc(marital), DateStr());
    DB_Exec(q);
    P[playerid][pQueueNo] = 0;
    P[playerid][pQueueCalled] = false;
    UI_Close(playerid, "passport_form");
    new s[128]; format(s, sizeof s, "Сотрудник паспортного стола выдал вам паспорт № %s.", number);
    SCM(playerid, C_GREEN, s);
    SendMe(playerid, "получил паспорт и убрал его во внутренний карман");
    Ach_Give(playerid, "passport", "Гражданин");
    Quest_Event(playerid, "passport", "*");
    return 1;
}

// ---------- больница: медкомиссия ----------
new gMedTimer[MAX_PLAYERS] = {-1, ...};

stock Hospital_Desk(playerid)
{
    Menu_Begin("Больница All Saints", "Регистратура");
    Menu_Add(1, "Пройти медкомиссию", P[playerid][pMedcard] ? "Медкарта уже есть" : "Осмотр терапевта, анализы, медкарта", 100);
    Menu_Add(2, "Лечение", "Восстановить здоровье до 100", 50);
    Menu_Add(3, "Купить аптечку", "Восстанавливает 50 HP", 120);
    Menu_Send(playerid, "hospital");
    return 1;
}

MENU:hospital(playerid, item, const extra[])
{
    UI_Close(playerid);
    switch (item)
    {
        case 1:
        {
            if (P[playerid][pMedcard]) return Info(playerid, "Ваша медкарта уже оформлена.");
            if (!HasPassport(playerid)) return Err(playerid, "Для медкомиссии нужен паспорт.");
            if (gMedTimer[playerid] != -1) return 1;
            if (!Pay(playerid, 100, "medcard")) return 1;
            TogglePlayerControllable(playerid, false);
            ApplyAnimation(playerid, "MEDIC", "CPR", 4.0, 1, 0, 0, 0, 0);
            UI_Data(playerid, "progress", "{\"text\":\"Проходит медосмотр...\",\"ms\":12000}");
            SendMe(playerid, "проходит медицинский осмотр");
            gMedTimer[playerid] = SetTimerEx("Med_Done", 12000, false, "i", playerid);
        }
        case 2:
        {
            new Float:hp; GetPlayerHealth(playerid, hp);
            if (hp >= 99.0) return Info(playerid, "Вы здоровы.");
            if (!Pay(playerid, 50, "heal")) return 1;
            SetPlayerHealth(playerid, 100.0);
            Ok(playerid, "Врач оказал вам помощь.");
        }
        case 3:
        {
            if (!Pay(playerid, 120, "medkit")) return 1;
            Inv_Add(playerid, Item_ByKey("medkit"), 1);
        }
    }
    return 1;
}

forward Med_Done(playerid);
public Med_Done(playerid)
{
    gMedTimer[playerid] = -1;
    if (!IsPlayerConnected(playerid)) return;
    TogglePlayerControllable(playerid, true);
    ClearAnimations(playerid);
    P[playerid][pMedcard] = 1;
    P_SetInt(playerid, "medcard", 1);
    SCM(playerid, C_GREEN, "Терапевт: «Здоров. Немного истощён, но жить будешь». Медкарта получена.");
    Quest_Event(playerid, "medcard", "*");
}

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
