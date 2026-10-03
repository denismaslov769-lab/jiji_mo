// =====================================================================
//  godjo_ai — агенты: база, генерация населения, сессии (вход/выход)
// =====================================================================
stock AI_Schema()
{
    AI_Exec("CREATE TABLE IF NOT EXISTS ai_agents (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE COLLATE NOCASE, sex INTEGER, skin INTEGER, level INTEGER, ping INTEGER, traits TEXT, needs TEXT, money INTEGER DEFAULT 300, job INTEGER DEFAULT 0, work_from INTEGER, work_to INTEGER, wake INTEGER, sleep INTEGER, home INTEGER, work INTEGER, enabled INTEGER DEFAULT 1, last_logout INTEGER DEFAULT 0, created INTEGER)");
    AI_Exec("CREATE TABLE IF NOT EXISTS ai_memory (id INTEGER PRIMARY KEY AUTOINCREMENT, agent INTEGER, kind INTEGER, who TEXT, value INTEGER, imp INTEGER, t INTEGER)");
    AI_Exec("CREATE INDEX IF NOT EXISTS ai_memory_agent ON ai_memory(agent, t)");
    AI_Exec("CREATE TABLE IF NOT EXISTS ai_relations (agent INTEGER, who TEXT COLLATE NOCASE, aff INTEGER DEFAULT 0, fam INTEGER DEFAULT 0, seen INTEGER, PRIMARY KEY (agent, who))");
    AI_Exec("CREATE TABLE IF NOT EXISTS ai_sessions (id INTEGER PRIMARY KEY AUTOINCREMENT, agent INTEGER, login INTEGER, logout INTEGER DEFAULT 0, reason TEXT DEFAULT '')");
}

stock bool:AI_NameFree(const name[])
{
    new q[160], e[MAX_PLAYER_NAME * 2];
    AI_Esc(e, name);
    format(q, sizeof q, "SELECT 1 FROM accounts WHERE name='%s' AND IFNULL(is_agent,0)=0 UNION SELECT 1 FROM ai_agents WHERE name='%s'", e, e);
    new DBResult:r = db_query(gAIDB, q);
    if (!r) return true;   // таблицы accounts ещё нет — значит, имя свободно
    new n = db_num_rows(r); db_free_result(r);
    if (n) return false;
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i)) continue;
        new pn[MAX_PLAYER_NAME]; GetPlayerName(i, pn, sizeof pn);
        if (!strcmp(pn, name, true)) return false;
    }
    return true;
}

stock AI_RandLevel()
{
    new r = random(100);
    if (r < 35) return 1 + random(2);
    if (r < 70) return 3 + random(3);
    if (r < 92) return 6 + random(5);
    return 11 + random(10);
}

// распределение работ в населении (водителей мало — они занимают машины)
stock AI_JobFor(i)
{
    switch (i)
    {
        case 0: return JOB_BUS;
        case 1, 2: return JOB_TAXI;
        case 3, 4, 5: return JOB_DRIVER;
        case 6, 7, 8: return JOB_SWEEPER;
        case 9, 10, 11, 12, 13: return JOB_OFFICE;
        case 14, 15, 16: return JOB_SELLER;
        case 17, 18, 19, 20: return JOB_COURIER;
    }
    return JOB_NONE;
}

stock AI_Generate(count)
{
    new made = 0;
    for (new i = 0; i < count; i++)
    {
        new sex = random(3) == 0 ? 1 : 0, job = AI_JobFor(i), name[MAX_PLAYER_NAME], first[16], tries = 0;
        if (job == JOB_BUS || job == JOB_TAXI) sex = 0;
        do
        {
            new last[20];
            if (random(4) == 0) format(last, sizeof last, "%s", gLastEn[random(sizeof gLastEn)]);
            else { format(last, sizeof last, "%s", gLastRu[random(sizeof gLastRu)]); if (sex) strcat(last, "a"); }
            if (sex) format(first, sizeof first, "%s", gFemFirst[random(sizeof gFemFirst)]);
            else format(first, sizeof first, "%s", gMaleFirst[random(sizeof gMaleFirst)]);
            format(name, sizeof name, "%s_%s", first, last);
        }
        while (!AI_NameFree(name) && ++tries < 30);
        if (tries >= 30) continue;
        new skin = sex ? gFemSkins[random(sizeof gFemSkins)] : gMaleSkins[random(sizeof gMaleSkins)];
        if (job == JOB_SWEEPER) skin = sex ? 157 : 16;
        else if (job == JOB_TAXI) skin = 255;
        else if (job == JOB_BUS) skin = 253;
        else if (job == JOB_OFFICE) skin = sex ? 150 : (random(2) ? 147 : 17);
        new tr[64]; format(tr, sizeof tr, "%d,%d,%d,%d,%d", random(101), 20 + random(81), random(101), random(101), random(101));
        new wf = 8 + random(3), wt = wf + 6 + random(4);
        if (job == JOB_DRIVER || job == JOB_TAXI || job == JOB_BUS) { wf = 6 + random(9); wt = wf + 8; if (wt > 24) wt = 24; }   // смены водителей разнесены по дню
        if (job == JOB_NONE) { wf = 0; wt = 0; }
        new wake = 6 + random(5), slp = 21 + random(5);   // слп >= 24 — после полуночи
        new hn = Nav_Nearest(1760.0 + float(random(1400)) - 700.0, -1700.0 + float(random(900)) - 450.0, 400.0);
        if (hn == -1) hn = Nav_Nearest(1760.0, -1895.0, 800.0);
        new home = Nav_RandomNear(hn, 0.0, 200.0);
        new work = AI_PickWorkNode(job);
        new q[512];
        format(q, sizeof q, "INSERT INTO ai_agents (name,sex,skin,level,ping,traits,needs,money,job,work_from,work_to,wake,sleep,home,work,created) VALUES ('%s',%d,%d,%d,%d,'%s','%d,%d,%d,%d',%d,%d,%d,%d,%d,%d,%d,%d,%d)",
            name, sex, skin, AI_RandLevel(), 35 + random(90), tr, 50 + random(50), 60 + random(40), random(100), random(100), 100 + random(900), job, wf, wt, wake, slp, home, work, gettime());
        AI_Exec(q);
        made++;
    }
    printf("[AI] Сгенерировано жителей: %d", made);
}

stock AI_PickWorkNode(job)
{
    new code[20];
    switch (job)
    {
        case JOB_SWEEPER: code = "job_janitor";
        case JOB_OFFICE: code = random(2) ? ("cityhall_door") : ("bank");
        case JOB_SELLER: code = random(2) ? ("kiosk") : ("shop247_door");
        case JOB_COURIER: code = "job_courier";
        default: return -1;
    }
    new p = Place_Find(code);
    return p == -1 ? -1 : gPlace[p][plNode];
}

stock AI_ParseCSV(const s[], out[], n)
{
    new k = 0, v = 0, bool:neg = false, bool:any = false;
    for (new i = 0; ; i++)
    {
        new c = s[i];
        if (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); any = true; }
        else if (c == '-') neg = true;
        else if (c == ',' || c == 0)
        {
            if (k < n && any) out[k++] = neg ? -v : v;
            v = 0; neg = false; any = false;
            if (c == 0) break;
        }
    }
    return k;
}

stock AI_LoadAll()
{
    gAgentCount = 0;
    new lq[96]; format(lq, sizeof lq, "SELECT * FROM ai_agents WHERE enabled=1 ORDER BY id LIMIT %d", AI_POP);
    new DBResult:r = db_query(gAIDB, lq);
    if (!r) return;
    new buf[96];
    for (new i = 0, n = db_num_rows(r); i < n && gAgentCount < AI_POP; i++, db_next_row(r))
    {
        new a = gAgentCount++;
        A[a][aId] = db_get_field_assoc_int(r, "id");
        db_get_field_assoc(r, "name", A[a][aName], MAX_PLAYER_NAME);
        format(A[a][aFirst], 16, "%s", A[a][aName]);
        new us = strfind(A[a][aFirst], "_"); if (us > 0) A[a][aFirst][us] = 0;
        A[a][aSex] = db_get_field_assoc_int(r, "sex"); A[a][aSkin] = db_get_field_assoc_int(r, "skin");
        A[a][aLevel] = db_get_field_assoc_int(r, "level"); A[a][aPing] = db_get_field_assoc_int(r, "ping");
        new tr[TR_COUNT]; db_get_field_assoc(r, "traits", buf, sizeof buf); AI_ParseCSV(buf, tr, TR_COUNT);
        for (new t = 0; t < TR_COUNT; t++) A[a][aTrait][t] = AI_Clamp(tr[t], 0, 100);
        new nd[N_COUNT]; db_get_field_assoc(r, "needs", buf, sizeof buf); AI_ParseCSV(buf, nd, N_COUNT);
        for (new t = 0; t < N_COUNT; t++) gNeed[a][t] = float(AI_Clamp(nd[t], 0, 100));
        A[a][aMoney] = db_get_field_assoc_int(r, "money"); A[a][aJob] = db_get_field_assoc_int(r, "job");
        A[a][aWorkFrom] = db_get_field_assoc_int(r, "work_from"); A[a][aWorkTo] = db_get_field_assoc_int(r, "work_to");
        A[a][aWake] = db_get_field_assoc_int(r, "wake"); A[a][aSleep] = db_get_field_assoc_int(r, "sleep");
        A[a][aHome] = db_get_field_assoc_int(r, "home"); A[a][aWork] = db_get_field_assoc_int(r, "work");
        if (A[a][aHome] < 0 || A[a][aHome] >= NAV_NODES) A[a][aHome] = Nav_Nearest(1760.0, -1895.0, 500.0);
        if (A[a][aWork] >= NAV_NODES) A[a][aWork] = -1;
        if (A[a][aWork] == -1 && A[a][aJob] >= JOB_SWEEPER && A[a][aJob] <= JOB_COURIER) A[a][aWork] = AI_PickWorkNode(A[a][aJob]);
        // время офлайн: потребности восстанавливаются (спал, ел дома)
        new off = gettime() - db_get_field_assoc_int(r, "last_logout");
        if (off > 3600) { gNeed[a][N_ENERGY] = 100.0; gNeed[a][N_HUNGER] = AI_ClampF(gNeed[a][N_HUNGER] + 40.0, 0.0, 100.0); }
        A[a][aOnline] = false; A[a][aNpc] = INVALID_PLAYER_ID; A[a][aVeh] = INVALID_VEHICLE_ID; A[a][aConv] = -1; A[a][aChatWith] = INVALID_PLAYER_ID;
        A[a][aNextLogin] = gettime() + random(120);
        A[a][aGoal] = G_NONE; A[a][aPlanLen] = 0;
    }
    db_free_result(r);
    printf("[AI] Загружено жителей: %d", gAgentCount);
}

stock AI_Save(a)
{
    new q[400];
    format(q, sizeof q, "UPDATE ai_agents SET needs='%d,%d,%d,%d', money=%d, level=%d, home=%d, work=%d, last_logout=%d WHERE id=%d",
        floatround(gNeed[a][N_HUNGER]), floatround(gNeed[a][N_ENERGY]), floatround(gNeed[a][N_SOCIAL]), floatround(gNeed[a][N_FUN]),
        A[a][aMoney], A[a][aLevel], A[a][aHome], A[a][aWork], A[a][aOnline] ? 0 : gettime(), A[a][aId]);
    AI_Exec(q);
}

// ---------------------------------------------------------------- расписание
stock bool:AI_Awake(a, h)
{
    new w = A[a][aWake], s = A[a][aSleep];
    if (s >= 24) return h >= w || h < s - 24;
    return h >= w && h < s;
}
stock bool:AI_WorkHours(a, h) return A[a][aJob] != JOB_NONE && h >= A[a][aWorkFrom] && h < A[a][aWorkTo];
stock bool:AI_IsDriver(a) return A[a][aJob] == JOB_DRIVER || A[a][aJob] == JOB_TAXI || A[a][aJob] == JOB_BUS;

stock AI_OnlineCount()
{
    new n = 0;
    for (new a = 0; a < gAgentCount; a++) if (A[a][aOnline]) n++;
    return n;
}

// сколько жителей «должно» быть онлайн: суточная кривая (ночью мало, вечером пик)
stock AI_TargetOnline()
{
    static const curve[24] = {35,25,18,12,10,12,20,35,50,55,60,65,70,70,70,75,80,90,100,100,95,85,70,50};
    return gMaxOnline * curve[AI_Hour()] / 100;
}

stock AI_SessionTick()
{
    if (!gEnabled) return;
    new now = gettime(), h = AI_Hour(), online = AI_OnlineCount(), target = AI_TargetOnline();
    // вход: по одному за раз, как живые игроки (не все сразу)
    if (online < target)
    {
        new pick = -1, seen = 0;
        for (new a = 0; a < gAgentCount; a++)
        {
            if (A[a][aOnline] || A[a][aStopped] || now < A[a][aNextLogin] || !AI_Awake(a, h)) continue;
            if (AI_IsDriver(a) && !AI_WorkHours(a, h)) continue;   // водители выходят только на смену
            seen++;
            new w = AI_IsDriver(a) ? 3 : 1;                       // водителей на смене — в приоритете
            if (random(seen * w) < w) pick = a;
        }
        if (pick != -1) AI_Login(pick);
    }
    else if (online > target + 1)
    {
        // лишние уходят домой естественно (не исчезают посреди улицы)
        for (new a = 0; a < gAgentCount; a++)
            if (A[a][aOnline] && A[a][aGoal] != G_HOME && !AI_IsDriver(a) && A[a][aChatWith] == INVALID_PLAYER_ID && random(3) == 0) { A[a][aSessionEnd] = now; break; }
    }
}

stock AI_Login(a)
{
    if (A[a][aOnline]) return 0;
    new npc = FCNPC_Create(A[a][aName]);
    if (npc == INVALID_PLAYER_ID) { A[a][aNextLogin] = gettime() + 600; return 0; }
    A[a][aOnline] = true; A[a][aNpc] = npc; gAgentOf[npc] = a;
    A[a][aAcc] = CallRemoteFunction("GM_AgentAttach", "is", npc, A[a][aName]);
    CallRemoteFunction("GM_BotRegister", "iii", npc, A[a][aLevel], A[a][aPing]);
    new node = A[a][aHome];
    if (!AI_IsDriver(a) && random(100) < 25) { new p = Place_Find("ls_station"); if (p != -1) node = gPlace[p][plNode]; }  // приехал на поезде
    FCNPC_Spawn(npc, A[a][aSkin], gNavPt[node][0], gNavPt[node][1], gNavPt[node][2]);
    FCNPC_SetInvulnerable(npc, false);
    FCNPC_SetHealth(npc, 100.0);
    SetPlayerColor(npc, 0xFFFFFF00);
    A[a][aSessionEnd] = gettime() + 60 * (25 + random(100));
    A[a][aGoal] = G_NONE; A[a][aPlanLen] = 0; A[a][aPathLen] = 0; A[a][aDest] = node;
    A[a][aConv] = -1; A[a][aChatWith] = INVALID_PLAYER_ID; A[a][aStuck] = 0; A[a][aThinkAt] = gTick + 4;
    A[a][aSaveAt] = gettime() + 300; A[a][aLastX] = gNavPt[node][0]; A[a][aLastY] = gNavPt[node][1];
    Mem_Load(a); Rel_Load(a);
    new q[160]; format(q, sizeof q, "INSERT INTO ai_sessions (agent,login) VALUES (%d,%d)", A[a][aId], gettime()); AI_Exec(q);
    new DBResult:r = db_query(gAIDB, "SELECT last_insert_rowid()");
    if (r) { A[a][aSessionRow] = db_get_field_int(r, 0); db_free_result(r); }
    if (AI_IsDriver(a)) Body_StartDriving(a);
    return 1;
}

stock AI_Logout(a, const reason[])
{
    if (!A[a][aOnline]) return;
    Conv_End(a);
    Body_StopDriving(a);
    A[a][aOnline] = false;
    AI_Save(a);
    new q[200], e[32]; AI_Esc(e, reason);
    format(q, sizeof q, "UPDATE ai_sessions SET logout=%d, reason='%s' WHERE id=%d", gettime(), e, A[a][aSessionRow]); AI_Exec(q);
    format(q, sizeof q, "UPDATE ai_agents SET last_logout=%d WHERE id=%d", gettime(), A[a][aId]); AI_Exec(q);
    if (A[a][aNpc] != INVALID_PLAYER_ID) { gAgentOf[A[a][aNpc]] = -1; FCNPC_Destroy(A[a][aNpc]); }
    A[a][aNpc] = INVALID_PLAYER_ID;
    A[a][aNextLogin] = gettime() + 60 * (20 + random(90));
    A[a][aChatWith] = INVALID_PLAYER_ID;
}
