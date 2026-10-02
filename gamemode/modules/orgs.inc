// =====================================================================
//  Организации: LSPD, больница, правительство, армия, СМИ, 2 банды, мафия
//  Ранги, зарплата, склад, рация, меню лидера (CEF)
// =====================================================================
#define MAX_ORGS 9
#define MAX_RANKS 7
enum { ORG_LSPD = 1, ORG_EMS, ORG_GOV, ORG_ARMY, ORG_NEWS, ORG_GROVE, ORG_BALLAS, ORG_MAFIA }
enum E_ORG { oName[48], oType, oLeader, oBank, oMats }
new gO[MAX_ORGS][E_ORG];
new gRankName[MAX_ORGS][MAX_RANKS][32], gRankSalary[MAX_ORGS][MAX_RANKS], gRankSkin[MAX_ORGS][MAX_RANKS];
static const gOrgColor[] = {0xFFFFFF00, 0x3D7BFFFF, 0xFF7FA8FF, 0x9AA3B5FF, 0x6B8E23FF, 0xFFB347FF, 0x2ECC40FF, 0xB10DC9FF, 0x8B6B4AFF};

stock Org_Load()
{
    new DBResult:r = DB_Query("SELECT * FROM orgs");
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n; i++)
        {
            new id = DB_GetInt(r, "id");
            if (id > 0 && id < MAX_ORGS)
            {
                DB_GetStr(r, "name", gO[id][oName], 48);
                gO[id][oType] = DB_GetInt(r, "type"); gO[id][oLeader] = DB_GetInt(r, "leader");
                gO[id][oBank] = DB_GetInt(r, "bank"); gO[id][oMats] = DB_GetInt(r, "mats");
            }
            db_next_row(r);
        }
        db_free_result(r);
    }
    r = DB_Query("SELECT * FROM org_ranks");
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n; i++)
        {
            new o = DB_GetInt(r, "org"), rk = DB_GetInt(r, "rank");
            if (o > 0 && o < MAX_ORGS && rk > 0 && rk < MAX_RANKS)
            {
                DB_GetStr(r, "name", gRankName[o][rk], 32);
                gRankSalary[o][rk] = DB_GetInt(r, "salary"); gRankSkin[o][rk] = DB_GetInt(r, "skin");
            }
            db_next_row(r);
        }
        db_free_result(r);
    }
}

stock Org_Save(o)
{
    new q[160]; format(q, sizeof q, "UPDATE orgs SET leader=%d,bank=%d,mats=%d WHERE id=%d", gO[o][oLeader], gO[o][oBank], gO[o][oMats], o);
    DB_Exec(q);
}

stock Org_Name(o) { new t[48]; if (o > 0 && o < MAX_ORGS) strcpy(t, gO[o][oName]); else t = "—"; return t; }
stock Org_Type(o) return (o > 0 && o < MAX_ORGS) ? gO[o][oType] : 0;
stock bool:IsCop(playerid) return P[playerid][pOrg] == ORG_LSPD || P[playerid][pOrg] == ORG_ARMY;
stock bool:IsMedic(playerid) return P[playerid][pOrg] == ORG_EMS;
stock bool:IsLeader(playerid) return P[playerid][pOrg] > 0 && P[playerid][pOrgRank] >= 6;

stock Org_Salary(playerid)
{
    new o = P[playerid][pOrg], r = P[playerid][pOrgRank];
    if (o > 0 && o < MAX_ORGS && r > 0 && r < MAX_RANKS)
    {
        if (gO[o][oType] == 1) return gRankSalary[o][r];
        // криминал — доля из общака
        if (gO[o][oBank] >= 200) { gO[o][oBank] -= 200; Org_Save(o); return 200; }
        return 0;
    }
    if (P[playerid][pLevel] <= 3) return 100; // пособие
    return 0;
}

stock Org_SetMember(playerid, o, rank)
{
    P[playerid][pOrg] = o; P[playerid][pOrgRank] = rank;
    P[playerid][pOrgSkin] = (o && rank) ? gRankSkin[o][rank] : 0;
    P[playerid][pDuty] = false;
    SetPlayerColor(playerid, o ? gOrgColor[o] : 0xFFFFFF00);
    SetPlayerSkin(playerid, P[playerid][pSkin]);
    P_Save(playerid, false);
}

stock Org_Join(playerid, o)
{
    if (P[playerid][pOrg]) return Err(playerid, "Вы уже состоите в организации.");
    if (gO[o][oType] == 1)
    {
        if (!HasPassport(playerid) || !P[playerid][pMedcard]) return Err(playerid, "Для госслужбы нужны паспорт и медкарта.");
        if (P[playerid][pLevel] < 2) return Err(playerid, "Госслужба доступна со 2 уровня.");
        if (P[playerid][pWanted]) return Err(playerid, "Вы в розыске.");
        if (P[playerid][pJob]) { P[playerid][pJob] = 0; Info(playerid, "Вы уволены с прежней работы."); }
    }
    else if (P[playerid][pLevel] < 2) return Err(playerid, "Вербовщик: «Подрасти сначала» (нужен 2 уровень).");
    Org_SetMember(playerid, o, 1);
    new s[128]; format(s, sizeof s, "Вы вступили в «%s» (ранг: %s).", gO[o][oName], gRankName[o][1]);
    Ok(playerid, s);
    Quest_Event(playerid, "org_join", gO[o][oType] == 1 ? ("state") : ("crime"));
    return 1;
}

// /r — рация организации
CMD:r(playerid, const params[])
{
    new o = P[playerid][pOrg];
    if (!o) return Err(playerid, "Вы не состоите в организации.");
    if (isnull(params)) return Err(playerid, "/r [текст]");
    new s[144]; format(s, sizeof s, "[R] %s %s: %s", gRankName[o][P[playerid][pOrgRank]], PName(playerid), params);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && P[i][pOrg] == o) SCM(i, C_RADIO, s);
    return 1;
}

// /d — общая волна госструктур
CMD:d(playerid, const params[])
{
    new o = P[playerid][pOrg];
    if (Org_Type(o) != 1) return Err(playerid, "Волна департаментов доступна только госструктурам.");
    if (isnull(params)) return Err(playerid, "/d [текст]");
    new s[144]; format(s, sizeof s, "[D] [%s] %s: %s", gO[o][oName], PName(playerid), params);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && Org_Type(P[i][pOrg]) == 1) SCM(i, 0xFF8282FF, s);
    return 1;
}

CMD:duty(playerid, const params[])
{
    new o = P[playerid][pOrg];
    if (!o) return Err(playerid, "Вы не в организации.");
    new code[12]; format(code, sizeof code, "org_%d", o);
    if (!IsPlayerAtLoc(playerid, code, 25.0) && !IsPlayerAtLoc(playerid, "lspd_in", 40.0)) return Err(playerid, "Переодеться можно только в штабе.");
    P[playerid][pDuty] = !P[playerid][pDuty];
    P[playerid][pOrgSkin] = gRankSkin[o][P[playerid][pOrgRank]];
    SetPlayerSkin(playerid, P[playerid][pDuty] ? P[playerid][pOrgSkin] : P[playerid][pSkin]);
    if (P[playerid][pDuty] && (o == ORG_LSPD || o == ORG_ARMY)) { SetPlayerArmour(playerid, 100.0); GivePlayerWeapon(playerid, 3, 1); GivePlayerWeapon(playerid, 24, 50); }
    if (!P[playerid][pDuty]) ResetPlayerWeapons(playerid);
    return Ok(playerid, P[playerid][pDuty] ? ("Вы заступили на смену.") : ("Смена окончена."));
}

CMD:members(playerid, const params[])
{
    new o = P[playerid][pOrg];
    if (!o) return Err(playerid, "Вы не в организации.");
    new s[128];
    format(s, sizeof s, "Онлайн в «%s»:", gO[o][oName]); SCM(playerid, C_BLUE, s);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsLogged(i) && P[i][pOrg] == o) { format(s, sizeof s, "  %s [%d] — %s%s", PName(i), i, gRankName[o][P[i][pOrgRank]], P[i][pDuty] ? (" (на смене)") : ("")); SCM(playerid, C_WHITE, s); }
    return 1;
}

CMD:invite(playerid, const params[])
{
    if (P[playerid][pOrgRank] < 5) return Err(playerid, "Приглашать могут 5-6 ранги.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || !P[t][pLogged] || t == playerid) return Err(playerid, "/invite [id]");
    if (P[t][pOrg]) return Err(playerid, "Игрок уже в организации.");
    P[t][pInvite] = P[playerid][pOrg]; P[t][pInviteType] = 1;
    new s[128]; format(s, sizeof s, "%s приглашает вас в «%s».", PName(playerid), gO[P[playerid][pOrg]][oName]);
    UI_Prompt(t, "orginvite", "Приглашение", s, false, "", "Вступить");
    return Ok(playerid, "Приглашение отправлено.");
}

PROMPT:orginvite(playerid, response, const text[])
{
    UI_Close(playerid);
    new o = P[playerid][pInvite]; P[playerid][pInvite] = 0;
    if (!response || !o) return 1;
    if (P[playerid][pOrg]) return 1;
    // приглашение лидера обходит требования к уровню, но не к документам для госструктур
    if (gO[o][oType] == 1 && (!HasPassport(playerid) || !P[playerid][pMedcard])) return Err(playerid, "Для госслужбы нужны паспорт и медкарта.");
    Org_SetMember(playerid, o, 1);
    Ok(playerid, "Добро пожаловать в организацию! /r — рация, /duty — смена.");
    Quest_Event(playerid, "org_join", gO[o][oType] == 1 ? ("state") : ("crime"));
    return 1;
}

CMD:uninvite(playerid, const params[])
{
    if (P[playerid][pOrgRank] < 5) return Err(playerid, "Нет прав.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || P[t][pOrg] != P[playerid][pOrg] || t == playerid) return Err(playerid, "/uninvite [id]");
    if (P[t][pOrgRank] >= P[playerid][pOrgRank]) return Err(playerid, "Нельзя уволить равного или старшего.");
    Org_SetMember(t, 0, 0);
    Err(t, "Вас исключили из организации.");
    return Ok(playerid, "Игрок исключён.");
}

CMD:giverank(playerid, const params[])
{
    if (P[playerid][pOrgRank] < 5) return Err(playerid, "Нет прав.");
    new t = GetParamPlayer(params, 0), rk = GetParamInt(params, 1, 0);
    if (t == INVALID_PLAYER_ID || P[t][pOrg] != P[playerid][pOrg] || rk < 1 || rk >= P[playerid][pOrgRank]) return Err(playerid, "/giverank [id] [ранг ниже вашего]");
    Org_SetMember(t, P[playerid][pOrg], rk);
    new s[96]; format(s, sizeof s, "Ваш новый ранг: %s", gRankName[P[t][pOrg]][rk]); Ok(t, s);
    return Ok(playerid, "Ранг изменён.");
}

// ---------- меню лидера (CEF) ----------
CMD:lmenu(playerid, const params[])
{
    if (P[playerid][pOrgRank] < 5) return Err(playerid, "Меню доступно 5-6 рангам.");
    new o = P[playerid][pOrg], j[4096], t[160];
    format(j, sizeof j, "{\"org\":\"%s\",\"bank\":%d,\"mats\":%d,\"myrank\":%d,\"ranks\":[", JsonEsc(gO[o][oName]), gO[o][oBank], gO[o][oMats], P[playerid][pOrgRank]);
    for (new r = 1; r < MAX_RANKS; r++) { format(t, sizeof t, "%s\"%s\"", r > 1 ? "," : "", JsonEsc(gRankName[o][r])); strcat(j, t); }
    strcat(j, "],\"members\":[");
    new q[128]; format(q, sizeof q, "SELECT id,name,org_rank,last_login FROM accounts WHERE org=%d ORDER BY org_rank DESC LIMIT 60", o);
    new DBResult:r = DB_Query(q);
    if (r)
    {
        new name[24], last[24];
        for (new i = 0, n = db_num_rows(r); i < n; i++)
        {
            DB_GetStr(r, "name", name); DB_GetStr(r, "last_login", last);
            new on = FindPlayerByAcc(DB_GetInt(r, "id"));
            format(t, sizeof t, "%s{\"id\":%d,\"n\":\"%s\",\"r\":%d,\"on\":%d,\"last\":\"%s\"}", i ? "," : "", DB_GetInt(r, "id"), name, on != INVALID_PLAYER_ID ? P[on][pOrgRank] : DB_GetInt(r, "org_rank"), on != INVALID_PLAYER_ID ? 1 : 0, last);
            strcat(j, t);
            db_next_row(r);
        }
        db_free_result(r);
    }
    strcat(j, "]}");
    UI_Open(playerid, "leader", j);
    return 1;
}

UI:leader(playerid, const a[])
{
    if (P[playerid][pOrgRank] < 5) return 1;
    new act[12], o = P[playerid][pOrg];
    CefField(a, 1, act);
    new acc = CefFieldInt(a, 2), val = CefFieldInt(a, 3), q[160];
    if (StrEq(act, "rank") || StrEq(act, "kick"))
    {
        if (acc == P[playerid][pID]) return 1;
        format(q, sizeof q, "SELECT org_rank FROM accounts WHERE id=%d AND org=%d", acc, o);
        new cur = DB_ScalarInt(q, -1);
        new t = FindPlayerByAcc(acc);
        if (t != INVALID_PLAYER_ID) cur = (P[t][pOrg] == o) ? P[t][pOrgRank] : -1;
        if (cur < 0 || cur >= P[playerid][pOrgRank]) return Err(playerid, "Нет прав на это действие.");
        if (StrEq(act, "rank"))
        {
            if (val < 1 || val >= P[playerid][pOrgRank]) return Err(playerid, "Недопустимый ранг.");
            if (t != INVALID_PLAYER_ID) Org_SetMember(t, o, val);
            else { format(q, sizeof q, "UPDATE accounts SET org_rank=%d WHERE id=%d", val, acc); DB_Exec(q); }
        }
        else
        {
            if (t != INVALID_PLAYER_ID) { Org_SetMember(t, 0, 0); Err(t, "Вас исключили из организации."); }
            else { format(q, sizeof q, "UPDATE accounts SET org=0,org_rank=0 WHERE id=%d", acc); DB_Exec(q); }
        }
        Ok(playerid, "Готово.");
        cmd_lmenu(playerid, "\1");
    }
    else if (StrEq(act, "withdraw"))
    {
        if (!IsLeader(playerid)) return Err(playerid, "Только лидер.");
        if (val <= 0 || val > gO[o][oBank]) return Err(playerid, "Неверная сумма.");
        gO[o][oBank] -= val; Org_Save(o);
        Money_Give(playerid, val, "org withdraw");
        new l[64]; format(l, sizeof l, "org %d withdraw %d", o, val); Log("org", PName(playerid), l);
        cmd_lmenu(playerid, "\1");
    }
    else if (StrEq(act, "deposit"))
    {
        if (val <= 0 || !Money_Take(playerid, val)) return Err(playerid, "Недостаточно наличных.");
        gO[o][oBank] += val; Org_Save(o);
        cmd_lmenu(playerid, "\1");
    }
    return 1;
}

// ---------- штаб: склад, вступление, снаряжение ----------
stock Org_HQ(playerid, o)
{
    Menu_Begin(gO[o][oName], gO[o][oType] == 1 ? ("Государственная структура") : ("Криминальная группировка"));
    if (P[playerid][pOrg] == o)
    {
        Menu_Add(1, "Склад", "Общее хранилище", -1, "box");
        if (o != ORG_EMS && o != ORG_GOV && o != ORG_NEWS) Menu_Add(2, "Получить снаряжение", "Расход материалов: 50", -1, "gun");
        Menu_Add(3, P[playerid][pDuty] ? ("Закончить смену") : ("Начать смену (форма)"), "", -1, "shirt");
    }
    else if (!P[playerid][pOrg] && gO[o][oType] != 1) Menu_Add(4, "Поговорить с вербовщиком", "Вступить (с 2 уровня)", -1, "user");
    else if (!P[playerid][pOrg]) Menu_Add(0, "Набор", "Заявления — в отделе кадров мэрии");
    SetPVarInt(playerid, "orgHQ", o);
    Menu_Send(playerid, "orghq");
    return 1;
}

MENU:orghq(playerid, item, const extra[])
{
    new o = GetPVarInt(playerid, "orgHQ");
    if (o < 1 || o >= MAX_ORGS) return 1;
    switch (item)
    {
        case 1: if (P[playerid][pOrg] == o) Storage_Open(playerid, ST_ORG, o, "Склад организации");
        case 2:
        {
            UI_Close(playerid);
            if (P[playerid][pOrg] != o) return 1;
            if (gO[o][oMats] < 50) return Err(playerid, "На складе нет материалов.");
            gO[o][oMats] -= 50; Org_Save(o);
            if (o == ORG_LSPD || o == ORG_ARMY)
            {
                Inv_Add(playerid, Item_ByKey(o == ORG_ARMY ? ("m4") : ("deagle")), 1);
                Inv_Add(playerid, Item_ByKey("ammo"), 100);
                Inv_Add(playerid, Item_ByKey("armour"), 1);
                if (o == ORG_LSPD) Inv_Add(playerid, Item_ByKey("baton"), 1);
            }
            else
            {
                Inv_Add(playerid, Item_ByKey(P[playerid][pOrgRank] >= 3 ? ("ak47") : ("colt")), 1);
                Inv_Add(playerid, Item_ByKey("ammo"), 60);
            }
            Ok(playerid, "Снаряжение получено (в инвентаре).");
        }
        case 3: { UI_Close(playerid); cmd_duty(playerid, "\1"); }
        case 4: { UI_Close(playerid); Org_Join(playerid, o); }
        default: UI_Close(playerid);
    }
    return 1;
}

// отдел кадров мэрии — вступление в госструктуры
stock HR_Open(playerid)
{
    Menu_Begin("Отдел кадров", "Нужны паспорт, медкарта, 2 уровень");
    for (new o = 1; o < MAX_ORGS; o++) if (gO[o][oType] == 1) Menu_Add(o, gO[o][oName], gRankName[o][1], gRankSalary[o][1], "org");
    Menu_Send(playerid, "hr");
    return 1;
}

MENU:hr(playerid, item, const extra[])
{
    UI_Close(playerid);
    if (item < 1 || item >= MAX_ORGS || gO[item][oType] != 1) return 1;
    return Org_Join(playerid, item);
}

// СМИ: объявления и новости
CMD:news(playerid, const params[])
{
    if (P[playerid][pOrg] != ORG_NEWS || P[playerid][pOrgRank] < 2) return Err(playerid, "Эфир доступен сотрудникам СМИ со 2 ранга.");
    if (isnull(params)) return Err(playerid, "/news [текст]");
    new s[144]; format(s, sizeof s, "[Godjo News] %s: %s", PName(playerid), params);
    SendClientMessageToAll(0x9ACD32FF, s);
    new q[300]; format(q, sizeof q, "INSERT INTO news (date,author,text) VALUES ('%s','%s','%s')", DateStr(), PName(playerid), SqlEsc(params)); DB_Exec(q);
    return 1;
}

CMD:ad(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/ad [текст] — объявление ($100)");
    if (!Pay(playerid, 100, "ad")) return 1;
    gO[ORG_NEWS][oBank] += 100; Org_Save(ORG_NEWS);
    new s[144]; format(s, sizeof s, "[Объявление] %s. Тел: %s (%s)", params, P[playerid][pPhone], PName(playerid));
    SendClientMessageToAll(0x9ACD32FF, s);
    return 1;
}

// /gov — заявление правительства
CMD:gov(playerid, const params[])
{
    if (!Org_Type(P[playerid][pOrg]) || Org_Type(P[playerid][pOrg]) != 1 || P[playerid][pOrgRank] < 5) return Err(playerid, "Только руководство госструктур.");
    if (isnull(params)) return Err(playerid, "/gov [текст]");
    new s[144]; format(s, sizeof s, "[Гос. новости] %s: %s", gO[P[playerid][pOrg]][oName], params);
    SendClientMessageToAll(0x6495EDFF, s);
    return 1;
}
