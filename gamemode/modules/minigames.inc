// =====================================================================
//  Мини-игры (CEF-экран "mini"): используются в квестах, медкомиссии,
//  паспортном столе и на работах.
//  Типы: timing (поймать зону), mash (жать пробел), hold (удержать и
//  отпустить в зоне), wires (соединить провода), seq (повторить
//  последовательность стрелок), eye (таблица окулиста), photo (поймать
//  кадр), sign (подпись), code (запомнить код), find (найти предметы).
//  Сервер выдаёт токен и проверяет минимальное время прохождения.
// =====================================================================
new gMiniToken[MAX_PLAYERS], gMiniStart[MAX_PLAYERS], gMiniCtx[MAX_PLAYERS][24], gMiniType[MAX_PLAYERS][8];

stock Mini_Start(playerid, const type[], level, const ctx[], const title[], const hint[])
{
    gMiniToken[playerid] = 1000 + random(899000);
    gMiniStart[playerid] = GetTickCount();
    strcpy(gMiniCtx[playerid], ctx, 24);
    strcpy(gMiniType[playerid], type, 8);
    if (level < 1) level = 1; if (level > 5) level = 5;
    new j[512];
    format(j, sizeof j, "{\"type\":\"%s\",\"lvl\":%d,\"token\":%d,\"title\":\"%s\",\"hint\":\"%s\",\"seed\":%d}",
        type, level, gMiniToken[playerid], JsonEsc(title), JsonEsc(hint), random(100000));
    UI_Open(playerid, "mini", j);
    return 1;
}

stock Mini_MinMs(const type[])
{
    if (StrEq(type, "wires") || StrEq(type, "seq") || StrEq(type, "eye") || StrEq(type, "code")) return 2500;
    if (StrEq(type, "sign") || StrEq(type, "find")) return 2000;
    if (StrEq(type, "mash") || StrEq(type, "hold")) return 1500;
    return 900;
}

UI:mini(playerid, const a[])
{
    new token = CefFieldInt(a, 1), ok = CefFieldInt(a, 2);
    if (!gMiniToken[playerid] || token != gMiniToken[playerid]) return 1;
    gMiniToken[playerid] = 0;
    if (ok == 1 && GetTickCount() - gMiniStart[playerid] < Mini_MinMs(gMiniType[playerid])) ok = 0; // слишком быстро — подозрительно
    UI_Close(playerid, "mini");
    ClearAnimations(playerid);
    new ctx[24]; strcpy(ctx, gMiniCtx[playerid]);
    gMiniCtx[playerid][0] = 0;
    Mini_Done(playerid, ctx, ok == 1);
    return 1;
}

stock Mini_Cancel(playerid)
{
    gMiniToken[playerid] = 0; gMiniCtx[playerid][0] = 0;
}

// маршрутизация результата по префиксу контекста
stock Mini_Done(playerid, const ctx[], bool:ok)
{
    if (!strcmp(ctx, "q:", false, 2)) return Quest_OnMini(playerid, strval(ctx[2]), ok);
    if (!strcmp(ctx, "med:", false, 4)) return Med_OnMini(playerid, ctx[4], ok);
    if (!strcmp(ctx, "pass:", false, 5)) return Pass_OnMini(playerid, ctx[5], ok);
    if (StrEq(ctx, "job:farm"))
    {
        if (!ok) { Err(playerid, "Ящик рассыпался. Попробуйте ещё раз."); Farmer_Next(playerid); return 1; }
        P[playerid][pCarry] = true;
        SetPlayerAttachedObject(playerid, 5, 2900, 5, 0.1, 0.1, 0.2, -100.0, 0.0, 0.0, 0.35, 0.35, 0.35);
        SetPlayerSpecialAction(playerid, SPECIAL_ACTION_CARRY);
        new Float:x, Float:y, Float:z; Loc_Get("job_farmer", x, y, z);
        SetPlayerCheckpoint(playerid, x, y, z, 2.0); P[playerid][pCP] = CP_JOB; P[playerid][pJobStep] = 1;
        return Info(playerid, "Ящик полон! Отнесите его в амбар (метка).");
    }
    if (StrEq(ctx, "job:elec"))
    {
        if (!ok)
        {
            Err(playerid, "Искра! Вас слегка ударило током. Попробуйте ещё раз.");
            new Float:hp; GetPlayerHealth(playerid, hp); if (hp > 20.0) SetPlayerHealth(playerid, hp - 5.0);
            new k = GetPVarInt(playerid, "elSpot");
            SetPlayerCheckpoint(playerid, gElecSpots[k][0], gElecSpots[k][1], gElecSpots[k][2], 2.5); P[playerid][pCP] = CP_JOB;
            return 1;
        }
        new pay = 60 + random(25);
        if (gettime() - GetPVarInt(playerid, "elStart") < 150) { pay += 20; Info(playerid, "Быстрый выезд: +$20 премии."); }
        Job_Count(playerid, pay);
        Quest_Event(playerid, "electric", "*");
        Electric_Next(playerid);
        return 1;
    }
    return 1;
}
