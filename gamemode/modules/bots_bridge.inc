// =====================================================================
//  Мост с filterscript godjo_bots (FCNPC): реплики ботов идут в
//  обычный локальный чат, у ботов есть уровень и пинг как у игроков.
// =====================================================================
new gBotLevel[MAX_PLAYERS], gBotPing[MAX_PLAYERS];

forward GM_BotRegister(npcid, level, ping);
public GM_BotRegister(npcid, level, ping)
{
    if (npcid < 0 || npcid >= MAX_PLAYERS) return 0;
    gBotLevel[npcid] = level; gBotPing[npcid] = ping;
    SetPlayerScore(npcid, level);
    return 1;
}

forward GM_BotSay(npcid, const text[]);
public GM_BotSay(npcid, const text[])
{
    if (!IsPlayerConnected(npcid)) return 0;
    new s[160];
    format(s, sizeof s, "%s[%d] говорит: %s", PName(npcid), npcid, text);
    ProxMsg(npcid, 20.0, C_WHITE, s);
    new b[144]; strcpy(b, text);
    SetPlayerChatBubble(npcid, b, 0xFFFFFFFF, 20.0, 5000);
    return 1;
}

// игрок напал на мирного жителя
forward GM_BotAttacked(npcid, playerid);
public GM_BotAttacked(npcid, playerid)
{
    if (!IsLogged(playerid) || IsCop(playerid)) return 0;
    if (GetPVarInt(playerid, "botHitCD") > gettime()) return 0;
    SetPVarInt(playerid, "botHitCD", gettime() + 60);
    Police_AddWanted(playerid, 1, "нападение на гражданина");
    return 1;
}

stock Bot_Level(id) return IsPlayerNPC(id) ? gBotLevel[id] : P[id][pLevel];
stock Bot_Ping(id) return IsPlayerNPC(id) ? gBotPing[id] - 10 + random(21) : GetPlayerPing(id);

stock Iter_OnlineAll()
{
    new c = 0;
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsPlayerConnected(i) && (IsPlayerNPC(i) || P[i][pLogged])) c++;
    return c;
}

CMD:bots(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new n = 0, s[96];
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsPlayerConnected(i) && IsPlayerNPC(i)) n++;
    format(s, sizeof s, "[A] Ботов (FCNPC) на сервере: %d. Управление: /rcon unloadfs godjo_bots | loadfs godjo_bots", n);
    return SCM(playerid, C_ADMIN, s);
}
