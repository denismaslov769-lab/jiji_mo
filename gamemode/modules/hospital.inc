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
