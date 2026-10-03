// =====================================================================
//  Квестовый движок. Квесты, шаги, условия, NPC и награды хранятся в БД
//  (quests, quest_steps, npcs) — новые истории добавляются без правки кода.
//  Типы шагов:
//    event (key=событие, arg=параметр или *), item (key=предмет),
//    bank / cash (сумма), level (уровень),
//    talk  (key=код NPC, реплика NPC — колонка say),
//    give  (key=код NPC, arg=предмет, target=кол-во — отдать NPC),
//    goto  (loc=точка — дойти/доехать до метки),
//    mini  (key=тип мини-игры, arg=сложность, loc=место — пройти мини-игру).
// =====================================================================
#define MAX_QSTEPS 420
#define MAX_QUESTS 40
#define MAX_PQ 40
enum E_QSTEP { qsQuest, qsStep, qsChapter, qsPath[8], qsType[8], qsKey[16], qsArg[16], qsTarget, qsTitle[64], qsHint[128], qsLoc[24], qsMoney, qsExp }
new gQS[MAX_QSTEPS][E_QSTEP], gQSCount;
enum E_QINFO { qiId, qiTitle[48], qiNpc[24], qiGiver[24], qiReq, qiReqLvl, qiCity[20] }
new gQI[MAX_QUESTS][E_QINFO], gQICount;

enum E_PQ { pqQuest, pqStep, pqProgress, pqStatus, pqPath[8] } // status: 0 активен, 1 отложен, 2 выполнен
new gPQ[MAX_PLAYERS][MAX_PQ][E_PQ];

stock Quest_LoadAll()
{
    gQSCount = 0; gQICount = 0;
    new DBResult:r = DB_Query("SELECT quest,step,chapter,path,type,key,arg,target,title,hint,loc,r_money,r_exp FROM quest_steps ORDER BY quest, step");
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n && gQSCount < MAX_QSTEPS; i++)
        {
            new k = gQSCount++;
            gQS[k][qsQuest] = DB_GetInt(r, "quest"); gQS[k][qsStep] = DB_GetInt(r, "step");
            gQS[k][qsChapter] = DB_GetInt(r, "chapter");
            DB_GetStr(r, "path", gQS[k][qsPath], 8); DB_GetStr(r, "type", gQS[k][qsType], 8);
            DB_GetStr(r, "key", gQS[k][qsKey], 16); DB_GetStr(r, "arg", gQS[k][qsArg], 16);
            gQS[k][qsTarget] = DB_GetInt(r, "target");
            DB_GetStr(r, "title", gQS[k][qsTitle], 64); DB_GetStr(r, "hint", gQS[k][qsHint], 128);
            DB_GetStr(r, "loc", gQS[k][qsLoc], 24);
            gQS[k][qsMoney] = DB_GetInt(r, "r_money"); gQS[k][qsExp] = DB_GetInt(r, "r_exp");
            db_next_row(r);
        }
        db_free_result(r);
    }
    r = DB_Query("SELECT id,title,npc,giver,req_quest,req_level,city FROM quests ORDER BY id");
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n && gQICount < MAX_QUESTS; i++)
        {
            new k = gQICount++;
            gQI[k][qiId] = DB_GetInt(r, "id");
            DB_GetStr(r, "title", gQI[k][qiTitle], 48);
            DB_GetStr(r, "npc", gQI[k][qiNpc], 24);
            DB_GetStr(r, "giver", gQI[k][qiGiver], 24);
            gQI[k][qiReq] = DB_GetInt(r, "req_quest");
            gQI[k][qiReqLvl] = DB_GetInt(r, "req_level");
            DB_GetStr(r, "city", gQI[k][qiCity], 20);
            db_next_row(r);
        }
        db_free_result(r);
    }
    printf("[QUEST] Квестов: %d, шагов: %d", gQICount, gQSCount);
}

stock Quest_Info(quest)
{
    for (new i = 0; i < gQICount; i++) if (gQI[i][qiId] == quest) return i;
    return -1;
}

stock Quest_Title(quest)
{
    new t[48]; t = "Квест";
    new i = Quest_Info(quest);
    if (i != -1) strcpy(t, gQI[i][qiTitle]);
    return t;
}

stock Quest_StepIdx(quest, step)
{
    for (new i = 0; i < gQSCount; i++) if (gQS[i][qsQuest] == quest && gQS[i][qsStep] == step) return i;
    return -1;
}

stock Quest_MaxStep(quest)
{
    new m = 0;
    for (new i = 0; i < gQSCount; i++) if (gQS[i][qsQuest] == quest && gQS[i][qsStep] > m) m = gQS[i][qsStep];
    return m;
}

stock Quest_Load(playerid)
{
    for (new k = 0; k < MAX_PQ; k++) gPQ[playerid][k][pqQuest] = 0;
    new q[96];
    format(q, sizeof q, "SELECT * FROM player_quests WHERE acc=%d", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    if (!r) return;
    for (new i = 0, n = db_num_rows(r); i < n && i < MAX_PQ; i++)
    {
        gPQ[playerid][i][pqQuest] = DB_GetInt(r, "quest");
        gPQ[playerid][i][pqStep] = DB_GetInt(r, "step");
        gPQ[playerid][i][pqProgress] = DB_GetInt(r, "progress");
        gPQ[playerid][i][pqStatus] = DB_GetInt(r, "status");
        DB_GetStr(r, "path", gPQ[playerid][i][pqPath], 8);
        db_next_row(r);
    }
    db_free_result(r);
}

stock Quest_Save(playerid, k)
{
    new q[200];
    format(q, sizeof q, "INSERT OR REPLACE INTO player_quests VALUES (%d,%d,%d,%d,%d,'%s')", P[playerid][pID],
        gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep], gPQ[playerid][k][pqProgress], gPQ[playerid][k][pqStatus], gPQ[playerid][k][pqPath]);
    DB_Exec(q);
}

stock Quest_Slot(playerid, quest)
{
    for (new k = 0; k < MAX_PQ; k++) if (gPQ[playerid][k][pqQuest] == quest) return k;
    return -1;
}

stock Quest_GetStep(playerid, quest)
{
    new k = Quest_Slot(playerid, quest);
    if (k == -1) return 0;
    if (gPQ[playerid][k][pqStatus] == 2) return 999;
    return gPQ[playerid][k][pqStep];
}

stock bool:Quest_IsActive(playerid, quest)
{
    new k = Quest_Slot(playerid, quest);
    return k != -1 && gPQ[playerid][k][pqStatus] == 0;
}

stock bool:Quest_IsDone(playerid, quest) return Quest_GetStep(playerid, quest) == 999;

stock bool:Quest_Available(playerid, qi)
{
    new quest = gQI[qi][qiId];
    if (!gQI[qi][qiGiver][0] || Quest_Slot(playerid, quest) != -1) return false;
    if (gQI[qi][qiReq] && !Quest_IsDone(playerid, gQI[qi][qiReq])) return false;
    return P[playerid][pLevel] >= gQI[qi][qiReqLvl];
}

stock Quest_Start(playerid, quest)
{
    if (Quest_Slot(playerid, quest) != -1) return 0;
    for (new k = 0; k < MAX_PQ; k++)
    {
        if (gPQ[playerid][k][pqQuest]) continue;
        gPQ[playerid][k][pqQuest] = quest; gPQ[playerid][k][pqStep] = 1; gPQ[playerid][k][pqProgress] = 0;
        gPQ[playerid][k][pqStatus] = 0; gPQ[playerid][k][pqPath][0] = 0;
        Quest_Save(playerid, k);
        new s[96]; format(s, sizeof s, "Начат квест «%s»", Quest_Title(quest));
        Ok(playerid, s);
        SetPVarInt(playerid, "qTrack", quest);
        PlayerPlaySound(playerid, 1057, 0.0, 0.0, 0.0);
        Quest_CheckItems(playerid);
        Quest_UpdateTracker(playerid);
        return 1;
    }
    return Err(playerid, "Слишком много квестов.");
}

stock Quest_SetPath(playerid, quest, const path[])
{
    new k = Quest_Slot(playerid, quest);
    if (k == -1) return;
    strcpy(gPQ[playerid][k][pqPath], path, 8);
    Quest_Save(playerid, k);
}

stock Quest_SetPaused(playerid, quest, bool:paused)
{
    new k = Quest_Slot(playerid, quest);
    if (k == -1 || gPQ[playerid][k][pqStatus] == 2) return;
    gPQ[playerid][k][pqStatus] = paused ? 1 : 0;
    Quest_Save(playerid, k);
    if (!paused) SetPVarInt(playerid, "qTrack", quest);
    Quest_UpdateTracker(playerid);
    Info(playerid, paused ? ("Квест отложен. Вернуться к нему можно в любой момент: /quest") : ("Квест продолжен."));
}

// текущий шаг активного квеста (индекс в gQS) или -1
stock Quest_CurIdx(playerid, k)
{
    if (!gPQ[playerid][k][pqQuest] || gPQ[playerid][k][pqStatus] != 0) return -1;
    return Quest_StepIdx(gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep]);
}

// Событие от игровых систем
stock Quest_Event(playerid, const ev[], const arg[] = "*", amount = 1)
{
    if (!P[playerid][pLogged]) return;
    for (new k = 0; k < MAX_PQ; k++)
    {
        new si = Quest_CurIdx(playerid, k);
        if (si == -1) continue;
        if (!StrEq(gQS[si][qsType], "event") || !StrEq(gQS[si][qsKey], ev)) continue;
        if (!StrEq(gQS[si][qsArg], "*") && !StrEq(gQS[si][qsArg], arg)) continue;
        gPQ[playerid][k][pqProgress] += amount;
        if (gPQ[playerid][k][pqProgress] >= gQS[si][qsTarget]) Quest_Advance(playerid, k);
        else { Quest_Save(playerid, k); Quest_UpdateTracker(playerid); }
    }
}

// Проверка шагов-состояний (предметы, деньги, уровень)
stock Quest_CheckItems(playerid)
{
    if (!P[playerid][pLogged]) return;
    for (new k = 0; k < MAX_PQ; k++)
    {
        new si = Quest_CurIdx(playerid, k);
        if (si == -1) continue;
        new have = -1;
        if (StrEq(gQS[si][qsType], "item")) have = Inv_Count(playerid, Item_ByKey(gQS[si][qsKey]));
        else if (StrEq(gQS[si][qsType], "bank")) have = P[playerid][pBank];
        else if (StrEq(gQS[si][qsType], "cash")) have = P[playerid][pCash];
        else if (StrEq(gQS[si][qsType], "level")) have = P[playerid][pLevel];
        if (have == -1) continue;
        new old = gPQ[playerid][k][pqProgress];
        gPQ[playerid][k][pqProgress] = (have > gQS[si][qsTarget]) ? gQS[si][qsTarget] : have;
        if (have >= gQS[si][qsTarget]) Quest_Advance(playerid, k);
        else if (old != gPQ[playerid][k][pqProgress]) Quest_UpdateTracker(playerid);
    }
}

stock Quest_Advance(playerid, k)
{
    new quest = gPQ[playerid][k][pqQuest];
    new si = Quest_StepIdx(quest, gPQ[playerid][k][pqStep]);
    if (si != -1)
    {
        if (gQS[si][qsMoney]) Money_Give(playerid, gQS[si][qsMoney], "quest");
        if (gQS[si][qsExp]) Lvl_AddExp(playerid, gQS[si][qsExp] / 20 + 1); // 20 очков = 1 ед. опыта уровня
        new s[128]; format(s, sizeof s, "Задание выполнено: %s", gQS[si][qsTitle]);
        Ok(playerid, s);
        PlayerPlaySound(playerid, 1083, 0.0, 0.0, 0.0);
    }
    if (P[playerid][pCP] == CP_QUEST) { DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE; }
    new oldChapter = (si != -1) ? gQS[si][qsChapter] : 0;
    new maxs = Quest_MaxStep(quest), next = gPQ[playerid][k][pqStep] + 1;
    // пропуск шагов чужого пути
    while (next <= maxs)
    {
        new ni = Quest_StepIdx(quest, next);
        if (ni == -1) { next++; continue; }
        if (gQS[ni][qsPath][0] && !StrEq(gQS[ni][qsPath], gPQ[playerid][k][pqPath])) { next++; continue; }
        break;
    }
    gPQ[playerid][k][pqProgress] = 0;
    if (next > maxs)
    {
        gPQ[playerid][k][pqStatus] = 2;
        gPQ[playerid][k][pqStep] = maxs;
        Quest_Save(playerid, k);
        if (GetPVarInt(playerid, "qTrack") == quest) DeletePVar(playerid, "qTrack");
        Quest_OnComplete(playerid, quest);
        Quest_UpdateTracker(playerid);
        return;
    }
    gPQ[playerid][k][pqStep] = next;
    Quest_Save(playerid, k);
    new ni = Quest_StepIdx(quest, next);
    if (quest == 1 && ni != -1 && gQS[ni][qsChapter] != oldChapter)
    {
        new j[160]; format(j, sizeof j, "{\"done\":%d,\"next\":%d,\"title\":\"%s\"}", oldChapter, gQS[ni][qsChapter], JsonEsc(Quest_ChapterName(gQS[ni][qsChapter])));
        UI_Data(playerid, "chapter", j);
    }
    Quest_UpdateTracker(playerid);
    Quest_CheckItems(playerid);
}

stock Quest_ChapterName(ch)
{
    new t[32];
    switch (ch)
    {
        case 1: t = "Выживание";
        case 2: t = "Голод";
        case 3: t = "Документы";
        case 4: t = "Медкомиссия";
        case 5: t = "Первая работа";
        case 6: t = "Связь";
        case 7: t = "Права";
        case 8: t = "Крыша над головой";
        case 9: t = "Выбор пути";
        case 10: t = "Финал";
        default: t = "";
    }
    return t;
}

stock Quest_OnComplete(playerid, quest)
{
    new s[96]; format(s, sizeof s, "Квест «%s» пройден!", Quest_Title(quest));
    Ok(playerid, s);
    if (quest == 1) Ach_Give(playerid, "bottom", "С самого дна");
    if (quest == 26) Ach_Give(playerid, "debt", "Долг платежом красен");
    new done = 0;
    for (new k = 0; k < MAX_PQ; k++) if (gPQ[playerid][k][pqQuest] && gPQ[playerid][k][pqStatus] == 2) done++;
    if (done == 10) Ach_Give(playerid, "q10", "Свой человек в штате");
    if (done == 25) Ach_Give(playerid, "q25", "Легенда Сан-Андреаса");
    new avail = 0;
    for (new i = 0; i < gQICount; i++) if (Quest_Available(playerid, i)) avail++;
    if (avail) { format(s, sizeof s, "Доступно новых историй: %d. Знаки «?» на карте, список — /quest.", avail); Info(playerid, s); }
}

// ---------- трекер, метки, чекпоинт цели ----------
stock Quest_Tracked(playerid)
{
    new t = GetPVarInt(playerid, "qTrack");
    if (t)
    {
        new k = Quest_Slot(playerid, t);
        if (k != -1 && gPQ[playerid][k][pqStatus] == 0) return k;
    }
    for (new k = 0; k < MAX_PQ; k++) if (gPQ[playerid][k][pqQuest] && gPQ[playerid][k][pqStatus] == 0 && Quest_CurIdx(playerid, k) != -1) return k;
    return -1;
}

stock bool:Quest_StepPos(si, &Float:x, &Float:y, &Float:z, &vw)
{
    new code[24], Float:a, it;
    if (gQS[si][qsLoc][0]) strcpy(code, gQS[si][qsLoc]);
    else if (StrEq(gQS[si][qsType], "talk") || StrEq(gQS[si][qsType], "give")) { if (!Npc_Loc(gQS[si][qsKey], code)) return false; }
    else return false;
    if (!Loc_Get(code, x, y, z, a, it, vw)) return false;
    return true;
}

stock Quest_UpdateTracker(playerid)
{
    RemovePlayerMapIcon(playerid, 99);
    new j[512], k = Quest_Tracked(playerid);
    new si = (k == -1) ? -1 : Quest_CurIdx(playerid, k);
    if (si == -1)
    {
        strcpy(j, "{\"active\":false}");
        if (P[playerid][pCP] == CP_QUEST) { DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE; }
    }
    else
    {
        new quest = gPQ[playerid][k][pqQuest], qi = Quest_Info(quest), chname[32];
        if (quest == 1) strcpy(chname, Quest_ChapterName(gQS[si][qsChapter]));
        else if (qi != -1) strcpy(chname, gQI[qi][qiCity]);
        format(j, sizeof j, "{\"active\":true,\"quest\":\"%s\",\"chapter\":%d,\"chapterName\":\"%s\",\"title\":\"%s\",\"hint\":\"%s\",\"progress\":%d,\"target\":%d}",
            JsonEsc(Quest_Title(quest)), quest == 1 ? gQS[si][qsChapter] : 0, JsonEsc(chname),
            JsonEsc(gQS[si][qsTitle]), JsonEsc(gQS[si][qsHint]), gPQ[playerid][k][pqProgress], gQS[si][qsTarget]);
        new Float:x, Float:y, Float:z, vw;
        if (Quest_StepPos(si, x, y, z, vw))
        {
            new Float:ix = x, Float:iy = y, Float:iz = z;
            // точка внутри интерьера — метку ставим на входе в здание
            if (vw == 1) Loc_Get("cityhall_door", ix, iy, iz);
            else if (vw == HOSP_VW) Loc_Get("hospital", ix, iy, iz);
            SetPlayerMapIcon(playerid, 99, ix, iy, iz, 41, 0, MAPICON_GLOBAL);
            new bool:cpStep = StrEq(gQS[si][qsType], "goto") || StrEq(gQS[si][qsType], "mini");
            if (cpStep && (P[playerid][pCP] == CP_NONE || P[playerid][pCP] == CP_QUEST) && (vw == 0 || vw == GetPlayerVirtualWorld(playerid)))
            {
                SetPlayerCheckpoint(playerid, x, y, z, 3.0);
                P[playerid][pCP] = CP_QUEST;
            }
            else if (!cpStep && P[playerid][pCP] == CP_QUEST) { DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE; }
        }
    }
    UI_Data(playerid, "quest", j);
    Quest_GiverIcons(playerid);
}

// знаки «?» у NPC, у которых есть доступные истории
stock Quest_GiverIcons(playerid)
{
    for (new i = 0; i < gQICount && i < 30; i++)
    {
        RemovePlayerMapIcon(playerid, 60 + i);
        if (!Quest_Available(playerid, i)) continue;
        new code[24], Float:x, Float:y, Float:z;
        if (!Npc_Loc(gQI[i][qiGiver], code) || !Loc_Get(code, x, y, z)) continue;
        SetPlayerMapIcon(playerid, 60 + i, x, y, z, 37, 0, MAPICON_LOCAL);
    }
}

// игрок вошёл в чекпоинт квеста
stock Quest_OnCP(playerid)
{
    new k = Quest_Tracked(playerid);
    new si = (k == -1) ? -1 : Quest_CurIdx(playerid, k);
    if (si == -1) { DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE; return 1; }
    if (StrEq(gQS[si][qsType], "goto")) { Quest_Advance(playerid, k); return 1; }
    if (StrEq(gQS[si][qsType], "mini"))
    {
        if (IsPlayerInAnyVehicle(playerid)) return Info(playerid, "Выйдите из транспорта, чтобы выполнить задание.");
        DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE;
        new ctx[24]; format(ctx, sizeof ctx, "q:%d", gPQ[playerid][k][pqQuest]);
        new lvl = strval(gQS[si][qsArg]);
        Mini_Start(playerid, gQS[si][qsKey], lvl ? lvl : 1, ctx, gQS[si][qsTitle], gQS[si][qsHint]);
        return 1;
    }
    return 1;
}

stock Quest_OnMini(playerid, quest, bool:ok)
{
    new k = Quest_Slot(playerid, quest);
    if (k == -1) return 1;
    new si = Quest_CurIdx(playerid, k);
    if (si == -1 || !StrEq(gQS[si][qsType], "mini")) return 1;
    if (ok) Quest_Advance(playerid, k);
    else { Err(playerid, "Не получилось. Попробуйте ещё раз — зайдите в метку снова."); SetTimerEx("Quest_RetryCP", 2500, false, "i", playerid); }
    return 1;
}

forward Quest_RetryCP(playerid);
public Quest_RetryCP(playerid) if (IsLogged(playerid)) Quest_UpdateTracker(playerid);

// ---------- разговоры с NPC по квестам ----------
stock Quest_StepSay(quest, step, out[], size = sizeof out)
{
    new q[96]; format(q, sizeof q, "SELECT say FROM quest_steps WHERE quest=%d AND step=%d", quest, step);
    out[0] = 0;
    new DBResult:r = DB_Query(q);
    if (!r) return;
    if (db_num_rows(r)) DB_GetStr(r, "say", out, size);
    db_free_result(r);
}

// возвращает 1, если у NPC был квестовый разговор
stock Quest_NpcTalk(playerid, n)
{
    for (new k = 0; k < MAX_PQ; k++)
    {
        new si = Quest_CurIdx(playerid, k);
        if (si == -1) continue;
        new bool:talk = StrEq(gQS[si][qsType], "talk"), bool:give = StrEq(gQS[si][qsType], "give");
        if ((!talk && !give) || !StrEq(gQS[si][qsKey], gNpc[n][nCode])) continue;
        new say[400];
        Quest_StepSay(gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep], say);
        if (!say[0]) strcpy(say, gQS[si][qsHint]);
        if (give)
        {
            new it = Item_ByKey(gQS[si][qsArg]);
            if (Inv_Count(playerid, it) < gQS[si][qsTarget])
            {
                new t[200]; format(t, sizeof t, "Ну что, принёс? Мне нужно: %s (%d шт.). Возвращайся, когда будет.", gQS[si][qsTitle], gQS[si][qsTarget]);
                Npc_Dialog(playerid, n, 100, t, "Скоро принесу");
                return 1;
            }
            Npc_Dialog(playerid, n, 200 + k, say, "Отдать");
            return 1;
        }
        Npc_Dialog(playerid, n, 200 + k, say, "Хорошо", "Расскажи ещё раз");
        return 1;
    }
    return 0;
}

stock Quest_OfferFrom(playerid, n)
{
    for (new i = 0; i < gQICount; i++)
    {
        if (!StrEq(gQI[i][qiGiver], gNpc[n][nCode]) || !Quest_Available(playerid, i)) continue;
        new q[96], descr[400];
        format(q, sizeof q, "SELECT descr FROM quests WHERE id=%d", gQI[i][qiId]);
        new DBResult:r = DB_Query(q);
        descr[0] = 0;
        if (r) { if (db_num_rows(r)) DB_GetStr(r, "descr", descr); db_free_result(r); }
        if (!descr[0]) strcpy(descr, "Есть одно дело. Поможешь?");
        Npc_Dialog(playerid, n, 300 + i, descr, "Взяться за дело", "Не сейчас");
        return 1;
    }
    // квест у этого NPC есть, но пока недоступен
    for (new i = 0; i < gQICount; i++)
    {
        if (!StrEq(gQI[i][qiGiver], gNpc[n][nCode]) || Quest_Slot(playerid, gQI[i][qiId]) != -1) continue;
        if (P[playerid][pLevel] < gQI[i][qiReqLvl])
        {
            new t[160]; format(t, sizeof t, "Для тебя есть работа, но ты пока зелёный. Приходи, когда будет %d уровень.", gQI[i][qiReqLvl]);
            Npc_Dialog(playerid, n, 100, t, "Понял");
            return 1;
        }
    }
    return 0;
}

stock Quest_DlgResponse(playerid, node, opt)
{
    if (node >= 300)
    {
        new i = node - 300;
        UI_Close(playerid);
        if (i < 0 || i >= gQICount || opt != 1 || !Quest_Available(playerid, i)) return 1;
        Quest_Start(playerid, gQI[i][qiId]);
        // первый шаг обычно — разговор с тем же NPC: засчитываем сразу
        new k = Quest_Slot(playerid, gQI[i][qiId]), si = Quest_CurIdx(playerid, k);
        if (si != -1 && StrEq(gQS[si][qsType], "talk") && StrEq(gQS[si][qsKey], gQI[i][qiGiver])) Quest_Advance(playerid, k);
        return 1;
    }
    new k = node - 200;
    if (k < 0 || k >= MAX_PQ) return UI_Close(playerid);
    new si = Quest_CurIdx(playerid, k);
    if (si == -1) return UI_Close(playerid);
    if (opt == 2)
    {   // повторить — показываем подсказку
        new t[200]; format(t, sizeof t, "Ещё раз: %s. %s", gQS[si][qsTitle], gQS[si][qsHint]);
        new n = Npc_Find(gQS[si][qsKey]);
        if (n != -1) return Npc_Dialog(playerid, n, node, t, "Понял");
    }
    UI_Close(playerid);
    if (StrEq(gQS[si][qsType], "give"))
    {
        new it = Item_ByKey(gQS[si][qsArg]);
        if (Inv_Count(playerid, it) < gQS[si][qsTarget]) return Err(playerid, "У вас нет нужных предметов.");
        Inv_Remove(playerid, it, gQS[si][qsTarget]);
        ApplyAnimation(playerid, "DEALER", "DEALER_DEAL", 4.0, 0, 0, 0, 0, 0);
    }
    Quest_Advance(playerid, k);
    return 1;
}

// ---------- /quest ----------
CMD:quest(playerid, const params[])
{
    Menu_Begin("Квесты", "Ваши истории · выберите, чтобы отслеживать или отложить");
    new done = 0, tr = Quest_Tracked(playerid);
    for (new k = 0; k < MAX_PQ; k++)
    {
        if (!gPQ[playerid][k][pqQuest]) continue;
        if (gPQ[playerid][k][pqStatus] == 2) { done++; continue; }
        new si = Quest_StepIdx(gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep]);
        new d[128];
        if (si != -1) format(d, sizeof d, "%s%s%s", k == tr ? ("[Отслеживается] ") : (""), gPQ[playerid][k][pqStatus] == 1 ? ("[Отложен] ") : (""), gQS[si][qsTitle]);
        Menu_Add(gPQ[playerid][k][pqQuest], Quest_Title(gPQ[playerid][k][pqQuest]), d, -1, gPQ[playerid][k][pqStatus] == 1 ? ("pause") : (k == tr ? ("target") : ("star")));
    }
    for (new i = 0; i < gQICount; i++)
    {
        if (!Quest_Available(playerid, i)) continue;
        new d[96], name[32]; Npc_NameByCode(gQI[i][qiGiver], name);
        format(d, sizeof d, "Новая история · %s · %s", gQI[i][qiCity], name);
        Menu_Add(500 + i, gQI[i][qiTitle], d, -1, "help");
    }
    if (done) { new d[64]; format(d, sizeof d, "Пройдено историй: %d из %d", done, gQICount); Menu_Add(0, "Архив", d, -1, "trophy"); }
    if (!gMBCount) Menu_Add(0, "Нет квестов", "Поговорите с Дядей Вовой у ночлежки");
    Menu_Send(playerid, "quests");
    return 1;
}

MENU:quests(playerid, item, const extra[])
{
    if (item >= 500)
    {
        UI_Close(playerid);
        new i = item - 500, code[24], Float:x, Float:y, Float:z;
        if (i >= gQICount || !Npc_Loc(gQI[i][qiGiver], code) || !Loc_Get(code, x, y, z)) return 1;
        if (P[playerid][pCP] == CP_JOB) return Err(playerid, "Сначала закончите рабочее задание.");
        SetPlayerCheckpoint(playerid, x, y, z, 3.0); P[playerid][pCP] = CP_GPS;
        return Ok(playerid, "Персонаж отмечен на карте — поговорите с ним (ALT).");
    }
    new k = Quest_Slot(playerid, item);
    if (k == -1 || gPQ[playerid][k][pqStatus] == 2) return UI_Close(playerid);
    SetPVarInt(playerid, "qMenu", item);
    Menu_Begin(Quest_Title(item), "Что сделать с квестом?");
    Menu_Add(1, "Отслеживать", "Показывать цель в трекере и на карте", -1, "target");
    Menu_Add(2, gPQ[playerid][k][pqStatus] == 1 ? ("Продолжить") : ("Отложить"), "Можно вернуться в любой момент", -1, "pause");
    Menu_Send(playerid, "quest_do");
    return 1;
}

MENU:quest_do(playerid, item, const extra[])
{
    UI_Close(playerid);
    new quest = GetPVarInt(playerid, "qMenu"), k = Quest_Slot(playerid, quest);
    if (k == -1) return 1;
    if (item == 1)
    {
        if (gPQ[playerid][k][pqStatus] == 1) Quest_SetPaused(playerid, quest, false);
        SetPVarInt(playerid, "qTrack", quest);
        Quest_UpdateTracker(playerid);
        return Ok(playerid, "Квест отслеживается.");
    }
    Quest_SetPaused(playerid, quest, gPQ[playerid][k][pqStatus] == 0);
    return 1;
}
