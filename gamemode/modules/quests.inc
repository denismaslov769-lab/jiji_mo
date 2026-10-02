// =====================================================================
//  Квестовый движок. Шаги, условия и награды хранятся в таблице
//  quest_steps — новые квесты добавляются без правки кода.
//  Типы шагов: event (key=событие, arg=параметр или *), item (key=предмет),
//              bank (сумма на счёте), cash (наличные), level (уровень)
// =====================================================================
#define MAX_QSTEPS 128
#define MAX_PQ 4
enum E_QSTEP { qsQuest, qsStep, qsChapter, qsPath[8], qsType[8], qsKey[16], qsArg[16], qsTarget, qsTitle[64], qsHint[128], qsLoc[24], qsMoney, qsExp }
new gQS[MAX_QSTEPS][E_QSTEP], gQSCount;
enum E_QINFO { qiId, qiTitle[48], qiNpc[24] }
new gQI[8][E_QINFO], gQICount;

enum E_PQ { pqQuest, pqStep, pqProgress, pqStatus, pqPath[8] } // status: 0 активен, 1 отложен, 2 выполнен
new gPQ[MAX_PLAYERS][MAX_PQ][E_PQ];

stock Quest_LoadAll()
{
    gQSCount = 0; gQICount = 0;
    new DBResult:r = DB_Query("SELECT * FROM quest_steps ORDER BY quest, step");
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
    r = DB_Query("SELECT * FROM quests");
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n && gQICount < 8; i++)
        {
            gQI[gQICount][qiId] = DB_GetInt(r, "id");
            DB_GetStr(r, "title", gQI[gQICount][qiTitle], 48);
            DB_GetStr(r, "npc", gQI[gQICount][qiNpc], 24);
            gQICount++;
            db_next_row(r);
        }
        db_free_result(r);
    }
    printf("[QUEST] Квестов: %d, шагов: %d", gQICount, gQSCount);
}

stock Quest_Title(quest)
{
    new t[48]; t = "Квест";
    for (new i = 0; i < gQICount; i++) if (gQI[i][qiId] == quest) strcpy(t, gQI[i][qiTitle]);
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
        Quest_CheckItems(playerid);
        Quest_UpdateTracker(playerid);
        return 1;
    }
    return 0;
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
    Quest_UpdateTracker(playerid);
    Info(playerid, paused ? ("Квест отложен. Вернуться к нему можно в любой момент: /quest") : ("Квест продолжен."));
}

// Событие от игровых систем
stock Quest_Event(playerid, const ev[], const arg[] = "*", amount = 1)
{
    if (!P[playerid][pLogged]) return;
    for (new k = 0; k < MAX_PQ; k++)
    {
        if (!gPQ[playerid][k][pqQuest] || gPQ[playerid][k][pqStatus] != 0) continue;
        new si = Quest_StepIdx(gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep]);
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
        if (!gPQ[playerid][k][pqQuest] || gPQ[playerid][k][pqStatus] != 0) continue;
        new si = Quest_StepIdx(gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep]);
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
        if (gQS[si][qsExp])
        {
            // опыт квеста — в «очках», 20 очков = 1 ед. опыта уровня
            Lvl_AddExp(playerid, gQS[si][qsExp] / 20 + 1);
        }
        new s[128]; format(s, sizeof s, "Задание выполнено: %s", gQS[si][qsTitle]);
        Ok(playerid, s);
        PlayerPlaySound(playerid, 1083, 0.0, 0.0, 0.0);
    }
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
        Quest_OnComplete(playerid, quest);
        Quest_UpdateTracker(playerid);
        return;
    }
    gPQ[playerid][k][pqStep] = next;
    Quest_Save(playerid, k);
    new ni = Quest_StepIdx(quest, next);
    if (ni != -1 && gQS[ni][qsChapter] != oldChapter)
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
}

stock Quest_UpdateTracker(playerid)
{
    RemovePlayerMapIcon(playerid, 99);
    new j[512], shown = 0;
    for (new k = 0; k < MAX_PQ && !shown; k++)
    {
        if (!gPQ[playerid][k][pqQuest] || gPQ[playerid][k][pqStatus] != 0) continue;
        new si = Quest_StepIdx(gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep]);
        if (si == -1) continue;
        format(j, sizeof j, "{\"active\":true,\"quest\":\"%s\",\"chapter\":%d,\"chapterName\":\"%s\",\"title\":\"%s\",\"hint\":\"%s\",\"progress\":%d,\"target\":%d}",
            JsonEsc(Quest_Title(gPQ[playerid][k][pqQuest])), gQS[si][qsChapter], Quest_ChapterName(gQS[si][qsChapter]),
            JsonEsc(gQS[si][qsTitle]), JsonEsc(gQS[si][qsHint]), gPQ[playerid][k][pqProgress], gQS[si][qsTarget]);
        if (gQS[si][qsLoc][0])
        {
            new Float:x, Float:y, Float:z, Float:a, it, vw;
            if (Loc_Get(gQS[si][qsLoc], x, y, z, a, it, vw))
            {
                if (vw != 0)
                {
                    // точка внутри интерьера — метку ставим на входе в здание
                    if (StrEq(gQS[si][qsLoc], "cityhall_queue") || StrEq(gQS[si][qsLoc], "cityhall_window") || StrEq(gQS[si][qsLoc], "cityhall_hr")) Loc_Get("cityhall_door", x, y, z);
                }
                SetPlayerMapIcon(playerid, 99, x, y, z, 41, 0, MAPICON_GLOBAL);
            }
        }
        shown = 1;
    }
    if (!shown) strcpy(j, "{\"active\":false}");
    UI_Data(playerid, "quest", j);
}

CMD:quest(playerid, const params[])
{
    Menu_Begin("Квесты", "Ваши истории");
    for (new k = 0; k < MAX_PQ; k++)
    {
        if (!gPQ[playerid][k][pqQuest]) continue;
        new si = Quest_StepIdx(gPQ[playerid][k][pqQuest], gPQ[playerid][k][pqStep]);
        new d[128];
        if (gPQ[playerid][k][pqStatus] == 2) d = "Пройден";
        else if (si != -1) format(d, sizeof d, "%s%s", gPQ[playerid][k][pqStatus] == 1 ? ("[Отложен] ") : (""), gQS[si][qsTitle]);
        Menu_Add(gPQ[playerid][k][pqQuest], Quest_Title(gPQ[playerid][k][pqQuest]), d, -1, gPQ[playerid][k][pqStatus] == 1 ? ("pause") : ("quest"));
    }
    if (!gMBCount) Menu_Add(0, "Нет квестов", "Поговорите с Дядей Вовой у ночлежки");
    Menu_Send(playerid, "quests");
    return 1;
}

MENU:quests(playerid, item, const extra[])
{
    new k = Quest_Slot(playerid, item);
    if (k == -1) return UI_Close(playerid);
    if (gPQ[playerid][k][pqStatus] == 2) return UI_Close(playerid);
    Quest_SetPaused(playerid, item, gPQ[playerid][k][pqStatus] == 0);
    UI_Close(playerid);
    return 1;
}
