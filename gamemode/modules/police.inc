// =====================================================================
//  Полиция и розыск: звёзды, наручники, обыск, КПЗ, штрафы
// =====================================================================
stock Police_UpdateWanted(playerid)
{
    SetPlayerWantedLevel(playerid, P[playerid][pWanted]);
    UI_Hud(playerid);
}

stock Police_AddWanted(playerid, amount, const reason[])
{
    P[playerid][pWanted] += amount;
    if (P[playerid][pWanted] > 6) P[playerid][pWanted] = 6;
    Police_UpdateWanted(playerid);
    new s[144]; format(s, sizeof s, "Вы объявлены в розыск: %s. Уровень: %d", reason, P[playerid][pWanted]);
    Err(playerid, s);
    format(s, sizeof s, "[Диспетчер] Розыск: %s [%d], ур. %d — %s", PName(playerid), playerid, P[playerid][pWanted], reason);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && IsCop(i) && P[i][pDuty]) SCM(i, C_BLUE, s);
}

stock bool:CopCheck(playerid)
{
    if (!IsCop(playerid)) { Err(playerid, "Только для сотрудников полиции/армии."); return false; }
    if (!P[playerid][pDuty]) { Err(playerid, "Вы не на смене (/duty)."); return false; }
    return true;
}

stock CopTarget(playerid, const params[], Float:range = 4.0)
{
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || !P[t][pLogged] || t == playerid) return INVALID_PLAYER_ID;
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z);
    if (!IsPlayerInRangeOfPoint(playerid, range, x, y, z)) return INVALID_PLAYER_ID;
    return t;
}

CMD:su(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    new t = GetParamPlayer(params, 0), lvl = GetParamInt(params, 1, 0), reason[64];
    GetRest(params, 2, reason);
    if (t == INVALID_PLAYER_ID || lvl < 1 || lvl > 6 || !reason[0]) return Err(playerid, "/su [id] [1-6] [причина]");
    Police_AddWanted(t, lvl, reason);
    return Ok(playerid, "Розыск выдан.");
}

CMD:clear(playerid, const params[])
{
    if (!CopCheck(playerid) || P[playerid][pOrgRank] < 3) return Err(playerid, "Снимать розыск могут с 3 ранга.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID) return Err(playerid, "/clear [id]");
    P[t][pWanted] = 0; Police_UpdateWanted(t);
    Ok(t, "С вас снят розыск."); return Ok(playerid, "Розыск снят.");
}

CMD:wanted(playerid, const params[])
{
    if (!IsCop(playerid)) return Err(playerid, "Только для полиции.");
    new s[96], c = 0;
    SCM(playerid, C_BLUE, "Разыскиваемые в сети:");
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i) && P[i][pWanted]) { format(s, sizeof s, "  %s [%d] — %d ур.", PName(i), i, P[i][pWanted]); SCM(playerid, C_WHITE, s); c++; }
    if (!c) SCM(playerid, C_GREY, "  Никого нет.");
    return 1;
}

CMD:cuff(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    new t = CopTarget(playerid, params, 2.5);
    if (t == INVALID_PLAYER_ID) return Err(playerid, "/cuff [id] — подойдите ближе.");
    if (P[t][pCuffed]) return Err(playerid, "Уже в наручниках.");
    new sa = GetPlayerSpecialAction(t);
    if (sa != SPECIAL_ACTION_HANDSUP && !P[t][pInjured] && GetPlayerState(t) != PLAYER_STATE_ONFOOT) return Err(playerid, "Нельзя надеть наручники сейчас.");
    P[t][pCuffed] = true; P[t][pCuffedBy] = playerid;
    SetPlayerSpecialAction(t, SPECIAL_ACTION_CUFFED);
    SetPlayerAttachedObject(t, 6, 19418, 6, -0.011, 0.028, -0.022, -15.6, -33.7, -81.7, 0.89, 1.0, 1.16);
    new s[96]; format(s, sizeof s, "надел наручники на %s", PName(t)); SendMe(playerid, s);
    return 1;
}

CMD:uncuff(playerid, const params[])
{
    if (!IsCop(playerid)) return Err(playerid, "Только для полиции.");
    new t = CopTarget(playerid, params, 2.5);
    if (t == INVALID_PLAYER_ID || !P[t][pCuffed]) return Err(playerid, "/uncuff [id]");
    P[t][pCuffed] = false; P[t][pCuffedBy] = INVALID_PLAYER_ID;
    SetPlayerSpecialAction(t, SPECIAL_ACTION_NONE);
    RemovePlayerAttachedObject(t, 6);
    new s[96]; format(s, sizeof s, "снял наручники с %s", PName(t)); SendMe(playerid, s);
    return 1;
}

CMD:follow(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    new t = CopTarget(playerid, params, 3.0);
    if (t == INVALID_PLAYER_ID || !P[t][pCuffed]) return Err(playerid, "/follow [id] — только задержанного.");
    new v = GetPlayerVehicleID(playerid);
    if (v) { PutPlayerInVehicle(t, v, 2); return Ok(playerid, "Задержанный посажен в машину."); }
    new Float:x, Float:y, Float:z; GetPlayerPos(playerid, x, y, z);
    SetPlayerPos(t, x + 0.8, y, z);
    return 1;
}

CMD:frisk(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    new t = CopTarget(playerid, params, 2.5);
    if (t == INVALID_PLAYER_ID) return Err(playerid, "/frisk [id]");
    new s[144], cnt = 0;
    format(s, sizeof s, "обыскивает %s", PName(t)); SendMe(playerid, s);
    format(s, sizeof s, "Результат обыска %s:", PName(t)); SCM(playerid, C_BLUE, s);
    for (new i = 0; i < MAX_INV_SLOTS; i++)
    {
        new it = gInv[t][i][0];
        if (!it) continue;
        format(s, sizeof s, "  %s x%d%s", gItems[it][iName], gInv[t][i][1], (gItems[it][iType] == IT_WEAPON || StrEq(gItems[it][iKey], "lockpick")) ? (" {FF5A5A}[запрещено]") : (""));
        SCM(playerid, C_WHITE, s); cnt++;
    }
    new w, a;
    for (new sl = 0; sl < 13; sl++) { GetPlayerWeaponData(t, sl, w, a); if (w > 0 && a > 0) { format(s, sizeof s, "  Оружие в руках: ID %d (%d патр.)", w, a); SCM(playerid, C_RED, s); cnt++; } }
    if (!cnt) SCM(playerid, C_GREY, "  Ничего не найдено.");
    SCM(playerid, C_GREY, "Изъять запрещённое: /confiscate [id]");
    return 1;
}

CMD:confiscate(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    new t = CopTarget(playerid, params, 2.5);
    if (t == INVALID_PLAYER_ID) return Err(playerid, "/confiscate [id]");
    for (new i = 0; i < MAX_INV_SLOTS; i++)
    {
        new it = gInv[t][i][0];
        if (it && (gItems[it][iType] == IT_WEAPON || StrEq(gItems[it][iKey], "lockpick"))) { gInv[t][i][0] = 0; gInv[t][i][1] = 0; Inv_SaveSlot(t, i); }
    }
    ResetPlayerWeapons(t);
    Inv_Refresh(t);
    Err(t, "У вас изъяли запрещённые предметы.");
    return Ok(playerid, "Запрещённое изъято.");
}

CMD:arrest(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    if (!IsPlayerAtLoc(playerid, "lspd_door", 15.0) && !IsPlayerAtLoc(playerid, "lspd_in", 40.0)) return Err(playerid, "Доставьте задержанного в участок LSPD.");
    new t = CopTarget(playerid, params, 5.0);
    if (t == INVALID_PLAYER_ID || !P[t][pWanted]) return Err(playerid, "/arrest [id] — задержанный в розыске рядом.");
    new mins = P[t][pWanted] * 5;
    P[t][pJail] = mins; P[t][pWanted] = 0; Police_UpdateWanted(t);
    if (P[t][pCuffed]) cmd_uncuff(playerid, params);
    ResetPlayerWeapons(t);
    Loc_Teleport(t, "jail");
    new s[128]; format(s, sizeof s, "Вы посажены в КПЗ на %d мин.", mins); Err(t, s);
    format(s, sizeof s, "Задержанный %s помещён в КПЗ на %d мин. Премия: $%d", PName(t), mins, mins * 20);
    Ok(playerid, s);
    Money_Give(playerid, mins * 20, "arrest");
    Log("police", PName(playerid), PName(t));
    return 1;
}

stock Jail_Tick(playerid)
{
    if (P[playerid][pJail] <= 0) return;
    P[playerid][pJail]--;
    if (!IsPlayerAtLoc(playerid, "jail", 15.0)) Loc_Teleport(playerid, "jail");
    if (P[playerid][pJail] <= 0)
    {
        Loc_Teleport(playerid, "jail_out");
        Ok(playerid, "Срок истёк. Вы на свободе.");
    }
}

CMD:ticket(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    new t = CopTarget(playerid, params, 5.0), sum = GetParamInt(params, 1, 0), reason[64];
    GetRest(params, 2, reason);
    if (t == INVALID_PLAYER_ID || sum < 50 || sum > 5000 || !reason[0]) return Err(playerid, "/ticket [id] [50-5000] [причина]");
    new q[300]; format(q, sizeof q, "INSERT INTO fines (acc,amount,reason,officer) VALUES (%d,%d,'%s','%s')", P[t][pID], sum, SqlEsc(reason), PName(playerid));
    DB_Exec(q);
    new s[144]; format(s, sizeof s, "Вам выписан штраф %s: %s. Оплатить: /fines", FormatMoney(sum), reason); Err(t, s);
    return Ok(playerid, "Штраф выписан.");
}

CMD:fines(playerid, const params[])
{
    Menu_Begin("Мои штрафы", "Нажмите, чтобы оплатить");
    new q[128]; format(q, sizeof q, "SELECT id,amount,reason FROM fines WHERE acc=%d AND paid=0", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    if (r)
    {
        new reason[64];
        for (new i = 0, n = db_num_rows(r); i < n; i++) { DB_GetStr(r, "reason", reason); Menu_Add(DB_GetInt(r, "id"), reason, "", DB_GetInt(r, "amount"), "tax"); db_next_row(r); }
        db_free_result(r);
    }
    if (!gMBCount) Menu_Add(0, "Штрафов нет", "");
    Menu_Send(playerid, "fines");
    return 1;
}

MENU:fines(playerid, item, const extra[])
{
    UI_Close(playerid);
    if (!item) return 1;
    new q[128]; format(q, sizeof q, "SELECT amount FROM fines WHERE id=%d AND acc=%d AND paid=0", item, P[playerid][pID]);
    new sum = DB_ScalarInt(q, 0);
    if (!sum || !Pay(playerid, sum, "fine")) return 1;
    format(q, sizeof q, "UPDATE fines SET paid=1 WHERE id=%d", item); DB_Exec(q);
    gO[ORG_LSPD][oBank] += sum / 2; gTreasury += sum / 2;
    return Ok(playerid, "Штраф оплачен.");
}

CMD:m(playerid, const params[])
{
    if (!IsCop(playerid) || !IsPlayerInAnyVehicle(playerid)) return Err(playerid, "Мегафон — в служебной машине.");
    if (isnull(params)) return Err(playerid, "/m [текст]");
    new s[144]; format(s, sizeof s, "[Мегафон] %s: %s", PName(playerid), params);
    ProxMsg(playerid, 60.0, C_YELLOW, s);
    return 1;
}

CMD:impound(playerid, const params[])
{
    if (!CopCheck(playerid)) return 1;
    new v = Veh_Nearest(playerid, 5.0);
    if (v == INVALID_VEHICLE_ID || !gV[v][vDB]) return Err(playerid, "Рядом нет личного транспорта.");
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsPlayerConnected(i) && GetPlayerVehicleID(i) == v) return Err(playerid, "В машине кто-то есть.");
    Veh_Impound(v, "эвакуирован полицией");
    Money_Give(playerid, 100, "impound");
    return Ok(playerid, "Транспорт эвакуирован на штрафстоянку.");
}
