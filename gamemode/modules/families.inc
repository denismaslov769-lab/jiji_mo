// =====================================================================
//  Семьи: создание, приглашение, чат, ранги, семейный банк и сейф
// =====================================================================
#define FAMILY_PRICE 50000

stock Fam_Name(id)
{
    new n[32], q[80]; n = "—";
    format(q, sizeof q, "SELECT name FROM families WHERE id=%d", id);
    new DBResult:r = DB_Query(q);
    if (r) { if (db_num_rows(r)) db_get_field(r, 0, n, sizeof n); db_free_result(r); }
    return n;
}

stock FamReg_Use(playerid)
{
    if (P[playerid][pFamily]) return Err(playerid, "Вы уже состоите в семье. /fmenu — меню семьи.");
    if (P[playerid][pLevel] < 5) return Err(playerid, "Создать семью можно с 5 уровня.");
    UI_Prompt(playerid, "famcreate", "Создание семьи", "Введите фамилию семьи (3-20 символов). Стоимость регистрации — $50.000.", true, "Например: Корлеоне", "Создать");
    return 1;
}

PROMPT:famcreate(playerid, response, const text[])
{
    UI_Close(playerid);
    if (!response || isnull(text)) return 1;
    new len = strlen(text);
    if (len < 3 || len > 20) return Err(playerid, "Название — от 3 до 20 символов.");
    new q[200]; format(q, sizeof q, "SELECT COUNT(*) FROM families WHERE name='%s'", SqlEsc(text));
    if (DB_ScalarInt(q)) return Err(playerid, "Такая семья уже существует.");
    if (!Pay(playerid, FAMILY_PRICE, "family create")) return 1;
    format(q, sizeof q, "INSERT INTO families (name,leader,created) VALUES ('%s',%d,'%s')", SqlEsc(text), P[playerid][pID], DateStr());
    DB_Exec(q);
    P[playerid][pFamily] = DB_LastId(); P[playerid][pFamilyRank] = 3;
    P_Save(playerid, false);
    new s[96]; format(s, sizeof s, "Семья «%s» зарегистрирована! /fmenu, /finvite, /fc.", text);
    Ok(playerid, s);
    Ach_Give(playerid, "family", "Глава семьи");
    return 1;
}

CMD:fc(playerid, const params[])
{
    if (!P[playerid][pFamily]) return Err(playerid, "Вы не в семье.");
    if (isnull(params)) return Err(playerid, "/fc [текст]");
    new s[144]; format(s, sizeof s, "[Семья] %s: %s", PName(playerid), params);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && P[i][pFamily] == P[playerid][pFamily]) SCM(i, C_FAMILY, s);
    return 1;
}

CMD:finvite(playerid, const params[])
{
    if (!P[playerid][pFamily] || P[playerid][pFamilyRank] < 2) return Err(playerid, "Приглашать могут глава и заместители.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || !P[t][pLogged] || P[t][pFamily] || GetNearestPlayer(playerid, 5.0) != t) return Err(playerid, "/finvite [id] — игрок рядом и без семьи.");
    P[t][pFamilyInvite] = P[playerid][pFamily];
    new s[128]; format(s, sizeof s, "%s приглашает вас в семью «%s».", PName(playerid), Fam_Name(P[playerid][pFamily]));
    UI_Prompt(t, "faminvite", "Приглашение в семью", s, false, "", "Вступить");
    return Ok(playerid, "Приглашение отправлено.");
}

PROMPT:faminvite(playerid, response, const text[])
{
    UI_Close(playerid);
    new f = P[playerid][pFamilyInvite]; P[playerid][pFamilyInvite] = 0;
    if (!response || !f || P[playerid][pFamily]) return 1;
    P[playerid][pFamily] = f; P[playerid][pFamilyRank] = 1; P_Save(playerid, false);
    new s[96]; format(s, sizeof s, "Вы вступили в семью «%s».", Fam_Name(f));
    return Ok(playerid, s);
}

CMD:fmenu(playerid, const params[])
{
    if (!P[playerid][pFamily]) return Err(playerid, "Вы не в семье.");
    new q[96]; format(q, sizeof q, "SELECT bank FROM families WHERE id=%d", P[playerid][pFamily]);
    new bank = DB_ScalarInt(q), sub[64];
    format(sub, sizeof sub, "Семейный банк: %s", FormatMoney(bank));
    Menu_Begin(Fam_Name(P[playerid][pFamily]), sub);
    Menu_Add(1, "Состав семьи", "", -1, "users");
    Menu_Add(2, "Положить в банк семьи $1.000", "", 1000, "cash");
    if (P[playerid][pFamilyRank] >= 3) Menu_Add(3, "Снять из банка семьи $1.000", "", -1, "cash");
    Menu_Add(4, "Семейный сейф", "Доступен в доме члена семьи", -1, "box");
    Menu_Add(5, "Покинуть семью", "", -1, "x");
    Menu_Send(playerid, "fmenu");
    return 1;
}

MENU:fmenu(playerid, item, const extra[])
{
    new f = P[playerid][pFamily], q[160];
    if (!f) return 1;
    switch (item)
    {
        case 1:
        {
            Menu_Begin("Состав семьи", Fam_Name(f));
            format(q, sizeof q, "SELECT id,name,family_rank FROM accounts WHERE family=%d ORDER BY family_rank DESC LIMIT 50", f);
            new DBResult:r = DB_Query(q);
            if (r)
            {
                new n[24];
                for (new i = 0, c = db_num_rows(r); i < c; i++)
                {
                    DB_GetStr(r, "name", n);
                    new rk = DB_GetInt(r, "family_rank");
                    Menu_Add(DB_GetInt(r, "id"), n, rk >= 3 ? ("Глава") : rk == 2 ? ("Заместитель") : ("Родственник"), -1, FindPlayerByAcc(DB_GetInt(r, "id")) != INVALID_PLAYER_ID ? ("online") : ("user"));
                    db_next_row(r);
                }
                db_free_result(r);
            }
            Menu_Send(playerid, "fammembers");
        }
        case 2:
        {
            UI_Close(playerid);
            if (!Pay(playerid, 1000, "family deposit")) return 1;
            format(q, sizeof q, "UPDATE families SET bank=bank+1000 WHERE id=%d", f); DB_Exec(q);
            Ok(playerid, "Внесено в семейный банк.");
        }
        case 3:
        {
            UI_Close(playerid);
            if (P[playerid][pFamilyRank] < 3) return 1;
            format(q, sizeof q, "SELECT bank FROM families WHERE id=%d", f);
            if (DB_ScalarInt(q) < 1000) return Err(playerid, "В семейном банке недостаточно денег.");
            format(q, sizeof q, "UPDATE families SET bank=bank-1000 WHERE id=%d", f); DB_Exec(q);
            Money_Give(playerid, 1000, "family withdraw");
        }
        case 4:
        {
            new k = House_Idx(P[playerid][pHouseIn]);
            if (k == -1 || !gH[k][hOwner]) { UI_Close(playerid); return Err(playerid, "Зайдите в дом члена семьи."); }
            format(q, sizeof q, "SELECT family FROM accounts WHERE id=%d", gH[k][hOwner]);
            if (DB_ScalarInt(q) != f) { UI_Close(playerid); return Err(playerid, "Это дом не вашей семьи."); }
            Storage_Open(playerid, ST_FAMILY, f, "Семейный сейф");
        }
        case 5:
        {
            UI_Close(playerid);
            if (P[playerid][pFamilyRank] >= 3)
            {
                format(q, sizeof q, "UPDATE accounts SET family=0,family_rank=0 WHERE family=%d", f); DB_Exec(q);
                format(q, sizeof q, "DELETE FROM families WHERE id=%d", f); DB_Exec(q);
                for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && P[i][pFamily] == f) { P[i][pFamily] = 0; P[i][pFamilyRank] = 0; Err(i, "Семья распущена."); }
            }
            else { P[playerid][pFamily] = 0; P[playerid][pFamilyRank] = 0; P_Save(playerid, false); Ok(playerid, "Вы покинули семью."); }
        }
    }
    return 1;
}

MENU:fammembers(playerid, item, const extra[])
{
    // глава может назначить заместителя / исключить
    if (P[playerid][pFamilyRank] < 3 || item == P[playerid][pID]) return UI_Close(playerid);
    SetPVarInt(playerid, "famTarget", item);
    Menu_Begin(AccName(item), "Управление");
    Menu_Add(1, "Назначить заместителем", "", -1, "up");
    Menu_Add(2, "Сделать родственником", "", -1, "down");
    Menu_Add(3, "Исключить из семьи", "", -1, "x");
    Menu_Send(playerid, "famact");
    return 1;
}

MENU:famact(playerid, item, const extra[])
{
    UI_Close(playerid);
    new acc = GetPVarInt(playerid, "famTarget"), q[128], f = P[playerid][pFamily];
    if (P[playerid][pFamilyRank] < 3 || !acc) return 1;
    new t = FindPlayerByAcc(acc);
    new nr = item == 1 ? 2 : item == 2 ? 1 : 0;
    if (t != INVALID_PLAYER_ID) { if (P[t][pFamily] != f) return 1; P[t][pFamilyRank] = nr; if (!nr) { P[t][pFamily] = 0; Err(t, "Вас исключили из семьи."); } P_Save(t, false); }
    else
    {
        if (nr) format(q, sizeof q, "UPDATE accounts SET family_rank=%d WHERE id=%d AND family=%d", nr, acc, f);
        else format(q, sizeof q, "UPDATE accounts SET family=0,family_rank=0 WHERE id=%d AND family=%d", acc, f);
        DB_Exec(q);
    }
    return Ok(playerid, "Готово.");
}

stock Fam_Payday(playerid)
{
    if (!P[playerid][pFamily]) return;
    new q[96]; format(q, sizeof q, "UPDATE families SET bank=bank+50 WHERE id=%d", P[playerid][pFamily]); DB_Exec(q);
}
