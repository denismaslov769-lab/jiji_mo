// =====================================================================
//  Потребности: голод и усталость (раз в минуту)
// =====================================================================
stock Needs_Tick(playerid)
{
    if (P[playerid][pJail] > 0) { P[playerid][pHunger] -= 0.2; }
    else
    {
        P[playerid][pHunger] -= 0.55;
        P[playerid][pFatigue] -= (P[playerid][pOnJob] ? 0.8 : 0.35);
    }
    if (P[playerid][pHunger] < 0.0) P[playerid][pHunger] = 0.0;
    if (P[playerid][pFatigue] < 0.0) P[playerid][pFatigue] = 0.0;
    new Float:hp;
    GetPlayerHealth(playerid, hp);
    if (P[playerid][pHunger] <= 0.0)
    {
        SetPlayerHealth(playerid, hp - 4.0);
        Err(playerid, "Вы умираете от голода! Срочно поешьте.");
    }
    else if (P[playerid][pHunger] < 15.0) Info(playerid, "Вы голодны. Найдите еду.");
    if (P[playerid][pFatigue] <= 0.0)
    {
        SetPlayerHealth(playerid, hp - 2.0);
        SetPlayerDrunkLevel(playerid, 3000);
        Err(playerid, "Вы валитесь с ног от усталости. Нужен сон (/sleep дома или в ночлежке).");
    }
    else if (P[playerid][pFatigue] < 15.0) Info(playerid, "Вы устали. Отдохните или выпейте энергетик.");
}

new gSleepTimer[MAX_PLAYERS] = {-1, ...};

CMD:sleep(playerid, const params[])
{
    new bool:ok = false;
    if (IsPlayerAtLoc(playerid, "shelter", 6.0)) ok = true;
    if (P[playerid][pHouseIn]) ok = true;
    if (!ok) return Err(playerid, "Спать можно в ночлежке или у себя дома.");
    if (gSleepTimer[playerid] != -1) return Err(playerid, "Вы уже спите.");
    TogglePlayerControllable(playerid, false);
    ApplyAnimation(playerid, "CRACK", "crckidle2", 4.0, 1, 0, 0, 0, 0);
    gSleepTimer[playerid] = SetTimerEx("Sleep_Tick", 3000, true, "i", playerid);
    Info(playerid, "Вы легли спать. /wake — проснуться.");
    return 1;
}

CMD:wake(playerid, const params[])
{
    if (gSleepTimer[playerid] == -1) return 1;
    KillTimer(gSleepTimer[playerid]); gSleepTimer[playerid] = -1;
    TogglePlayerControllable(playerid, true);
    ClearAnimations(playerid);
    return Ok(playerid, "Вы проснулись.");
}

forward Sleep_Tick(playerid);
public Sleep_Tick(playerid)
{
    P[playerid][pFatigue] += (P[playerid][pHouseIn] ? 6.0 : 3.0);
    if (P[playerid][pFatigue] >= 100.0)
    {
        P[playerid][pFatigue] = 100.0;
        cmd_wake(playerid, "\1");
        Ok(playerid, "Вы отлично выспались.");
    }
    UI_Hud(playerid);
}
