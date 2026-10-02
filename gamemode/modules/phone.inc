// =====================================================================
//  Телефон (CEF): контакты, SMS, звонки, банк, такси
// =====================================================================
stock Phone_AssignNumber(playerid)
{
    if (P[playerid][pPhone][0]) return;
    new num[12], q[96];
    for (new t = 0; t < 50; t++)
    {
        format(num, sizeof num, "%d", 1000000 + random(8999999));
        format(q, sizeof q, "SELECT COUNT(*) FROM accounts WHERE phone='%s'", num);
        if (!DB_ScalarInt(q) && !StrEq(num, VOVA_NUMBER)) break;
    }
    strcpy(P[playerid][pPhone], num, 12);
}

stock FindPlayerByPhone(const num[])
{
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && StrEq(P[i][pPhone], num)) return i;
    return INVALID_PLAYER_ID;
}

stock Phone_Open(playerid)
{
    if (Inv_Count(playerid, Item_ByKey("phone")) <= 0) return Err(playerid, "У вас нет телефона. Купите его в 24/7.");
    new j[4096], t[200], q[160];
    format(j, sizeof j, "{\"number\":\"%s\",\"cash\":%d,\"bank\":%d,\"contacts\":[{\"n\":\"Дядя Вова\",\"num\":\"" VOVA_NUMBER "\"},{\"n\":\"Экстренная служба\",\"num\":\"911\"}", P[playerid][pPhone], P[playerid][pCash], P[playerid][pBank]);
    format(q, sizeof q, "SELECT name,number FROM contacts WHERE acc=%d LIMIT 40", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    if (r)
    {
        new n[32], num[12];
        for (new i = 0, c = db_num_rows(r); i < c; i++) { DB_GetStr(r, "name", n); DB_GetStr(r, "number", num); format(t, sizeof t, ",{\"n\":\"%s\",\"num\":\"%s\"}", JsonEsc(n), num); strcat(j, t); db_next_row(r); }
        db_free_result(r);
    }
    strcat(j, "],\"sms\":[");
    format(q, sizeof q, "SELECT from_no,to_no,text,date FROM sms WHERE to_no='%s' OR from_no='%s' ORDER BY id DESC LIMIT 15", P[playerid][pPhone], P[playerid][pPhone]);
    r = DB_Query(q);
    if (r)
    {
        new f[12], to[12], tx[128], d[24];
        for (new i = 0, c = db_num_rows(r); i < c; i++)
        {
            DB_GetStr(r, "from_no", f); DB_GetStr(r, "to_no", to); DB_GetStr(r, "text", tx); DB_GetStr(r, "date", d);
            format(t, sizeof t, "%s{\"f\":\"%s\",\"to\":\"%s\",\"t\":\"%s\",\"d\":\"%s\"}", i ? "," : "", f, to, JsonEsc(tx), d);
            if (strlen(j) + strlen(t) < sizeof(j) - 8) strcat(j, t);
            db_next_row(r);
        }
        db_free_result(r);
    }
    strcat(j, "]}");
    UI_Open(playerid, "phone", j);
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_USECELLPHONE);
    return 1;
}

CMD:phone(playerid, const params[]) return Phone_Open(playerid);
CMD:number(playerid, const params[])
{
    new s[64]; format(s, sizeof s, "Ваш номер: %s", P[playerid][pPhone]);
    return Info(playerid, s);
}

UI:phone(playerid, const a[])
{
    new act[12], num[16];
    CefField(a, 1, act);
    if (StrEq(act, "open")) return Phone_Open(playerid);
    if (Inv_Count(playerid, Item_ByKey("phone")) <= 0) return 1;
    if (StrEq(act, "close"))
    {
        if (!P[playerid][pInCall]) SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
        return 1;
    }
    CefField(a, 2, num);
    if (StrEq(act, "call"))
    {
        if (P[playerid][pInCall] || P[playerid][pCallTo] != INVALID_PLAYER_ID) return Err(playerid, "Вы уже разговариваете.");
        if (StrEq(num, VOVA_NUMBER)) return Vova_Call(playerid);
        if (StrEq(num, "911")) return cmd_call911(playerid, "\1");
        new t = FindPlayerByPhone(num);
        if (t == INVALID_PLAYER_ID || t == playerid) return UI_Data(playerid, "call_state", "{\"state\":\"unavailable\"}");
        if (Inv_Count(t, Item_ByKey("phone")) <= 0 || P[t][pInCall] || P[t][pCallTo] != INVALID_PLAYER_ID) return UI_Data(playerid, "call_state", "{\"state\":\"busy\"}");
        P[playerid][pCallTo] = t; P[t][pCallTo] = playerid;
        new j[96]; format(j, sizeof j, "{\"state\":\"incoming\",\"num\":\"%s\"}", P[playerid][pPhone]);
        UI_Data(t, "call_state", j);
        SCM(t, C_YELLOW, "Вам звонят. Ответить: /pickup, сбросить: /hangup (или в телефоне).");
        UI_Data(playerid, "call_state", "{\"state\":\"ringing\"}");
        SendMe(t, "достаёт звонящий телефон");
    }
    else if (StrEq(act, "answer")) cmd_pickup(playerid, "\1");
    else if (StrEq(act, "hang")) cmd_hangup(playerid, "\1");
    else if (StrEq(act, "sms"))
    {
        new text[128], q[300];
        CefField(a, 3, text);
        if (!text[0] || strlen(num) < 3) return 1;
        if (!Pay(playerid, 2, "sms")) return 1;
        format(q, sizeof q, "INSERT INTO sms (from_no,to_no,text,date) VALUES ('%s','%s','%s','%s')", P[playerid][pPhone], SqlEsc(num), SqlEsc(text), DateStr());
        DB_Exec(q);
        new t = FindPlayerByPhone(num);
        if (t != INVALID_PLAYER_ID)
        {
            new s[144]; format(s, sizeof s, "SMS от %s: %s", P[playerid][pPhone], text);
            SCM(t, C_YELLOW, s);
            PlayerPlaySound(t, 1052, 0.0, 0.0, 0.0);
        }
        Ok(playerid, "Сообщение отправлено.");
    }
    else if (StrEq(act, "addc"))
    {
        new name[32], q[200];
        CefField(a, 3, name);
        if (!name[0] || !num[0]) return 1;
        format(q, sizeof q, "INSERT INTO contacts VALUES (%d,'%s','%s')", P[playerid][pID], SqlEsc(name), SqlEsc(num)); DB_Exec(q);
        Phone_Open(playerid);
    }
    else if (StrEq(act, "delc"))
    {
        new q[160]; format(q, sizeof q, "DELETE FROM contacts WHERE acc=%d AND number='%s'", P[playerid][pID], SqlEsc(num)); DB_Exec(q);
        Phone_Open(playerid);
    }
    else if (StrEq(act, "taxi")) cmd_taxi(playerid, "\1");
    else if (StrEq(act, "transfer"))
    {
        // перевод по номеру телефона
        new amount = CefFieldInt(a, 3);
        new t = FindPlayerByPhone(num);
        if (t == INVALID_PLAYER_ID) return Err(playerid, "Абонент не в сети.");
        new args[64]; format(args, sizeof args, "bank|tr|%d|%s", amount, PName(t));
        ui_bank(playerid, args);
        Phone_Open(playerid);
    }
    return 1;
}

CMD:pickup(playerid, const params[])
{
    new t = P[playerid][pCallTo];
    if (t == INVALID_PLAYER_ID || P[playerid][pInCall]) return Err(playerid, "Вам никто не звонит.");
    P[playerid][pInCall] = true; P[t][pInCall] = true;
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_USECELLPHONE);
    UI_Data(playerid, "call_state", "{\"state\":\"talking\"}");
    UI_Data(t, "call_state", "{\"state\":\"talking\"}");
    SCM(playerid, C_YELLOW, "Разговор начат. Пишите в чат — вас слышит собеседник. /hangup — положить трубку.");
    SCM(t, C_YELLOW, "Абонент ответил. Пишите в чат. /hangup — положить трубку.");
    return 1;
}

CMD:hangup(playerid, const params[])
{
    new t = P[playerid][pCallTo];
    P[playerid][pInCall] = false; P[playerid][pCallTo] = INVALID_PLAYER_ID;
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_STOPUSECELLPHONE);
    UI_Data(playerid, "call_state", "{\"state\":\"idle\"}");
    if (t != INVALID_PLAYER_ID && IsPlayerConnected(t))
    {
        P[t][pInCall] = false; P[t][pCallTo] = INVALID_PLAYER_ID;
        SetPlayerSpecialAction(t, SPECIAL_ACTION_STOPUSECELLPHONE);
        UI_Data(t, "call_state", "{\"state\":\"idle\"}");
        SCM(t, C_GREY, "Собеседник положил трубку.");
    }
    return 1;
}

stock Phone_OnText(playerid, const text[])
{
    new t = P[playerid][pCallTo];
    if (!P[playerid][pInCall] || t == INVALID_PLAYER_ID) return 0;
    new s[144]; format(s, sizeof s, "[Телефон] %s: %s", PName(playerid), text);
    SCM(t, C_YELLOW, s); SCM(playerid, C_YELLOW, s);
    format(s, sizeof s, "%s говорит по телефону: %s", PName(playerid), text);
    ProxMsg(playerid, 10.0, C_GREY, s);
    return 1;
}

stock Phone_OnDisconnect(playerid)
{
    if (P[playerid][pCallTo] != INVALID_PLAYER_ID) cmd_hangup(playerid, "\1");
}
