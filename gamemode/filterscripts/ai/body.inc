// =====================================================================
//  godjo_ai — «тело»: выполнение задач плана, движение по графу, машины
// =====================================================================
stock Body_Anim(a, kind)
{
    new npc = A[a][aNpc];
    switch (kind)
    {
        case 0: FCNPC_ApplyAnimation(npc, "SMOKING", "M_smklean_loop", 4.1, 1, 0, 0, 0, 0);
        case 1: { FCNPC_ApplyAnimation(npc, "PED", "phone_talk", 4.1, 1, 0, 0, 0, 0); if (random(2)) AI_Say(a, gPhoneTalk[random(sizeof gPhoneTalk)]); }
        case 2: FCNPC_ApplyAnimation(npc, "PED", "IDLE_tired", 4.1, 1, 0, 0, 0, 0);
        case 3: FCNPC_ApplyAnimation(npc, "COP_AMBIENT", "Coplook_loop", 4.1, 1, 0, 0, 0, 0);
        case 4: FCNPC_ApplyAnimation(npc, "COP_AMBIENT", "Coplook_think", 4.1, 1, 0, 0, 0, 0);
        case 10: FCNPC_ApplyAnimation(npc, "FOOD", "EAT_Burger", 4.1, 1, 0, 0, 0, 0);
        case 11: FCNPC_ApplyAnimation(npc, "DEALER", "DEALER_DEAL", 4.1, 0, 0, 0, 0, 0);
        case 12: FCNPC_ApplyAnimation(npc, "BD_FIRE", "wash_up", 4.1, 1, 0, 0, 0, 0);
        default: FCNPC_ApplyAnimation(npc, "PED", "IDLE_stance", 4.1, 1, 0, 0, 0, 0);
    }
}

stock Body_CurTask(a) return A[a][aPlanPos] < A[a][aPlanLen] ? A[a][aPlan][A[a][aPlanPos]] : T_NONE;
stock Body_CurArg(a) return A[a][aPlanPos] < A[a][aPlanLen] ? A[a][aPlanArg][A[a][aPlanPos]] : 0;

stock Body_Next(a)
{
    if (!A[a][aOnline]) return;
    FCNPC_ClearAnimations(A[a][aNpc]);
    A[a][aPlanPos]++;
    if (A[a][aPlanPos] >= A[a][aPlanLen])
    {
        if (A[a][aGoal] == G_FLEE) A[a][aFleeUntil] = 0;
        A[a][aGoal] = G_NONE;
        A[a][aThinkAt] = gTick + 2 + random(6);
        return;
    }
    Body_StartTask(a);
}

stock Body_StartTask(a)
{
    if (!A[a][aOnline] || A[a][aChatWith] != INVALID_PLAYER_ID || A[a][aConv] != -1) return;
    new t = Body_CurTask(a), arg = Body_CurArg(a), npc = A[a][aNpc];
    switch (t)
    {
        case T_NONE: return;
        case T_MOVE:
        {
            new Float:x, Float:y, Float:z; FCNPC_GetPosition(npc, x, y, z);
            new from = Nav_Nearest(x, y, 150.0);
            if (from == -1) { from = A[a][aHome]; FCNPC_SetPosition(npc, gNavPt[from][0], gNavPt[from][1], gNavPt[from][2]); }
            A[a][aDest] = arg;
            A[a][aPathLen] = Nav_Path(from, arg, gPath[a], AI_PATH_MAX);
            A[a][aPathPos] = 0;
            if (A[a][aPathLen] <= 0) { Body_Next(a); return; }
            A[a][aMoveType] = FCNPC_MOVE_TYPE_WALK;
            if (A[a][aGoal] == G_FLEE) A[a][aMoveType] = FCNPC_MOVE_TYPE_SPRINT;
            else if (A[a][aGoal] == G_WORK && random(4) == 0) A[a][aMoveType] = FCNPC_MOVE_TYPE_RUN;   // опаздывает
            Body_StepMove(a);
        }
        case T_ANIM:
        {
            Body_Anim(a, arg);
            A[a][aTaskUntil] = gTick + (arg == 11 ? 10 : 24 + random(56));
        }
        case T_INSIDE:
        {
            FCNPC_SetVirtualWorld(npc, AI_HIDDEN_VW);   // зашёл в здание
            A[a][aTaskUntil] = arg < 0 ? -1 : gTick + arg * 4;
        }
        case T_SOCIAL, T_WAIT:
        {
            Body_Anim(a, random(2) ? 3 : 4);
            A[a][aTaskUntil] = arg < 0 ? -1 : gTick + arg * 4;
        }
        case T_SWEEP:
        {
            Body_Anim(a, 12);
            A[a][aTaskUntil] = gTick + 40 + random(30);
            A[a][aPathLen] = 0;
        }
        case T_LOGOUT: AI_Logout(a, "ушёл домой");
    }
}

stock Body_StepMove(a)
{
    new npc = A[a][aNpc];
    if (A[a][aPathPos] >= A[a][aPathLen])
    {
        // путь был обрезан — дотягиваем до цели
        if (A[a][aPathLen] > 0 && gPath[a][A[a][aPathLen] - 1] != A[a][aDest]) { Body_StartTask(a); return; }
        Body_Next(a);
        return;
    }
    new n = gPath[a][A[a][aPathPos]];
    new Float:ox = 0.0, Float:oy = 0.0;
    if (A[a][aPathPos] == A[a][aPathLen] - 1) { ox = float(random(200) - 100) / 100.0; oy = float(random(200) - 100) / 100.0; }
    FCNPC_GoTo(npc, gNavPt[n][0] + ox, gNavPt[n][1] + oy, gNavPt[n][2], A[a][aMoveType], FCNPC_MOVE_SPEED_AUTO, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
}

// такт тела: таймеры задач, «застревание», выход из здания
stock Body_Tick(a)
{
    new t = Body_CurTask(a);
    if (A[a][aChatWith] != INVALID_PLAYER_ID)
    {
        if (gTick >= A[a][aChatUntil] || !IsPlayerConnected(A[a][aChatWith])) { A[a][aChatWith] = INVALID_PLAYER_ID; FCNPC_ClearAnimations(A[a][aNpc]); Body_StartTask(a); }
        return;
    }
    if (A[a][aConv] != -1) return;
    switch (t)
    {
        case T_ANIM, T_INSIDE, T_SOCIAL, T_WAIT:
        {
            new bool:done = false;
            if (A[a][aTaskUntil] == -1)
            {   // «до конца смены»
                if (gTick % 40 == a % 40 && (!AI_WorkHours(a, AI_Hour()) || gettime() >= A[a][aSessionEnd])) done = true;
            }
            else if (gTick >= A[a][aTaskUntil]) done = true;
            if (t == T_SOCIAL && !done && gTick % 20 == a % 20) Conv_TryStart(a);
            if (done)
            {
                if (t == T_INSIDE) Body_Exit(a);
                if (Body_CurTask(a) == t && A[a][aGoal] == G_EAT && (t == T_INSIDE || t == T_ANIM)) { gNeed[a][N_HUNGER] = 100.0; A[a][aMoney] -= 15 + random(20); if (A[a][aMoney] < 0) A[a][aMoney] = 0; }
                if (A[a][aGoal] == G_SOCIAL && t == T_SOCIAL) gNeed[a][N_FUN] = AI_ClampF(gNeed[a][N_FUN] + 20.0, 0.0, 100.0);
                if (A[a][aGoal] == G_WORK && (t == T_INSIDE || t == T_WAIT)) A[a][aMoney] += 150 + random(100);
                if (A[a][aGoal] == G_WORK && t == T_ANIM) A[a][aMoney] += 30 + random(20);   // курьер вручил посылку
                Body_Next(a);
            }
        }
        case T_SWEEP:
        {
            if (A[a][aPathLen] == 0 && gTick >= A[a][aTaskUntil])
            {
                if (!AI_WorkHours(a, AI_Hour()) || gettime() >= A[a][aSessionEnd]) { A[a][aMoney] += 120 + random(80); Body_Next(a); return; }
                new n = Nav_RandomNear(A[a][aWork] == -1 ? A[a][aHome] : A[a][aWork], 4.0, 30.0);
                FCNPC_ClearAnimations(A[a][aNpc]);
                A[a][aPathLen] = 1;   // «перехожу на новое место»
                FCNPC_GoTo(A[a][aNpc], gNavPt[n][0], gNavPt[n][1], gNavPt[n][2], FCNPC_MOVE_TYPE_WALK, FCNPC_MOVE_SPEED_AUTO, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
            }
        }
        case T_MOVE:
        {
            if (gTick % 20 != a % 20) return;   // проверка застревания раз в 5 с
            new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
            if (AI_Dist2(x, y, A[a][aLastX], A[a][aLastY]) < 0.4)
            {
                if (++A[a][aStuck] >= 2 && A[a][aPathPos] < A[a][aPathLen])
                {   // упёрся — переставляем на следующий узел (если никто не видит) или пропускаем его
                    new n = gPath[a][A[a][aPathPos]];
                    if (!FCNPC_IsStreamedInForAnyone(A[a][aNpc])) FCNPC_SetPosition(A[a][aNpc], gNavPt[n][0], gNavPt[n][1], gNavPt[n][2]);
                    A[a][aPathPos]++; A[a][aStuck] = 0;
                    Body_StepMove(a);
                }
            }
            else A[a][aStuck] = 0;
            A[a][aLastX] = x; A[a][aLastY] = y;
        }
    }
}

stock Body_Exit(a)
{
    FCNPC_SetVirtualWorld(A[a][aNpc], 0);
}

stock Body_OnReach(a)
{
    if (AI_IsDriver(a)) { Drive_Next(a); return; }
    if (A[a][aChatWith] != INVALID_PLAYER_ID || A[a][aConv] != -1) return;
    switch (Body_CurTask(a))
    {
        case T_MOVE: { A[a][aPathPos]++; Body_StepMove(a); }
        case T_SWEEP: { A[a][aPathLen] = 0; Body_Anim(a, 12); A[a][aTaskUntil] = gTick + 40 + random(40); }
    }
}

// ---------------------------------------------------------------- водители (смена на маршруте)
stock Body_StartDriving(a)
{
    new kind = A[a][aJob], path = kind == JOB_BUS ? 0 : random(sizeof gDrivePath);
    A[a][aDrivePath] = path;
    A[a][aDrivePt] = random(gDrivePath[path][1]);
    new k = gDrivePath[path][0] + A[a][aDrivePt], k2 = gDrivePath[path][0] + (A[a][aDrivePt] + 1) % gDrivePath[path][1];
    new Float:x = gDrivePt[k][0], Float:y = gDrivePt[k][1], Float:z = gDrivePt[k][2];
    new Float:ang = -atan2(gDrivePt[k2][0] - x, gDrivePt[k2][1] - y); if (ang < 0.0) ang += 360.0;
    new model = kind == JOB_TAXI ? 420 : kind == JOB_BUS ? 431 : gCarModels[A[a][aId] % sizeof gCarModels];
    FCNPC_SetPosition(A[a][aNpc], x, y, z);
    A[a][aVeh] = CreateVehicle(model, x, y, z, ang, kind == JOB_TAXI ? 6 : A[a][aId] % 127, kind == JOB_TAXI ? 1 : (A[a][aId] * 7) % 127, -1);
    SetVehicleParamsEx(A[a][aVeh], 1, 1, 0, 0, 0, 0, 0);
    FCNPC_PutInVehicle(A[a][aNpc], A[a][aVeh], 0);
    A[a][aGoal] = G_WORK; A[a][aGoalSince] = gettime(); Plan_Clear(a); Plan_Add(a, T_DRIVE, 0);
    A[a][aTaskUntil] = gTick + 12;   // прогрев, потом поехали
}

stock Body_StopDriving(a)
{
    if (A[a][aVeh] == INVALID_VEHICLE_ID) return;
    if (A[a][aNpc] != INVALID_PLAYER_ID) FCNPC_RemoveFromVehicle(A[a][aNpc]);
    DestroyVehicle(A[a][aVeh]);
    A[a][aVeh] = INVALID_VEHICLE_ID;
}

stock Drive_Next(a)
{
    if (A[a][aVeh] == INVALID_VEHICLE_ID) return;
    if (!AI_WorkHours(a, AI_Hour()) || gettime() >= A[a][aSessionEnd]) { AI_Logout(a, "смена окончена"); return; }
    // остановки: автобус — каждые 9 точек, такси — изредка «берёт пассажира»
    if ((A[a][aJob] == JOB_BUS && A[a][aDrivePt] % 9 == 0) || (A[a][aJob] == JOB_TAXI && random(30) == 0)) { A[a][aTaskUntil] = gTick + 32 + random(24); return; }
    Drive_Go(a);
}

stock Drive_Go(a)
{
    new path = A[a][aDrivePath];
    A[a][aDrivePt] = (A[a][aDrivePt] + 1) % gDrivePath[path][1];
    new k = gDrivePath[path][0] + A[a][aDrivePt];
    new Float:sp = A[a][aJob] == JOB_BUS ? 0.75 : 0.85 + float(A[a][aTrait][TR_TEMPER]) / 250.0;   // вспыльчивые гоняют
    FCNPC_GoTo(A[a][aNpc], gDrivePt[k][0], gDrivePt[k][1], gDrivePt[k][2], FCNPC_MOVE_TYPE_DRIVE, sp, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
    A[a][aTaskUntil] = 0;
}

stock Drive_Tick(a)
{
    if (A[a][aTaskUntil] && gTick >= A[a][aTaskUntil]) Drive_Go(a);
}
