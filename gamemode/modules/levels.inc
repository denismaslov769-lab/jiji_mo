// =====================================================================
//  Уровни, опыт и достижения
// =====================================================================
stock Lvl_Need(level) return 4 + level * 2;

stock Lvl_AddExp(playerid, amount)
{
    P[playerid][pExp] += amount;
    while (P[playerid][pExp] >= Lvl_Need(P[playerid][pLevel]))
    {
        P[playerid][pExp] -= Lvl_Need(P[playerid][pLevel]);
        P[playerid][pLevel]++;
        SetPlayerScore(playerid, P[playerid][pLevel]);
        new s[96]; format(s, sizeof s, "Новый уровень: %d!", P[playerid][pLevel]);
        Ok(playerid, s);
        PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
        if (P[playerid][pLevel] == 2) Referral_Reward(playerid);
        if (P[playerid][pLevel] == 5) Ach_Give(playerid, "lvl5", "Свой человек");
        if (P[playerid][pLevel] == 10) Ach_Give(playerid, "lvl10", "Старожил");
    }
    UI_Hud(playerid);
}

stock Referral_Reward(playerid)
{
    if (!P[playerid][pRefBy][0]) return;
    new q[160];
    format(q, sizeof q, "SELECT ref_paid FROM accounts WHERE id=%d", P[playerid][pID]);
    if (DB_ScalarInt(q)) return;
    format(q, sizeof q, "UPDATE accounts SET ref_paid=1 WHERE id=%d", P[playerid][pID]); DB_Exec(q);
    format(q, sizeof q, "UPDATE accounts SET bank=bank+5000 WHERE name='%s'", SqlEsc(P[playerid][pRefBy])); DB_Exec(q);
    Money_Give(playerid, 2000, "referral");
    Ok(playerid, "Бонус за реферальный код: $2.000. Пригласившему начислено $5.000.");
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsLogged(i) && StrEq(PName(i), P[playerid][pRefBy])) { P[i][pBank] += 5000; Ok(i, "Ваш реферал достиг 2 уровня: +$5.000 на счёт."); }
}

stock Ach_Give(playerid, const code[], const title[])
{
    new q[200];
    format(q, sizeof q, "SELECT COUNT(*) FROM achievements WHERE acc=%d AND code='%s'", P[playerid][pID], code);
    if (DB_ScalarInt(q)) return 0;
    format(q, sizeof q, "INSERT INTO achievements VALUES (%d,'%s','%s')", P[playerid][pID], code, DateStr());
    DB_Exec(q);
    new j[160]; format(j, sizeof j, "{\"title\":\"%s\"}", JsonEsc(title));
    UI_Data(playerid, "achievement", j);
    new s[96]; format(s, sizeof s, "Достижение получено: «%s»", title);
    SCM(playerid, C_PURPLE, s);
    Lvl_AddExp(playerid, 1);
    return 1;
}

CMD:ach(playerid, const params[])
{
    new q[128];
    format(q, sizeof q, "SELECT code,date FROM achievements WHERE acc=%d", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    Menu_Begin("Достижения", "Ваши награды");
    if (r)
    {
        new code[24], date[24];
        for (new i = 0, n = db_num_rows(r); i < n; i++)
        {
            DB_GetStr(r, "code", code); DB_GetStr(r, "date", date);
            Menu_Add(i, Ach_Title(code), date);
            db_next_row(r);
        }
        db_free_result(r);
    }
    Menu_Send(playerid, "noop");
    return 1;
}

stock Ach_Title(const code[])
{
    new t[48];
    if (StrEq(code, "born")) t = "Новая жизнь";
    else if (StrEq(code, "lvl5")) t = "Свой человек";
    else if (StrEq(code, "lvl10")) t = "Старожил";
    else if (StrEq(code, "bottom")) t = "С самого дна";
    else if (StrEq(code, "passport")) t = "Гражданин";
    else if (StrEq(code, "driver")) t = "За рулём";
    else if (StrEq(code, "fisher")) t = "Рыбак";
    else if (StrEq(code, "treasure")) t = "Кладоискатель";
    else if (StrEq(code, "jackpot")) t = "Фортуна";
    else if (StrEq(code, "home")) t = "Крыша над головой";
    else if (StrEq(code, "car")) t = "Свои колёса";
    else if (StrEq(code, "family")) t = "Глава семьи";
    else strcpy(t, code);
    return t;
}

MENU:noop(playerid, item, const extra[]) { return 1; }
