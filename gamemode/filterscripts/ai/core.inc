// =====================================================================
//  godjo_ai — ядро: настройки, структуры агентов, общие утилиты
// =====================================================================
#define AI_POP            40      // жителей в базе (население)
#define AI_MAX_ONLINE_DEF 18      // одновременно онлайн по умолчанию (пешеходы + водители)
#define AI_MAX_ONLINE_CAP 28      // жёсткий предел (maxnpc в server.cfg = 30)
#define AI_TICK_MS        250     // такт тела (движение, разговоры)
#define AI_THINK_BUDGET   4       // сколько «мозгов» думает за такт (колесо времени)
#define AI_PATH_MAX       180     // узлов в плане пути
#define AI_MEM            16      // воспоминаний в памяти агента
#define AI_REL            12      // отношений в кэше агента
#define AI_PLAN           8       // задач в плане
#define AI_LASTREPLY      6       // анти-повтор: помним последние реплики
#define AI_HIDDEN_VW      9990    // «внутри здания» — агент невидим
#define AI_LOD_NEAR       60.0    // LOD0: рядом с игроком — полная реактивность

// цели (Utility AI)
enum { G_NONE, G_IDLE, G_WANDER, G_EAT, G_WORK, G_SOCIAL, G_HOME, G_FLEE, G_CHAT, G_COUNT }
// задачи плана (HTN-lite: цель раскрывается в цепочку задач)
enum { T_NONE, T_MOVE, T_ANIM, T_INSIDE, T_SOCIAL, T_SWEEP, T_DRIVE, T_LOGOUT, T_WAIT }
// работы
enum { JOB_NONE, JOB_SWEEPER, JOB_OFFICE, JOB_SELLER, JOB_COURIER, JOB_DRIVER, JOB_TAXI, JOB_BUS, JOB_COUNT }
// потребности (100 = полностью удовлетворена)
#define N_HUNGER 0
#define N_ENERGY 1
#define N_SOCIAL 2
#define N_FUN    3
#define N_COUNT  4   // (не enum: индекс-константа enum снимает тег Float у элемента)
// черты характера 0..100
enum { TR_SOCIAL, TR_DILIGENT, TR_CURIOUS, TR_TEMPER, TR_POLITE, TR_COUNT }
// виды воспоминаний
enum { MEM_NONE, MEM_MET, MEM_ATTACKED, MEM_TALKED, MEM_ASKED_WAY, MEM_INSULTED, MEM_THANKED, MEM_HELPED }

enum E_AGENT
{
    aId, aAcc, aName[MAX_PLAYER_NAME], aFirst[16], aSex, aSkin, aLevel, aPing,
    aTrait[TR_COUNT], aMoney, aJob, aWorkFrom, aWorkTo, aWake, aSleep,
    aHome, aWork,                                   // узлы графа
    bool:aOnline, aNpc, aSessionEnd, aSessionRow, aNextLogin, bool:aStopped, bool:aDebug,
    aGoal, aGoalSince, aGoalMin, Float:aGoalScore,
    aPlan[AI_PLAN], aPlanArg[AI_PLAN], aPlanLen, aPlanPos, aTaskUntil,
    aPathLen, aPathPos, aMoveType, aDest,           // текущий путь
    aVeh, aDrivePath, aDrivePt,
    aConv, aChatWith, aChatUntil, aLastGreet, aThinkAt, aSaveAt, aLastReply[AI_LASTREPLY], aLastReplyPos,
    aFleeUntil, aStuck, Float:aLastX, Float:aLastY, aNeedsAt
}
new A[AI_POP][E_AGENT];
new Float:gNeed[AI_POP][N_COUNT];   // потребности (отдельный массив: у полей enum теряется тег Float)
new gAgentCount;
new gAgentOf[MAX_PLAYERS] = {-1, ...};
new gPath[AI_POP][AI_PATH_MAX];
new gTick, gTimer, gMaxOnline = AI_MAX_ONLINE_DEF, bool:gEnabled = true, gThinkCursor;
new DB:gAIDB;

// память и отношения (кэш в RAM, хранение — ai_memory / ai_relations)
enum E_MEM { mKind, mWho[MAX_PLAYER_NAME], mValue, mImp, mTime }
new Mem[AI_POP][AI_MEM][E_MEM];
enum E_REL { bool:rUsed, rWho[MAX_PLAYER_NAME], rAff, rFam, rSeen }
new Rel[AI_POP][AI_REL][E_REL];

stock const gGoalName[G_COUNT][] = {"-", "отдых", "прогулка", "еда", "работа", "общение", "домой", "бегство", "разговор"};
stock const gJobName[JOB_COUNT][] = {"без работы", "дворник", "офис", "продавец", "курьер", "водитель", "таксист", "водитель автобуса"};

// ---------------------------------------------------------------- утилиты
stock Float:AI_Dist2(Float:ax, Float:ay, Float:bx, Float:by) return floatsqroot((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
stock AI_Clamp(v, lo, hi) return v < lo ? lo : v > hi ? hi : v;
stock Float:AI_ClampF(Float:v, Float:lo, Float:hi) return v < lo ? lo : v > hi ? hi : v;
stock AI_Hour() { new h, m, s; gettime(h, m, s); return h; }
stock AI_Now() return gettime();

// нижний регистр для CP1251 (А-Я, Ё) и латиницы; ё -> е для простоты сравнения
stock AI_Lower(dest[], const src[], size = sizeof dest)
{
    new i = 0;
    for (; src[i] && i < size - 1; i++)
    {
        new c = src[i] & 0xFF;
        if (c >= 'A' && c <= 'Z') c += 32;
        else if (c >= 0xC0 && c <= 0xDF) c += 32;
        else if (c == 0xA8 || c == 0xB8) c = 0xE5;
        dest[i] = c;
    }
    dest[i] = 0;
}

// есть ли в (уже нормализованном) тексте ключ из списка "a|b|c"
stock bool:AI_Has(const low[], const keys[])
{
    new k[48], n = 0;
    for (new i = 0; ; i++)
    {
        new c = keys[i];
        if (c == '|' || c == 0)
        {
            k[n] = 0;
            if (n)
            {
                new kl[48]; AI_Lower(kl, k);
                if (strfind(low, kl) != -1) return true;
            }
            n = 0;
            if (c == 0) break;
            continue;
        }
        if (n < sizeof k - 1) k[n++] = c;
    }
    return false;
}

stock AI_Exec(const q[])
{
    new DBResult:r = db_query(gAIDB, q);
    if (r) db_free_result(r);
}

stock AI_Esc(dest[], const src[], size = sizeof dest)
{
    new j = 0;
    for (new i = 0; src[i] && j < size - 2; i++)
    {
        if (src[i] == '\'') dest[j++] = '\'';
        dest[j++] = src[i];
    }
    dest[j] = 0;
}

stock AI_AdminLevel(playerid) return CallRemoteFunction("GM_AdminLevel", "i", playerid);
forward bool:Conv_TryStart(a);
stock bool:AI_IsRealPlayer(playerid) return IsPlayerConnected(playerid) && !IsPlayerNPC(playerid) && CallRemoteFunction("GM_IsLogged", "i", playerid) != 0;
