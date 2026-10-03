// =====================================================================
//  godjo_ai — «мозг»: потребности, Utility AI с инерцией, план (HTN-lite)
// =====================================================================
stock Place_Find(const code[])
{
    for (new p = 0; p < sizeof gPlace; p++) if (!strcmp(gPlace[p][plCode], code)) return p;
    return -1;
}

// ближайшее место с флагом (среди случайных кандидатов — чтобы не все шли в одно)
stock Place_Pick(a, flags, Float:maxd = 900.0)
{
    new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
    new best = -1, Float:bs = -1.0;
    for (new p = 0; p < sizeof gPlace; p++)
    {
        if (!(gPlace[p][plFlags] & flags) || gPlace[p][plNode] == -1) continue;
        new Float:d = AI_Dist2(x, y, gPlace[p][plX], gPlace[p][plY]);
        if (d > maxd) continue;
        new Float:s = 1000.0 - d + float(random(250));
        if (s > bs) { bs = s; best = p; }
    }
    return best;
}

stock Places_Init()
{
    // координаты берём из таблицы locations мода (если точку передвинули /setloc)
    for (new p = 0; p < sizeof gPlace; p++)
    {
        new q[128]; format(q, sizeof q, "SELECT x,y,z FROM locations WHERE code='%s' AND interior=0", gPlace[p][plCode]);
        new DBResult:r = db_query(gAIDB, q);
        if (r)
        {
            if (db_num_rows(r)) { gPlace[p][plX] = db_get_field_float(r, 0); gPlace[p][plY] = db_get_field_float(r, 1); gPlace[p][plZ] = db_get_field_float(r, 2); }
            db_free_result(r);
        }
        gPlace[p][plNode] = Nav_Nearest(gPlace[p][plX], gPlace[p][plY], 90.0);
    }
}

// ---------------------------------------------------------------- потребности
stock Brain_Needs(a, Float:minutes)
{
    new Float:soc = 0.4 + float(A[a][aTrait][TR_SOCIAL]) / 125.0;
    gNeed[a][N_HUNGER] = AI_ClampF(gNeed[a][N_HUNGER] - 0.55 * minutes, 0.0, 100.0);
    gNeed[a][N_ENERGY] = AI_ClampF(gNeed[a][N_ENERGY] - (A[a][aGoal] == G_WORK ? 0.45 : 0.22) * minutes, 0.0, 100.0);
    if (A[a][aGoal] != G_SOCIAL && A[a][aGoal] != G_CHAT) gNeed[a][N_SOCIAL] = AI_ClampF(gNeed[a][N_SOCIAL] - 0.7 * soc * minutes, 0.0, 100.0);
    if (A[a][aGoal] != G_WANDER) gNeed[a][N_FUN] = AI_ClampF(gNeed[a][N_FUN] - 0.45 * minutes, 0.0, 100.0);
    else gNeed[a][N_FUN] = AI_ClampF(gNeed[a][N_FUN] + 2.0 * minutes, 0.0, 100.0);
    if (A[a][aGoal] == G_SOCIAL) gNeed[a][N_SOCIAL] = AI_ClampF(gNeed[a][N_SOCIAL] + 1.0 * minutes, 0.0, 100.0);
}

// ---------------------------------------------------------------- полезность целей
stock Float:Brain_Score(a, g, h)
{
    new Float:hu = 100.0 - gNeed[a][N_HUNGER], Float:so = 100.0 - gNeed[a][N_SOCIAL], Float:fu = 100.0 - gNeed[a][N_FUN];
    switch (g)
    {
        case G_IDLE: return 14.0 + float(random(8));
        case G_WANDER: return fu * 0.7 + float(A[a][aTrait][TR_CURIOUS]) * 0.15 + float(random(10));
        case G_EAT: return A[a][aMoney] >= 15 ? hu * 1.25 - 10.0 : hu * 0.4;
        case G_WORK:
        {
            if (!AI_WorkHours(a, h) || A[a][aWork] == -1 && !AI_IsDriver(a)) return 0.0;
            return 55.0 + float(A[a][aTrait][TR_DILIGENT]) * 0.35 + (A[a][aMoney] < 100 ? 15.0 : 0.0);
        }
        case G_SOCIAL: return so * (0.45 + float(A[a][aTrait][TR_SOCIAL]) / 200.0) + float(random(10));
        case G_HOME:
        {
            if (gettime() >= A[a][aSessionEnd] || !AI_Awake(a, h)) return 95.0;
            if (gNeed[a][N_ENERGY] < 15.0) return 92.0;
            return 0.0;
        }
    }
    return 0.0;
}

// выбор цели: лучшая по полезности, но с инерцией — текущую не бросаем без явной причины
stock Brain_Think(a)
{
    if (A[a][aGoal] == G_FLEE || A[a][aGoal] == G_CHAT || A[a][aConv] != -1) return;
    if (AI_IsDriver(a)) return;   // водители живут сменой (см. Body_Drive*)
    new h = AI_Hour(), now = gettime(), best = G_IDLE, Float:bs = -1.0;
    for (new g = G_IDLE; g <= G_HOME; g++)
    {
        if (g == G_FLEE || g == G_CHAT) continue;
        new Float:s = Brain_Score(a, g, h);
        if (s > bs) { bs = s; best = g; }
    }
    new cur = A[a][aGoal];
    if (cur != G_NONE && A[a][aPlanPos] < A[a][aPlanLen])
    {
        new Float:cs = Brain_Score(a, cur, h);
        if (now - A[a][aGoalSince] < A[a][aGoalMin]) cs += 25.0;   // обязательство
        cs += 10.0;                                                // гистерезис
        if (best == cur || bs <= cs) return;
        if (cur == G_HOME) return;                                 // ушёл домой — дойдёт
    }
    Brain_SetGoal(a, best, bs);
}

stock Plan_Clear(a) { A[a][aPlanLen] = 0; A[a][aPlanPos] = 0; }
stock Plan_Add(a, task, arg) { if (A[a][aPlanLen] < AI_PLAN) { A[a][aPlan][A[a][aPlanLen]] = task; A[a][aPlanArg][A[a][aPlanLen]] = arg; A[a][aPlanLen]++; } }

// HTN-lite: цель -> последовательность задач
stock Brain_SetGoal(a, g, Float:score = 0.0)
{
    if (Body_CurTask(a) == T_INSIDE) Body_Exit(a);   // вышел из здания, если был внутри
    if (Body_CurTask(a) == T_MOVE || Body_CurTask(a) == T_SWEEP) FCNPC_Stop(A[a][aNpc]);
    FCNPC_ClearAnimations(A[a][aNpc]);
    Plan_Clear(a);
    A[a][aGoal] = g; A[a][aGoalSince] = gettime(); A[a][aGoalScore] = score;
    switch (g)
    {
        case G_IDLE:
        {
            A[a][aGoalMin] = 20;
            Plan_Add(a, T_ANIM, random(5));
        }
        case G_WANDER:
        {
            A[a][aGoalMin] = 120;
            new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
            new from = Nav_Nearest(x, y, 120.0); if (from == -1) from = A[a][aHome];
            Plan_Add(a, T_MOVE, Nav_RandomNear(from, 80.0, 260.0));
            if (random(2)) Plan_Add(a, T_ANIM, random(5));
            Plan_Add(a, T_MOVE, Nav_RandomNear(from, 60.0, 220.0));
        }
        case G_EAT:
        {
            A[a][aGoalMin] = 240;
            new p = Place_Pick(a, PF_FOOD);
            if (p == -1) { Plan_Add(a, T_ANIM, 0); return; }
            Plan_Add(a, T_MOVE, gPlace[p][plNode]);
            if (gPlace[p][plFlags] & PF_INSIDE) Plan_Add(a, T_INSIDE, 40 + random(60));
            else Plan_Add(a, T_ANIM, 10);   // 10 = еда
        }
        case G_WORK:
        {
            A[a][aGoalMin] = 900;
            switch (A[a][aJob])
            {
                case JOB_SWEEPER: { Plan_Add(a, T_MOVE, A[a][aWork]); Plan_Add(a, T_SWEEP, 0); }
                case JOB_OFFICE: { Plan_Add(a, T_MOVE, A[a][aWork]); Plan_Add(a, T_INSIDE, -1); }
                case JOB_SELLER:
                {
                    Plan_Add(a, T_MOVE, A[a][aWork]);
                    new p = Place_Find("kiosk");
                    if (p != -1 && gPlace[p][plNode] == A[a][aWork]) Plan_Add(a, T_WAIT, -1); else Plan_Add(a, T_INSIDE, -1);
                }
                case JOB_COURIER:
                {
                    Plan_Add(a, T_MOVE, A[a][aWork]);
                    for (new i = 0; i < 3; i++)
                    {
                        new p = random(sizeof gPlace);
                        if (gPlace[p][plNode] != -1) { Plan_Add(a, T_MOVE, gPlace[p][plNode]); Plan_Add(a, T_ANIM, 11); }   // 11 = вручить посылку
                    }
                }
            }
        }
        case G_SOCIAL:
        {
            A[a][aGoalMin] = 300;
            new p = Place_Pick(a, PF_SOCIAL, 1200.0);
            if (p != -1) Plan_Add(a, T_MOVE, Nav_RandomNear(gPlace[p][plNode], 0.0, 25.0));
            Plan_Add(a, T_SOCIAL, 60 + random(120));
        }
        case G_HOME:
        {
            A[a][aGoalMin] = 99999;
            Plan_Add(a, T_MOVE, A[a][aHome]);
            Plan_Add(a, T_LOGOUT, 0);
        }
    }
    A[a][aPlanPos] = 0;
    Body_StartTask(a);
}

// «Что делаешь?» — текст текущего занятия
stock Brain_Doing(a, out[], size = sizeof out)
{
    new t = A[a][aPlanPos] < A[a][aPlanLen] ? A[a][aPlan][A[a][aPlanPos]] : T_NONE;
    switch (A[a][aGoal])
    {
        case G_WORK:
        {
            if (t == T_MOVE) format(out, size, "иду на работу");
            else switch (A[a][aJob])
            {
                case JOB_SWEEPER: format(out, size, "подметаю тут, работа такая");
                case JOB_COURIER: format(out, size, "посылки разношу");
                default: format(out, size, "работаю");
            }
        }
        case G_EAT: format(out, size, "иду перекусить");
        case G_SOCIAL: format(out, size, t == T_MOVE ? ("иду к знакомым") : ("жду друзей"));
        case G_WANDER: format(out, size, "гуляю");
        case G_HOME: format(out, size, "домой иду, устал%s", A[a][aSex] ? ("а") : (""));
        default: format(out, size, "отдыхаю");
    }
}
