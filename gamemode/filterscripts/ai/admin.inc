// =====================================================================
//  godjo_ai — админ-команды: /aibots, /aibot
// =====================================================================
stock AI_Msg(playerid, const text[]) { SendClientMessage(playerid, 0x7FB2FFFF, text); return 1; }

stock AI_FindAgent(const s[])
{
    if (!s[0]) return -1;
    new bool:num = true;
    for (new i = 0; s[i]; i++) if (s[i] < '0' || s[i] > '9') num = false;
    if (num)
    {
        new id = strval(s);
        if (id >= 0 && id < MAX_PLAYERS && gAgentOf[id] != -1) return gAgentOf[id];   // по playerid
        for (new a = 0; a < gAgentCount; a++) if (A[a][aId] == id) return a;          // по id жителя
        return -1;
    }
    for (new a = 0; a < gAgentCount; a++) if (strfind(A[a][aName], s, true) != -1) return a;
    return -1;
}

stock AI_Word(const src[], idx, dest[], size = sizeof dest)
{
    new i = 0, w = 0, j = 0;
    while (src[i] == ' ') i++;
    while (src[i] && w < idx) { while (src[i] && src[i] != ' ') i++; while (src[i] == ' ') i++; w++; }
    while (src[i] && src[i] != ' ' && j < size - 1) dest[j++] = src[i++];
    dest[j] = 0;
}

stock AI_Cmd(playerid, const cmd[], const params[])
{
    if (!strcmp(cmd, "aibots", true))
    {
        if (AI_AdminLevel(playerid) < 3) return 0;
        new w[16], s[144]; AI_Word(params, 0, w);
        if (!strcmp(w, "on", true)) { gEnabled = true; return AI_Msg(playerid, "[AI] Жители включены."); }
        if (!strcmp(w, "off", true))
        {
            gEnabled = false;
            for (new a = 0; a < gAgentCount; a++) if (A[a][aOnline]) AI_Logout(a, "выключено админом");
            return AI_Msg(playerid, "[AI] Жители выключены (все вышли).");
        }
        if (!strcmp(w, "max", true))
        {
            new v[8]; AI_Word(params, 1, v);
            gMaxOnline = AI_Clamp(strval(v), 0, AI_MAX_ONLINE_CAP);
            format(s, sizeof s, "[AI] Максимум онлайн: %d.", gMaxOnline);
            return AI_Msg(playerid, s);
        }
        if (!strcmp(w, "list", true))
        {
            for (new a = 0; a < gAgentCount; a++)
            {
                if (!A[a][aOnline]) continue;
                format(s, sizeof s, "[%d] %s — %s, цель: %s", A[a][aNpc], A[a][aName], gJobName[A[a][aJob]], gGoalName[A[a][aGoal]]);
                AI_Msg(playerid, s);
            }
            return 1;
        }
        format(s, sizeof s, "[AI] Жителей: %d, онлайн %d (цель сейчас %d, максимум %d), %s. /aibots on|off|max N|list, /aibot",
            gAgentCount, AI_OnlineCount(), AI_TargetOnline(), gMaxOnline, gEnabled ? ("включены") : ("выключены"));
        return AI_Msg(playerid, s);
    }
    if (!strcmp(cmd, "aibot", true))
    {
        if (AI_AdminLevel(playerid) < 3) return 0;
        new who[24], act[16], s[144]; AI_Word(params, 0, who); AI_Word(params, 1, act);
        new a = AI_FindAgent(who);
        if (a == -1) return AI_Msg(playerid, "/aibot [id игрока | id жителя | имя] info|goto|task <eat|work|social|wander|home|idle>|memory|rel|stop|start|login|debug");
        if (!act[0] || !strcmp(act, "info", true))
        {
            format(s, sizeof s, "[AI] %s (житель #%d, %s) | %s | ур. %d | $%d | работа %d-%d | день %d-%d",
                A[a][aName], A[a][aId], A[a][aOnline] ? ("онлайн") : ("офлайн"), gJobName[A[a][aJob]], A[a][aLevel], A[a][aMoney], A[a][aWorkFrom], A[a][aWorkTo], A[a][aWake], A[a][aSleep] % 24);
            AI_Msg(playerid, s);
            format(s, sizeof s, "Характер: общит. %d, трудол. %d, любопыт. %d, вспыльч. %d, вежл. %d",
                A[a][aTrait][TR_SOCIAL], A[a][aTrait][TR_DILIGENT], A[a][aTrait][TR_CURIOUS], A[a][aTrait][TR_TEMPER], A[a][aTrait][TR_POLITE]);
            AI_Msg(playerid, s);
            format(s, sizeof s, "Потребности: сытость %.0f, бодрость %.0f, общение %.0f, досуг %.0f | цель: %s (%.0f), задача %d/%d, путь %d/%d",
                gNeed[a][N_HUNGER], gNeed[a][N_ENERGY], gNeed[a][N_SOCIAL], gNeed[a][N_FUN], gGoalName[A[a][aGoal]], A[a][aGoalScore],
                A[a][aPlanPos], A[a][aPlanLen], A[a][aPathPos], A[a][aPathLen]);
            return AI_Msg(playerid, s);
        }
        if (!strcmp(act, "login", true)) { A[a][aStopped] = false; if (!AI_Login(a)) return AI_Msg(playerid, "[AI] Не удалось (уже онлайн или занят слот)."); return AI_Msg(playerid, "[AI] Житель зашёл."); }
        if (!A[a][aOnline]) return AI_Msg(playerid, "[AI] Житель офлайн. /aibot ... login");
        if (!strcmp(act, "goto", true))
        {
            new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
            SetPlayerInterior(playerid, 0); SetPlayerVirtualWorld(playerid, 0);
            SetPlayerPos(playerid, x + 1.5, y, z + 0.5);
            return 1;
        }
        if (!strcmp(act, "task", true))
        {
            new g[12]; AI_Word(params, 2, g);
            new goal = G_NONE;
            if (!strcmp(g, "eat", true)) goal = G_EAT; else if (!strcmp(g, "work", true)) goal = G_WORK;
            else if (!strcmp(g, "social", true)) goal = G_SOCIAL; else if (!strcmp(g, "wander", true)) goal = G_WANDER;
            else if (!strcmp(g, "home", true)) goal = G_HOME; else if (!strcmp(g, "idle", true)) goal = G_IDLE;
            if (goal == G_NONE || AI_IsDriver(a)) return AI_Msg(playerid, "[AI] Цели: eat, work, social, wander, home, idle (не для водителей).");
            Conv_End(a); A[a][aChatWith] = INVALID_PLAYER_ID; FCNPC_Stop(A[a][aNpc]); Body_Exit(a);
            Brain_SetGoal(a, goal, 100.0);
            A[a][aGoalSince] = gettime() + 600;   // приказ: не передумывать 10 минут
            format(s, sizeof s, "[AI] %s: новая цель — %s.", A[a][aName], gGoalName[goal]);
            return AI_Msg(playerid, s);
        }
        if (!strcmp(act, "memory", true))
        {
            static const kinds[][] = {"-", "знакомство", "нападение", "разговор", "спрашивал дорогу", "оскорбление", "благодарность", "помощь"};
            new n = 0;
            for (new i = 0; i < AI_MEM; i++)
            {
                if (Mem[a][i][mKind] == MEM_NONE) continue;
                format(s, sizeof s, "- %s: %s (важность %d, %d мин назад)", kinds[Mem[a][i][mKind]], Mem[a][i][mWho], Mem[a][i][mImp], (gettime() - Mem[a][i][mTime]) / 60);
                AI_Msg(playerid, s); n++;
            }
            if (!n) AI_Msg(playerid, "[AI] Память пуста.");
            return 1;
        }
        if (!strcmp(act, "rel", true))
        {
            new n = 0;
            for (new i = 0; i < AI_REL; i++)
            {
                if (!Rel[a][i][rUsed]) continue;
                format(s, sizeof s, "- %s: симпатия %d, знакомство %d", Rel[a][i][rWho], Rel[a][i][rAff], Rel[a][i][rFam]);
                AI_Msg(playerid, s); n++;
            }
            if (!n) AI_Msg(playerid, "[AI] Пока ни с кем не знаком.");
            return 1;
        }
        if (!strcmp(act, "stop", true)) { A[a][aStopped] = true; AI_Logout(a, "остановлен админом"); return AI_Msg(playerid, "[AI] Житель вышел и не будет заходить до /aibot ... start."); }
        if (!strcmp(act, "start", true)) { A[a][aStopped] = false; return AI_Msg(playerid, "[AI] Житель снова живёт по расписанию."); }
        if (!strcmp(act, "debug", true))
        {
            A[a][aDebug] = !A[a][aDebug];
            format(s, sizeof s, "[AI] Отладка %s: %s.", A[a][aName], A[a][aDebug] ? ("вкл (подпись над головой)") : ("выкл"));
            return AI_Msg(playerid, s);
        }
        return AI_Msg(playerid, "[AI] Неизвестное действие.");
    }
    return 0;
}

// подпись над головой в режиме отладки
stock AI_DebugBubble(a)
{
    new s[96];
    format(s, sizeof s, "[%s] задача %d/%d, сыт %.0f, бодр %.0f", gGoalName[A[a][aGoal]], A[a][aPlanPos], A[a][aPlanLen], gNeed[a][N_HUNGER], gNeed[a][N_ENERGY]);
    SetPlayerChatBubble(A[a][aNpc], s, 0x7FB2FFFF, 25.0, 2200);
}
