// =====================================================================
//  Godjo RP — godjo_ai: «симуляция игроков». Жители — агенты с
//  характером, потребностями, расписанием, работой, памятью и
//  отношениями. Они заходят и выходят с сервера как люди, ходят по
//  пешеходному графу (A*), работают сменами, едят, общаются друг с
//  другом и разговаривают с игроками (намерение -> желание -> знания
//  -> стиль -> анти-повтор -> задержка «печати»).
//  Мозг: Utility AI с инерцией + HTN-lite план. Тело: FCNPC.
//  Хранение: SQLite (ai_agents, ai_memory, ai_relations, ai_sessions).
//  Кодировка исходника: UTF-8 (при сборке перекодируется в CP1251).
// =====================================================================
#define FILTERSCRIPT
#pragma warning disable 239
#pragma warning disable 214
#include <a_samp>
#include <FCNPC>
#include "bots_paths.inc"
#include "ai/navgraph.inc"
#include "ai/core.inc"
#include "ai/data.inc"
#include "ai/nav.inc"
#include "ai/agents.inc"
#include "ai/brain.inc"
#include "ai/body.inc"
#include "ai/social.inc"
#include "ai/admin.inc"

forward AI_Start();
public AI_Start()
{
    new f[16] = "godjo.db";
    gAIDB = db_open(f);
    if (!gAIDB) { print("[AI] Не удалось открыть базу — жители отключены."); return; }
    AI_Schema();
    Places_Init();
    new DBResult:r = db_query(gAIDB, "SELECT COUNT(*) FROM ai_agents"), have = 0;
    if (r) { have = db_get_field_int(r, 0); db_free_result(r); }
    if (have < AI_POP) AI_Generate(AI_POP - have);
    AI_LoadAll();
    gTimer = SetTimer("AI_Tick", AI_TICK_MS, true);
    printf("[AI] godjo_ai запущен: граф %d узлов, мест %d, онлайн до %d", NAV_NODES, sizeof gPlace, gMaxOnline);
}

stock bool:AI_PlayerNear(a, Float:r)
{
    new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsPlayerConnected(i) && !IsPlayerNPC(i) && IsPlayerInRangeOfPoint(i, r, x, y, z)) return true;
    return false;
}

forward AI_Tick();
public AI_Tick()
{
    gTick++;
    for (new c = 0; c < MAX_CONV; c++) if (C[c][cUsed]) Conv_Tick(c);
    for (new a = 0; a < gAgentCount; a++)
    {
        if (!A[a][aOnline]) continue;
        if (AI_IsDriver(a)) Drive_Tick(a); else Body_Tick(a);
    }
    // колесо времени: за такт думают не больше AI_THINK_BUDGET агентов
    new done = 0;
    for (new k = 0; k < gAgentCount && done < AI_THINK_BUDGET; k++)
    {
        new a = (gThinkCursor + k) % gAgentCount;
        if (!A[a][aOnline] || gTick < A[a][aThinkAt]) continue;
        done++;
        new now = gettime();
        if (A[a][aNeedsAt]) Brain_Needs(a, float(now - A[a][aNeedsAt]) / 60.0);
        A[a][aNeedsAt] = now;
        new bool:near = AI_PlayerNear(a, AI_LOD_NEAR);   // LOD0 — рядом игрок
        if (!AI_IsDriver(a))
        {
            if (A[a][aGoal] == G_NONE || A[a][aPlanPos] >= A[a][aPlanLen]) A[a][aGoal] = G_NONE;
            Brain_Think(a);
            if (near && Body_CurTask(a) != T_INSIDE) Social_GreetNear(a);
        }
        if (A[a][aDebug]) AI_DebugBubble(a);
        if (now >= A[a][aSaveAt]) { A[a][aSaveAt] = now + 300; AI_Save(a); }
        A[a][aThinkAt] = gTick + (near ? 8 : 32) + random(4);
        gThinkCursor = (a + 1) % gAgentCount;
    }
    if (gTick % 40 == 0) AI_SessionTick();
}

public FCNPC_OnReachDestination(npcid)
{
    new a = gAgentOf[npcid];
    if (a != -1 && A[a][aOnline]) Body_OnReach(a);
    return 1;
}

stock const gHurt[][] = {"Ай! Ты что творишь?!", "Помогите! Полиция!", "Эй, псих, отвали!", "Ты больной?!", "Отстань от меня!"};

public FCNPC_OnTakeDamage(npcid, issuerid, Float:amount, weaponid, bodypart)
{
    new a = gAgentOf[npcid];
    if (a == -1 || !A[a][aOnline]) return 1;
    if (issuerid != INVALID_PLAYER_ID && !IsPlayerNPC(issuerid))
    {
        new pname[MAX_PLAYER_NAME]; GetPlayerName(issuerid, pname, sizeof pname);
        if (Mem_Find(a, MEM_ATTACKED, pname, 60) == -1) { Mem_Add(a, MEM_ATTACKED, pname, weaponid, 8); Rel_Change(a, pname, -35, 1); }
        CallRemoteFunction("GM_BotAttacked", "ii", npcid, issuerid);
    }
    if (AI_IsDriver(a) || A[a][aGoal] == G_FLEE) return 1;
    Conv_End(a); A[a][aChatWith] = INVALID_PLAYER_ID;
    Body_Exit(a);
    AI_Say(a, gHurt[random(sizeof gHurt)]);
    // бегство: подальше от обидчика
    new Float:x, Float:y, Float:z; FCNPC_GetPosition(npcid, x, y, z);
    new from = Nav_Nearest(x, y, 150.0); if (from == -1) from = A[a][aHome];
    Plan_Clear(a);
    A[a][aGoal] = G_FLEE; A[a][aGoalSince] = gettime(); A[a][aGoalMin] = 60;
    Plan_Add(a, T_MOVE, Nav_RandomNear(from, 80.0, 160.0));
    Plan_Add(a, T_ANIM, 2);
    Body_StartTask(a);
    return 1;
}

forward AI_AfterDeath(a, id);
public AI_AfterDeath(a, id)
{
    if (a < 0 || a >= gAgentCount || A[a][aId] != id || !A[a][aOnline]) return;
    AI_Logout(a, "погиб");
    gNeed[a][N_HUNGER] = 60.0;
    A[a][aNextLogin] = gettime() + 60 * (20 + random(30));   // «лежал в больнице»
}

public FCNPC_OnDeath(npcid, killerid, reason)
{
    new a = gAgentOf[npcid];
    if (a == -1) return 1;
    if (killerid != INVALID_PLAYER_ID && !IsPlayerNPC(killerid))
    {
        new pname[MAX_PLAYER_NAME]; GetPlayerName(killerid, pname, sizeof pname);
        Mem_Add(a, MEM_ATTACKED, pname, reason, 10); Rel_Change(a, pname, -60, 1);
    }
    SetTimerEx("AI_AfterDeath", 5000, false, "ii", a, A[a][aId]);
    return 1;
}

public OnVehicleDeath(vehicleid, killerid)
{
    for (new a = 0; a < gAgentCount; a++) if (A[a][aOnline] && A[a][aVeh] == vehicleid) AI_Logout(a, "машина разбита");
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (IsPlayerNPC(playerid)) return 0;
    new cmd[24], i = 1, j = 0;
    while (cmdtext[i] > ' ' && j < sizeof cmd - 1) cmd[j++] = cmdtext[i++];
    cmd[j] = 0;
    while (cmdtext[i] == ' ') i++;
    if (cmd[0] != 'a' && cmd[0] != 'A') return 0;
    return AI_Cmd(playerid, cmd, cmdtext[i]);
}

public OnFilterScriptInit()
{
    for (new a = 0; a < AI_POP; a++) { A[a][aOnline] = false; A[a][aNpc] = INVALID_PLAYER_ID; A[a][aConv] = -1; A[a][aVeh] = INVALID_VEHICLE_ID; A[a][aChatWith] = INVALID_PLAYER_ID; }
    FCNPC_SetUpdateRate(80);
    SetTimer("AI_Start", 12000, false);   // ждём мод: миграции и таблица accounts
    print("[AI] godjo_ai: жители начнут заходить через ~15 секунд");
    return 1;
}

public OnFilterScriptExit()
{
    KillTimer(gTimer);
    for (new a = 0; a < gAgentCount; a++) if (A[a][aOnline]) AI_Logout(a, "перезапуск");
    if (gAIDB) db_close(gAIDB);
    return 1;
}
