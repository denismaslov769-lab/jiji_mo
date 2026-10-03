// =====================================================================
//  godjo_ai — социальное: память, отношения, разговоры, ответы игрокам
// =====================================================================
stock AI_Say(a, const text[])
{
    if (!A[a][aOnline]) return;
    CallRemoteFunction("GM_BotSay", "is", A[a][aNpc], text);
}

forward AI_DelayedSay(a, id, const text[]);
public AI_DelayedSay(a, id, const text[])
{
    if (a < 0 || a >= gAgentCount || A[a][aId] != id || !A[a][aOnline]) return;
    if (!IsPlayerInAnyVehicle(A[a][aNpc])) FCNPC_ApplyAnimation(A[a][aNpc], "PED", "IDLE_chat", 4.1, 0, 1, 1, 1, 1);
    AI_Say(a, text);
}

// ---------------------------------------------------------------- память
stock Mem_Add(a, kind, const who[], value, imp)
{
    new slot = -1;
    for (new i = 0; i < AI_MEM; i++) if (Mem[a][i][mKind] == MEM_NONE) { slot = i; break; }
    if (slot == -1)
    {   // вытесняем самое неважное, при равенстве — самое старое
        slot = 0;
        for (new i = 1; i < AI_MEM; i++)
            if (Mem[a][i][mImp] < Mem[a][slot][mImp] || (Mem[a][i][mImp] == Mem[a][slot][mImp] && Mem[a][i][mTime] < Mem[a][slot][mTime])) slot = i;
    }
    Mem[a][slot][mKind] = kind; format(Mem[a][slot][mWho], MAX_PLAYER_NAME, "%s", who);
    Mem[a][slot][mValue] = value; Mem[a][slot][mImp] = imp; Mem[a][slot][mTime] = gettime();
    if (imp >= 3)
    {
        new q[200], e[MAX_PLAYER_NAME * 2]; AI_Esc(e, who);
        format(q, sizeof q, "INSERT INTO ai_memory (agent,kind,who,value,imp,t) VALUES (%d,%d,'%s',%d,%d,%d)", A[a][aId], kind, e, value, imp, gettime());
        AI_Exec(q);
    }
}

stock Mem_Find(a, kind, const who[], maxAge = 0)
{
    new best = -1;
    for (new i = 0; i < AI_MEM; i++)
    {
        if (Mem[a][i][mKind] != kind || strcmp(Mem[a][i][mWho], who, true)) continue;
        if (maxAge && gettime() - Mem[a][i][mTime] > maxAge) continue;
        if (best == -1 || Mem[a][i][mTime] > Mem[a][best][mTime]) best = i;
    }
    return best;
}

stock Mem_Load(a)
{
    for (new i = 0; i < AI_MEM; i++) Mem[a][i][mKind] = MEM_NONE;
    new q[160]; format(q, sizeof q, "SELECT kind,who,value,imp,t FROM ai_memory WHERE agent=%d ORDER BY t DESC LIMIT %d", A[a][aId], AI_MEM);
    new DBResult:r = db_query(gAIDB, q);
    if (!r) return;
    new k = 0, now = gettime();
    for (new i = 0, n = db_num_rows(r); i < n && k < AI_MEM; i++, db_next_row(r))
    {
        new t = db_get_field_int(r, 4), imp = db_get_field_int(r, 3) - (now - t) / 86400;   // забывание: -1 важности в сутки
        if (imp <= 0) continue;
        Mem[a][k][mKind] = db_get_field_int(r, 0); db_get_field(r, 1, Mem[a][k][mWho], MAX_PLAYER_NAME);
        Mem[a][k][mValue] = db_get_field_int(r, 2); Mem[a][k][mImp] = imp; Mem[a][k][mTime] = t;
        k++;
    }
    db_free_result(r);
}

// ---------------------------------------------------------------- отношения
stock Rel_Load(a)
{
    for (new i = 0; i < AI_REL; i++) Rel[a][i][rUsed] = false;
    new q[160]; format(q, sizeof q, "SELECT who,aff,fam,seen FROM ai_relations WHERE agent=%d ORDER BY seen DESC LIMIT %d", A[a][aId], AI_REL);
    new DBResult:r = db_query(gAIDB, q);
    if (!r) return;
    for (new i = 0, n = db_num_rows(r); i < n && i < AI_REL; i++, db_next_row(r))
    {
        Rel[a][i][rUsed] = true; db_get_field(r, 0, Rel[a][i][rWho], MAX_PLAYER_NAME);
        Rel[a][i][rAff] = db_get_field_int(r, 1); Rel[a][i][rFam] = db_get_field_int(r, 2); Rel[a][i][rSeen] = db_get_field_int(r, 3);
    }
    db_free_result(r);
}

stock Rel_Get(a, const who[])
{
    new oldest = 0;
    for (new i = 0; i < AI_REL; i++)
    {
        if (Rel[a][i][rUsed] && !strcmp(Rel[a][i][rWho], who, true)) return i;
        if (!Rel[a][i][rUsed]) { oldest = i; break; }
        if (Rel[a][i][rSeen] < Rel[a][oldest][rSeen]) oldest = i;
    }
    // нет в кэше — подгружаем из базы или заводим новое
    new i = oldest;
    Rel[a][i][rUsed] = true; format(Rel[a][i][rWho], MAX_PLAYER_NAME, "%s", who); Rel[a][i][rAff] = 0; Rel[a][i][rFam] = 0; Rel[a][i][rSeen] = gettime();
    new q[200], e[MAX_PLAYER_NAME * 2]; AI_Esc(e, who);
    format(q, sizeof q, "SELECT aff,fam FROM ai_relations WHERE agent=%d AND who='%s'", A[a][aId], e);
    new DBResult:r = db_query(gAIDB, q);
    if (r)
    {
        if (db_num_rows(r)) { Rel[a][i][rAff] = db_get_field_int(r, 0); Rel[a][i][rFam] = db_get_field_int(r, 1); }
        db_free_result(r);
    }
    return i;
}

stock Rel_Change(a, const who[], daff, dfam)
{
    new i = Rel_Get(a, who);
    Rel[a][i][rAff] = AI_Clamp(Rel[a][i][rAff] + daff, -100, 100);
    Rel[a][i][rFam] = AI_Clamp(Rel[a][i][rFam] + dfam, 0, 1000);
    Rel[a][i][rSeen] = gettime();
    new q[220], e[MAX_PLAYER_NAME * 2]; AI_Esc(e, who);
    format(q, sizeof q, "INSERT OR REPLACE INTO ai_relations (agent,who,aff,fam,seen) VALUES (%d,'%s',%d,%d,%d)", A[a][aId], e, Rel[a][i][rAff], Rel[a][i][rFam], gettime());
    AI_Exec(q);
    return i;
}

// ---------------------------------------------------------------- разговоры агентов между собой
#define MAX_CONV 8
enum E_CONV { bool:cUsed, cA, cB, cTalk, cLine, cNext }
new C[MAX_CONV][E_CONV];

stock bool:Conv_TryStart(a)
{
    if (A[a][aConv] != -1 || A[a][aChatWith] != INVALID_PLAYER_ID || IsPlayerInAnyVehicle(A[a][aNpc])) return false;
    new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
    for (new o = 0; o < gAgentCount; o++)
    {
        if (o == a || !A[o][aOnline] || A[o][aConv] != -1 || A[o][aChatWith] != INVALID_PLAYER_ID || AI_IsDriver(o)) continue;
        new t = Body_CurTask(o);
        if (t != T_SOCIAL && t != T_ANIM && t != T_WAIT) continue;
        new Float:ox, Float:oy, Float:oz; FCNPC_GetPosition(A[o][aNpc], ox, oy, oz);
        if (AI_Dist2(x, y, ox, oy) > 12.0) continue;
        for (new c = 0; c < MAX_CONV; c++)
        {
            if (C[c][cUsed]) continue;
            C[c][cUsed] = true; C[c][cA] = a; C[c][cB] = o; C[c][cTalk] = random(sizeof gTalk); C[c][cLine] = 0; C[c][cNext] = gTick + 8;
            A[a][aConv] = c; A[o][aConv] = c;
            FCNPC_Stop(A[o][aNpc]); FCNPC_ClearAnimations(A[o][aNpc]); FCNPC_ClearAnimations(A[a][aNpc]);
            new Float:mx = (x + ox) / 2.0, Float:my = (y + oy) / 2.0, Float:dx = ox - x, Float:dy = oy - y, Float:l = floatsqroot(dx * dx + dy * dy);
            if (l < 0.1) l = 0.1;
            FCNPC_GoTo(A[a][aNpc], mx - dx / l * 0.7, my - dy / l * 0.7, z, FCNPC_MOVE_TYPE_WALK, FCNPC_MOVE_SPEED_AUTO, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
            FCNPC_GoTo(A[o][aNpc], mx + dx / l * 0.7, my + dy / l * 0.7, oz, FCNPC_MOVE_TYPE_WALK, FCNPC_MOVE_SPEED_AUTO, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
            new ka[MAX_PLAYER_NAME], kb[MAX_PLAYER_NAME];
            format(ka, sizeof ka, "%s", A[a][aName]); format(kb, sizeof kb, "%s", A[o][aName]);
            Rel_Change(a, kb, 1, 1); Rel_Change(o, ka, 1, 1);
            return true;
        }
        return false;
    }
    return false;
}

stock Conv_Tick(c)
{
    if (gTick < C[c][cNext]) return;
    new t = C[c][cTalk], l = C[c][cLine], a = C[c][cA], o = C[c][cB];
    if (l >= 6 || !gTalk[t][l][0] || !A[a][aOnline] || !A[o][aOnline]) { Conv_Finish(c); return; }
    if (l == 0)
    {
        new Float:x, Float:y, Float:z, Float:ox, Float:oy, Float:oz;
        FCNPC_GetPosition(A[a][aNpc], x, y, z); FCNPC_GetPosition(A[o][aNpc], ox, oy, oz);
        FCNPC_SetAngleToPos(A[a][aNpc], ox, oy); FCNPC_SetAngleToPos(A[o][aNpc], x, y);
    }
    new sp = (l % 2 == 0) ? a : o;
    AI_Say(sp, gTalk[t][l]);
    FCNPC_ApplyAnimation(A[sp][aNpc], "PED", "IDLE_chat", 4.1, 0, 1, 1, 1, 1);
    C[c][cLine]++;
    C[c][cNext] = gTick + 16 + strlen(gTalk[t][l]) / 3;
}

stock Conv_Finish(c)
{
    new a = C[c][cA], o = C[c][cB];
    C[c][cUsed] = false;
    if (A[a][aConv] == c) { A[a][aConv] = -1; gNeed[a][N_SOCIAL] = 100.0; if (A[a][aOnline]) Body_StartTask(a); }
    if (A[o][aConv] == c) { A[o][aConv] = -1; gNeed[o][N_SOCIAL] = 100.0; if (A[o][aOnline]) Body_StartTask(o); }
}

stock Conv_End(a)
{
    new c = A[a][aConv];
    if (c != -1 && C[c][cUsed]) Conv_Finish(c);
    A[a][aConv] = -1;
}

// ---------------------------------------------------------------- ответы игрокам
static sFillName[MAX_PLAYER_NAME], sFillPlace[48], sFillDir[32], sFillDist[24], sFillDoing[64];

stock AI_Fill(a, out[], const tpl[], size = sizeof out)
{
    new j = 0;
    for (new i = 0; tpl[i] && j < size - 1; i++)
    {
        if (tpl[i] == '{')
        {
            new key[8], k = 0, p = i + 1;
            while (tpl[p] && tpl[p] != '}' && k < sizeof key - 1) key[k++] = tpl[p++];
            key[k] = 0;
            if (tpl[p] == '}')
            {
                new val[64];
                if (!strcmp(key, "n")) format(val, sizeof val, "%s", sFillName);
                else if (!strcmp(key, "me")) format(val, sizeof val, "%s", A[a][aFirst]);
                else if (!strcmp(key, "place")) format(val, sizeof val, "%s", sFillPlace);
                else if (!strcmp(key, "dir")) format(val, sizeof val, "%s", sFillDir);
                else if (!strcmp(key, "dist")) format(val, sizeof val, "%s", sFillDist);
                else if (!strcmp(key, "doing")) format(val, sizeof val, "%s", sFillDoing);
                else if (!strcmp(key, "a")) format(val, sizeof val, "%s", A[a][aSex] ? ("а") : (""));
                for (new v = 0; val[v] && j < size - 1; v++) out[j++] = val[v];
                i = p;
                continue;
            }
        }
        out[j++] = tpl[i];
    }
    out[j] = 0;
    // в начале фразы — заглавная (для подстановок вроде «{doing}»)
    new c = out[0] & 0xFF;
    if (c >= 0xE0 && c <= 0xFF) out[0] = c - 32; else if (c >= 'a' && c <= 'z') out[0] = c - 32;
}

stock AI_Direction(Float:dx, Float:dy, out[], size = sizeof out)
{
    static const dirs[8][] = {"на север", "на северо-запад", "на запад", "на юго-запад", "на юг", "на юго-восток", "на восток", "на северо-восток"};
    new Float:ang = atan2(dy, dx) - 90.0;   // 0 = север, против часовой
    while (ang < 0.0) ang += 360.0;
    new k = floatround(ang / 45.0) % 8;
    format(out, size, "%s", dirs[k]);
}

stock AI_Intent(const low[], &place)
{
    place = -1;
    if (AI_Has(low, "где|как пройти|как дойти|как добраться|подскаж"))
    {
        for (new p = 0; p < sizeof gPlace; p++) if (AI_Has(low, gPlace[p][plKeys])) { place = p; break; }
        return I_WHERE;
    }
    if (AI_Has(low, "бот|нпс|npc|робот|ии ")) return I_BOT;
    if (AI_Has(low, "дурак|идиот|лох|тупой|урод|придурок|дебил|чмо|пошел ты|иди нах")) return I_INSULT;
    if (AI_Has(low, "спасибо|спс|благодар|thx|thanks")) return I_THANKS;
    if (AI_Has(low, "пока|до свидан|бывай|увидимся|bye")) return I_BYE;
    if (AI_Has(low, "как дела|как ты|как жизнь|как сам|как поживаешь")) return I_HOW;
    if (AI_Has(low, "как зовут|как тебя|твое имя|ты кто|кто ты")) return I_NAME;
    if (AI_Has(low, "что делаешь|чем занят|куда идешь|куда собрался|что тут делаешь")) return I_DOING;
    if (AI_Has(low, "работ|заработ|подработ")) return I_JOB;
    if (AI_Has(low, "денег|деньги|займи|одолжи|дай бабл|подкинь")) return I_MONEY;
    if (AI_Has(low, "привет|здравств|здаров|здоров|хай|салам|добрый|доброе|доброй|hi|hello|ку ")) return I_GREET;
    return I_UNKNOWN;
}

stock AI_PickReply(a, intent, style, out[], size = sizeof out)
{
    new opts[4], n = 0;
    for (new k = 0; k < 4; k++)
    {
        if (!gReply[intent][style][k][0]) continue;
        new id = intent * 16 + style * 4 + k, bool:recent = false;
        for (new r = 0; r < AI_LASTREPLY; r++) if (A[a][aLastReply][r] == id + 1) recent = true;
        if (!recent) opts[n++] = k;
    }
    if (n == 0) for (new k = 0; k < 4; k++) if (gReply[intent][style][k][0]) opts[n++] = k;
    if (n == 0) { out[0] = 0; return; }
    new k = opts[random(n)];
    A[a][aLastReply][A[a][aLastReplyPos]] = intent * 16 + style * 4 + k + 1;
    A[a][aLastReplyPos] = (A[a][aLastReplyPos] + 1) % AI_LASTREPLY;
    AI_Fill(a, out, gReply[intent][style][k], size);
}

forward AI_OnPlayerText(playerid, const text[]);
public AI_OnPlayerText(playerid, const text[])
{
    if (!gEnabled || IsPlayerNPC(playerid)) return 0;
    new low[144]; AI_Lower(low, text);
    new Float:px, Float:py, Float:pz; GetPlayerPos(playerid, px, py, pz);
    new vw = GetPlayerVirtualWorld(playerid), best = -1, Float:bd = 6.0, bool:named = false;
    for (new a = 0; a < gAgentCount; a++)
    {
        if (!A[a][aOnline] || AI_IsDriver(a) || FCNPC_GetVirtualWorld(A[a][aNpc]) != vw) continue;
        new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
        new Float:d = AI_Dist2(px, py, x, y);
        if (d > 15.0) continue;
        new fl[20]; AI_Lower(fl, A[a][aFirst]);
        if (strfind(low, fl) != -1) { best = a; bd = d; named = true; break; }
        if (d < bd) { bd = d; best = a; }
    }
    if (best == -1) return 0;
    new a = best, place, intent = AI_Intent(low, place);
    if (intent == I_UNKNOWN && !named && (bd > 2.5 || random(3))) return 0;     // разговор не с ним
    new pname[MAX_PLAYER_NAME]; GetPlayerName(playerid, pname, sizeof pname);
    format(sFillName, sizeof sFillName, "%s", pname);
    new us = strfind(sFillName, "_"); if (us > 0) sFillName[us] = 0;
    new ri = Rel_Get(a, pname), aff = Rel[a][ri][rAff], fam = Rel[a][ri][rFam];
    // желание отвечать: характер + отношение + занятость
    new w = 40 + A[a][aTrait][TR_SOCIAL] / 2 + aff, bool:busy = (A[a][aGoal] == G_WORK && Body_CurTask(a) == T_MOVE) || A[a][aGoal] == G_HOME || A[a][aGoal] == G_FLEE;
    if (busy) w -= 30;
    if (named) w += 15;
    if (Mem_Find(a, MEM_ATTACKED, pname, 3 * 86400) != -1) intent = I_HOSTILE;
    else if (intent == I_GREET && fam > 0) intent = I_GREET_AGAIN;
    else if (intent == I_WHERE && place == -1) intent = I_WHERE_UNKNOWN;
    if (intent != I_HOSTILE && w < 25)
    {
        if (!busy && random(2)) return 0;   // просто не ответил — бывает
        intent = I_BUSY;
    }
    // стиль речи
    new style = 1;
    if (A[a][aTrait][TR_POLITE] >= 65) style = 0;
    if (A[a][aTrait][TR_POLITE] < 25 || aff < -20 || (intent == I_INSULT && A[a][aTrait][TR_TEMPER] > 60)) style = 2;
    // знания: куда идти
    if (intent == I_WHERE)
    {
        new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
        new Float:dx = gPlace[place][plX] - x, Float:dy = gPlace[place][plY] - y, Float:d = floatsqroot(dx * dx + dy * dy);
        format(sFillPlace, sizeof sFillPlace, "%s", gPlace[place][plTitle]);
        AI_Direction(dx, dy, sFillDir);
        if (d < 40.0) { sFillDir = "прямо тут"; format(sFillDist, sizeof sFillDist, "пара шагов"); }
        else if (d < 1000.0) format(sFillDist, sizeof sFillDist, "%d м", (floatround(d) + 25) / 50 * 50);
        else format(sFillDist, sizeof sFillDist, "%d км", (floatround(d) + 500) / 1000);
        Mem_Add(a, MEM_ASKED_WAY, pname, place, 1);
    }
    Brain_Doing(a, sFillDoing);
    new reply[144]; AI_PickReply(a, intent, style, reply);
    if (!reply[0]) return 0;
    // «живая» небрежность: в простом стиле иногда без точки и с маленькой буквы
    if (style >= 1 && random(3) == 0)
    {
        new l = strlen(reply); if (l > 1 && reply[l - 1] == '.') reply[l - 1] = 0;
        new c = reply[0] & 0xFF; if (c >= 0xC0 && c <= 0xDF) reply[0] = c + 32;
    }
    // последствия для отношений и памяти
    switch (intent)
    {
        case I_INSULT: { Rel_Change(a, pname, -12, 1); Mem_Add(a, MEM_INSULTED, pname, 0, 4); }
        case I_THANKS: { Rel_Change(a, pname, 4, 1); Mem_Add(a, MEM_THANKED, pname, 0, 2); }
        default: Rel_Change(a, pname, intent == I_HOSTILE ? 0 : 1, 1);
    }
    if (fam == 0) Mem_Add(a, MEM_MET, pname, 0, 3);
    // остановиться, повернуться к собеседнику, «печатать» ответ
    if (A[a][aConv] == -1 && A[a][aGoal] != G_FLEE)
    {
        FCNPC_Stop(A[a][aNpc]);
        FCNPC_SetAngleToPos(A[a][aNpc], px, py);
        A[a][aChatWith] = playerid;
        A[a][aChatUntil] = gTick + (intent == I_BYE || intent == I_BUSY || intent == I_HOSTILE ? 10 : 56);
        gNeed[a][N_SOCIAL] = AI_ClampF(gNeed[a][N_SOCIAL] + 8.0, 0.0, 100.0);
    }
    SetTimerEx("AI_DelayedSay", 900 + strlen(reply) * 45 + random(900), false, "iis", a, A[a][aId], reply);
    return 1;
}

// приветствие знакомых и (редко) незнакомых прохожих
stock Social_GreetNear(a)
{
    if (gTick - A[a][aLastGreet] < 240 || IsPlayerInAnyVehicle(A[a][aNpc])) return;
    new Float:x, Float:y, Float:z; FCNPC_GetPosition(A[a][aNpc], x, y, z);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i) || IsPlayerNPC(i) || IsPlayerInAnyVehicle(i) || GetPlayerVirtualWorld(i) != FCNPC_GetVirtualWorld(A[a][aNpc])) continue;
        if (!IsPlayerInRangeOfPoint(i, 4.0, x, y, z)) continue;
        new pname[MAX_PLAYER_NAME]; GetPlayerName(i, pname, sizeof pname);
        A[a][aLastGreet] = gTick;
        format(sFillName, sizeof sFillName, "%s", pname);
        new us = strfind(sFillName, "_"); if (us > 0) sFillName[us] = 0;
        new ri = Rel_Get(a, pname), reply[144], style = A[a][aTrait][TR_POLITE] >= 65 ? 0 : A[a][aTrait][TR_POLITE] < 25 ? 2 : 1;
        if (Mem_Find(a, MEM_ATTACKED, pname, 3 * 86400) != -1) { AI_PickReply(a, I_HOSTILE, style, reply); AI_Say(a, reply); return; }
        if (Rel[a][ri][rFam] >= 2 && Rel[a][ri][rAff] >= 0) { AI_PickReply(a, I_GREET_AGAIN, style, reply); AI_Say(a, reply); Rel_Change(a, pname, 0, 1); return; }
        if (random(100) < A[a][aTrait][TR_SOCIAL] / 6) { AI_PickReply(a, I_GREET, style, reply); AI_Say(a, reply); }
        return;
    }
}
