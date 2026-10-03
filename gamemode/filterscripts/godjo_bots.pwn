// =====================================================================
//  Godjo RP — «жизнь на сервере»: боты FCNPC, похожие на игроков.
//  У каждого бота свой ID, RP-ник, уровень и пинг (через мод), они
//  гуляют по тротуарам (точки сняты с путевых узлов GTA SA), бегают,
//  прыгают, работают, водят машины и автобус, болтают друг с другом
//  и отвечают игрокам, которые к ним обращаются.
//  Кодировка исходника: UTF-8 (при сборке перекодируется в CP1251).
// =====================================================================
#define FILTERSCRIPT
#pragma warning disable 239
#pragma warning disable 214
#pragma warning disable 208
#include <a_samp>
#include <FCNPC>
#include "bots_paths.inc"

#define BOT_WALKERS    20
#define BOT_DRIVERS    6
#define MAX_BOTS       (BOT_WALKERS + BOT_DRIVERS)
#define TICK_MS        500

enum { BS_NONE, BS_WALK, BS_PAUSE, BS_TALK, BS_FLEE, BS_DRIVE, BS_WAITCAR }
enum { BK_WALKER, BK_JOGGER, BK_SWEEPER, BK_PHONE, BK_DRIVER, BK_TAXI, BK_BUS }
enum E_BOT
{
    bool:bUsed, bNpc, bKind, bState, bPath, bPt, bDir, bUntil, bVeh, bConv, bSex,
    bName[MAX_PLAYER_NAME], bFirst[16], bLastGreet, bRespawnAt, bSkin
}
new B[MAX_BOTS][E_BOT];
new gBotOf[MAX_PLAYERS] = {-1, ...};
new gTick, gTimer;

// ---------------- имена и скины ----------------
new const gMaleFirst[][] = {"Ivan","Dmitriy","Sergey","Alexey","Maxim","Artem","Nikita","Egor","Kirill","Denis","Anton","Roman","Pavel","Andrey","Oleg","Vlad","Timur","Ruslan","Arthur","Mark","Daniil","Ilya","Gleb","Matvey","Stepan","Tony","Carlos","Jason","Kevin","Marcus","Luis","Frank"};
new const gFemFirst[][] = {"Anna","Maria","Elena","Olga","Daria","Alina","Kristina","Polina","Sofia","Victoria","Ekaterina","Yulia","Vera","Milana","Arina","Eva","Jessica","Linda","Kate","Monica"};
new const gLastRu[][] = {"Petrov","Smirnov","Kuznetsov","Popov","Volkov","Sokolov","Lebedev","Kozlov","Novikov","Morozov","Orlov","Belov","Zaitsev","Pavlov","Semenov","Golubev","Vinogradov","Bogdanov","Vorobiev","Fedorov","Mikhailov","Tarasov","Belyaev","Komarov","Kiselev","Makarov","Andreev","Kovalev","Ilyin","Gusev"};
new const gLastEn[][] = {"Johnson","Miller","Walker","Lopez","Carter","Brooks","Turner","Collins","Reed","Hayes","Morgan","Price","Foster","Russo","Moretti","Vance"};
new const gMaleSkins[] = {7,14,15,17,20,21,22,23,24,25,26,29,30,32,34,35,36,37,44,46,47,48,57,58,59,60,66,67,72,73,94,95,98,101,128,170,180,184,185,186,187,188,206,217,223,240,250,261};
new const gFemSkins[] = {9,10,11,12,13,31,39,40,41,55,56,69,76,88,89,91,93,141,148,150,151,157,169,190,191,192,193,211,216,219,224,225,226,233};
new const gSportSkins[] = {96,97,45,18,154,138,139,140};
new const gCarModels[] = {400,401,404,405,410,412,418,419,421,426,436,439,445,458,466,467,474,475,479,480,489,491,492,496,507,516,517,518,526,527,529,533,534,540,542,543,545,546,547,549,550,551,554,566,567,580,585,589,600,602};

// ---------------- разговоры ботов (реплики поочерёдно A/B) ----------------
new const gTalk[][6][110] = {
    {"Здарова! Ты куда пропал?", "Да на работе завал, смены одна за другой.", "Где пашешь сейчас?", "Таксую. Клиенты на тротуарах прямо машут, удобно.", "О, тоже думаю туда пойти.", "Иди, без работы не останешься."},
    {"Слышал, в мэрии очередь опять до самого входа?", "Ага, полчаса стоял за паспортом.", "Зато фото сделали нормальное?", "Да ну, глаза закрыл, переснимали.", "", ""},
    {"Привет! Как дела?", "Нормально, вот на права готовлюсь.", "Теорию сдал уже?", "Вчера. Восемь из десяти, еле-еле.", "Практику не завали, там маршрут по городу.", "Постараюсь машину не побить."},
    {"Шаурму у Ашота пробовал новую?", "Конечно, с двойным мясом. Огонь.", "Он говорит, курьера ищет.", "Может схожу, подзаработаю.", "", ""},
    {"Блин, бензин опять подорожал.", "Да ладно, три доллара всего.", "Тебе легко говорить, у тебя мопед.", "Зато в пробках не стою, ха-ха.", "", ""},
    {"Ты в Сан-Фиерро был когда-нибудь?", "Был, поездом с Юнити, быстро доехал.", "И как там?", "Холмы, мосты, туман. Красиво, но дорого.", "А в Вегас?", "Вегас — это деньги. Там казино и стройки."},
    {"Говорят, под эстакадой старик живёт, всем помогает.", "Дядя Вова? Он мне с документами помог.", "Серьёзно?", "Ага. Хороший мужик, только жизнь его потрепала.", "", ""},
    {"Привет, ты на ферму к Михалычу не ездил?", "Ездил, урожай собирал. Спина до сих пор болит.", "Платит хоть нормально?", "Нормально, ещё и молоком угостил.", "", ""},
    {"Видел, как полиция вчера гналась за кем-то?", "Видел, по Гантону летели с сиреной.", "Поймали?", "Конечно, у них вертолёт.", "", ""},
    {"Ты в больнице медкарту делал?", "Делал, сейчас там всё по-новому: кабинеты, врачи.", "А денег нет, бесплатно можно?", "По ОМС можно, если совсем на мели.", "Отлично, завтра схожу.", "Только зрение проверь, там таблица."},
    {"Ну и жара сегодня.", "Не говори. На пляж бы.", "Санта-Мария рядом, погнали после работы?", "Давай, только переоденусь.", "", ""},
    {"Сколько на автобусе за смену получаешь?", "Нормально, плюс пассажиры за проезд платят.", "А маршрут длинный?", "Городской — минут на десять, пригородный до Монтгомери.", "", ""},
    {"Эй, у тебя телефон новый?", "Ага, в 24/7 взял.", "Скинь номер.", "Пиши: /number, сам глянешь.", "Ха, ладно.", ""},
    {"Ты семью свою уже собрал?", "Пока нет, полтинник тысяч надо на регистрацию.", "Дорого.", "Зато свой герб и общак.", "", ""},
    {"Доброе утро!", "Доброе. Кофе есть?", "Только из автомата в мэрии.", "Пойдёт, лишь бы горячий.", "", ""}
};
new const gGreet[][] = {"Привет!", "Здарова!", "Привет-привет.", "О, здравствуй!", "Хай!", "Доброго дня!"};
new const gPhoneTalk[][] = {"Да, мам, уже еду.", "Алло? Да, я у вокзала.", "Не, сегодня не смогу, работа.", "Скинь адрес, сейчас буду.", "Слушай, перезвоню позже.", "Ага, взял хлеб и молоко."};
new const gHurt[][] = {"Ай! Ты что творишь?!", "Помогите! Полиция!", "Эй, псих, отвали!", "Ты больной?!"};

// ---------------- вспомогательное ----------------
stock Bot_Say(b, const text[])
{
    if (!B[b][bUsed]) return;
    CallRemoteFunction("GM_BotSay", "is", B[b][bNpc], text);
}

stock Bot_MakeName(b)
{
    new name[MAX_PLAYER_NAME], tries = 0;
    do
    {
        new last[20];
        if (random(4) == 0) format(last, sizeof last, "%s", gLastEn[random(sizeof gLastEn)]);
        else
        {
            format(last, sizeof last, "%s", gLastRu[random(sizeof gLastRu)]);
            if (B[b][bSex]) strcat(last, "a");
        }
        if (B[b][bSex]) format(B[b][bFirst], 16, "%s", gFemFirst[random(sizeof gFemFirst)]);
        else format(B[b][bFirst], 16, "%s", gMaleFirst[random(sizeof gMaleFirst)]);
        format(name, sizeof name, "%s_%s", B[b][bFirst], last);
    }
    while (Bot_NameTaken(name) && ++tries < 20);
    format(B[b][bName], MAX_PLAYER_NAME, "%s", name);
}

stock bool:Bot_NameTaken(const name[])
{
    new n[MAX_PLAYER_NAME];
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i)) continue;
        GetPlayerName(i, n, sizeof n);
        if (!strcmp(n, name, true)) return true;
    }
    return false;
}

stock Bot_Level()
{
    new r = random(100);
    if (r < 35) return 1 + random(2);
    if (r < 70) return 3 + random(3);
    if (r < 92) return 6 + random(5);
    return 11 + random(10);
}

stock Float:Dist2D(Float:ax, Float:ay, Float:bx, Float:by) return floatsqroot((ax - bx) * (ax - bx) + (ay - by) * (ay - by));

// ---------------- создание ----------------
stock Bot_Create(b, kind)
{
    B[b][bKind] = kind; B[b][bSex] = (kind == BK_DRIVER || kind == BK_TAXI || kind == BK_BUS || kind == BK_SWEEPER) ? (random(6) == 0 ? 1 : 0) : random(2);
    if (kind == BK_TAXI || kind == BK_BUS) B[b][bSex] = 0;
    Bot_MakeName(b);
    new npc = FCNPC_Create(B[b][bName]);
    if (npc == INVALID_PLAYER_ID) return 0;
    B[b][bUsed] = true; B[b][bNpc] = npc; B[b][bConv] = -1; B[b][bVeh] = INVALID_VEHICLE_ID; B[b][bLastGreet] = 0;
    gBotOf[npc] = b;
    if (kind == BK_JOGGER) B[b][bSkin] = gSportSkins[random(sizeof gSportSkins)];
    else if (kind == BK_SWEEPER) B[b][bSkin] = 16;
    else if (kind == BK_TAXI) B[b][bSkin] = 255;
    else if (kind == BK_BUS) B[b][bSkin] = 253;
    else B[b][bSkin] = B[b][bSex] ? gFemSkins[random(sizeof gFemSkins)] : gMaleSkins[random(sizeof gMaleSkins)];
    new Float:x, Float:y, Float:z;
    if (kind >= BK_DRIVER)
    {
        B[b][bPath] = (kind == BK_BUS) ? 0 : random(sizeof gDrivePath);
        B[b][bPt] = random(gDrivePath[B[b][bPath]][1]);
        Bot_DrivePoint(b, B[b][bPt], x, y, z);
    }
    else
    {
        B[b][bPath] = (kind == BK_SWEEPER) ? 2 + random(2) : random(sizeof gWalkPath);
        B[b][bPt] = random(gWalkPath[B[b][bPath]][1]);
        B[b][bDir] = random(2) ? 1 : -1;
        Bot_WalkPoint(b, B[b][bPt], x, y, z);
    }
    FCNPC_Spawn(npc, B[b][bSkin], x, y, z);
    FCNPC_SetInvulnerable(npc, true);
    SetPlayerColor(npc, 0xFFFFFF00);
    CallRemoteFunction("GM_BotRegister", "iii", npc, Bot_Level(), 35 + random(90));
    if (kind >= BK_DRIVER)
    {
        new model = (kind == BK_TAXI) ? 420 : (kind == BK_BUS) ? 431 : gCarModels[random(sizeof gCarModels)];
        new Float:nx, Float:ny, Float:nz;
        Bot_DrivePoint(b, (B[b][bPt] + 1) % gDrivePath[B[b][bPath]][1], nx, ny, nz);
        new Float:a = atan2(nx - x, ny - y); a = -a; if (a < 0.0) a += 360.0;
        B[b][bVeh] = CreateVehicle(model, x, y, z, a, (kind == BK_TAXI) ? 6 : random(127), (kind == BK_TAXI) ? 1 : random(127), -1);
        SetVehicleParamsEx(B[b][bVeh], 1, 1, 0, 0, 0, 0, 0);
        FCNPC_PutInVehicle(npc, B[b][bVeh], 0);
        B[b][bState] = BS_WAITCAR; B[b][bUntil] = gTick + 4;
    }
    else { B[b][bState] = BS_PAUSE; B[b][bUntil] = gTick + 2 + random(6); }
    return 1;
}

stock Bot_Remove(b)
{
    if (!B[b][bUsed]) return;
    Conv_End(b);
    if (B[b][bVeh] != INVALID_VEHICLE_ID) DestroyVehicle(B[b][bVeh]);
    gBotOf[B[b][bNpc]] = -1;
    FCNPC_Destroy(B[b][bNpc]);
    B[b][bUsed] = false; B[b][bVeh] = INVALID_VEHICLE_ID;
}

stock Bot_WalkPoint(b, pt, &Float:x, &Float:y, &Float:z)
{
    new k = gWalkPath[B[b][bPath]][0] + pt;
    x = gWalkPt[k][0]; y = gWalkPt[k][1]; z = gWalkPt[k][2];
}
stock Bot_DrivePoint(b, pt, &Float:x, &Float:y, &Float:z)
{
    new k = gDrivePath[B[b][bPath]][0] + pt;
    x = gDrivePt[k][0]; y = gDrivePt[k][1]; z = gDrivePt[k][2];
}

// ---------------- движение ----------------
stock Bot_Next(b)
{
    new npc = B[b][bNpc], Float:x, Float:y, Float:z;
    if (B[b][bKind] >= BK_DRIVER)
    {
        B[b][bPt] = (B[b][bPt] + 1) % gDrivePath[B[b][bPath]][1];
        Bot_DrivePoint(b, B[b][bPt], x, y, z);
        new Float:sp = (B[b][bKind] == BK_BUS) ? 0.75 : 0.9 + float(random(40)) / 100.0;
        FCNPC_GoTo(npc, x, y, z, FCNPC_MOVE_TYPE_DRIVE, sp, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
        B[b][bState] = BS_DRIVE;
        return;
    }
    new n = gWalkPath[B[b][bPath]][1];
    new nxt = B[b][bPt] + B[b][bDir];
    if (nxt < 0 || nxt >= n) { B[b][bDir] = -B[b][bDir]; nxt = B[b][bPt] + B[b][bDir]; }
    B[b][bPt] = nxt;
    Bot_WalkPoint(b, nxt, x, y, z);
    new type = FCNPC_MOVE_TYPE_WALK;
    if (B[b][bKind] == BK_JOGGER) type = FCNPC_MOVE_TYPE_RUN;
    else if (B[b][bState] == BS_FLEE) type = FCNPC_MOVE_TYPE_SPRINT;
    else if (random(25) == 0) type = FCNPC_MOVE_TYPE_RUN; // торопится
    FCNPC_GoTo(npc, x, y, z, type, FCNPC_MOVE_SPEED_AUTO, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
    if (B[b][bState] != BS_FLEE) B[b][bState] = BS_WALK;
}

public FCNPC_OnReachDestination(npcid)
{
    new b = gBotOf[npcid];
    if (b == -1 || !B[b][bUsed]) return 1;
    if (B[b][bKind] >= BK_DRIVER)
    {
        // автобус и такси иногда стоят на точке (остановка / посадка пассажира)
        if ((B[b][bKind] == BK_BUS && B[b][bPt] % 9 == 0) || (B[b][bKind] == BK_TAXI && random(30) == 0))
        { B[b][bState] = BS_WAITCAR; B[b][bUntil] = gTick + 8 + random(6); return 1; }
        Bot_Next(b);
        return 1;
    }
    if (B[b][bState] == BS_FLEE)
    {
        if (gTick < B[b][bUntil]) { Bot_Next(b); return 1; }
        B[b][bState] = BS_WALK;
    }
    new r = random(100);
    if (B[b][bKind] == BK_SWEEPER && r < 45)
    {
        FCNPC_ApplyAnimation(npcid, "BD_FIRE", "wash_up", 4.1, 1, 0, 0, 0, 0);
        B[b][bState] = BS_PAUSE; B[b][bUntil] = gTick + 8 + random(8);
        return 1;
    }
    if (r < 7 && Conv_TryStart(b)) return 1;
    if (r < 13)
    {
        B[b][bState] = BS_PAUSE; B[b][bUntil] = gTick + 6 + random(14);
        switch (random(4))
        {
            case 0: FCNPC_ApplyAnimation(npcid, "SMOKING", "M_smklean_loop", 4.1, 1, 0, 0, 0, 0);
            case 1: { FCNPC_ApplyAnimation(npcid, "PED", "phone_talk", 4.1, 1, 0, 0, 0, 0); if (random(2)) Bot_Say(b, gPhoneTalk[random(sizeof gPhoneTalk)]); }
            case 2: FCNPC_ApplyAnimation(npcid, "PED", "IDLE_tired", 4.1, 1, 0, 0, 0, 0);
            default: FCNPC_ApplyAnimation(npcid, "COP_AMBIENT", "Coplook_loop", 4.1, 1, 0, 0, 0, 0);
        }
        return 1;
    }
    if (B[b][bKind] == BK_JOGGER && r < 22)
    {   // прыжок на бегу
        FCNPC_SetKeys(npcid, 0, 0, KEY_JUMP);
        SetTimerEx("Bot_JumpEnd", 450, false, "i", b);
        return 1;
    }
    Bot_Next(b);
    return 1;
}

forward Bot_JumpEnd(b);
public Bot_JumpEnd(b)
{
    if (!B[b][bUsed]) return;
    FCNPC_SetKeys(B[b][bNpc], 0, 0, 0);
    Bot_Next(b);
}

// ---------------- разговоры ----------------
#define MAX_CONV 8
enum E_CONV { bool:cUsed, cA, cB, cTalk, cLine, cNext }
new C[MAX_CONV][E_CONV];

stock bool:Conv_TryStart(b)
{
    if (B[b][bKind] != BK_WALKER && B[b][bKind] != BK_PHONE) return false;
    new Float:x, Float:y, Float:z, Float:ox, Float:oy, Float:oz;
    FCNPC_GetPosition(B[b][bNpc], x, y, z);
    for (new o = 0; o < MAX_BOTS; o++)
    {
        if (o == b || !B[o][bUsed] || B[o][bKind] >= BK_DRIVER || B[o][bConv] != -1 || B[o][bState] == BS_FLEE) continue;
        FCNPC_GetPosition(B[o][bNpc], ox, oy, oz);
        if (Dist2D(x, y, ox, oy) > 9.0) continue;
        for (new c = 0; c < MAX_CONV; c++)
        {
            if (C[c][cUsed]) continue;
            C[c][cUsed] = true; C[c][cA] = b; C[c][cB] = o; C[c][cTalk] = random(sizeof gTalk); C[c][cLine] = 0; C[c][cNext] = gTick + 3;
            B[b][bConv] = c; B[o][bConv] = c; B[b][bState] = BS_TALK; B[o][bState] = BS_TALK;
            // подходим друг к другу и встаём лицом
            FCNPC_Stop(B[o][bNpc]);
            new Float:mx = (x + ox) / 2.0, Float:my = (y + oy) / 2.0, Float:dx = ox - x, Float:dy = oy - y, Float:l = floatsqroot(dx * dx + dy * dy);
            if (l < 0.1) l = 0.1;
            FCNPC_GoTo(B[b][bNpc], mx - dx / l * 0.7, my - dy / l * 0.7, z, FCNPC_MOVE_TYPE_WALK, FCNPC_MOVE_SPEED_AUTO, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
            FCNPC_GoTo(B[o][bNpc], mx + dx / l * 0.7, my + dy / l * 0.7, oz, FCNPC_MOVE_TYPE_WALK, FCNPC_MOVE_SPEED_AUTO, FCNPC_MOVE_MODE_NONE, FCNPC_MOVE_PATHFINDING_NONE, 0.0, true, 0.0, 0);
            return true;
        }
        return false;
    }
    return false;
}

stock Conv_Face(c)
{
    new a = B[C[c][cA]][bNpc], o = B[C[c][cB]][bNpc], Float:x, Float:y, Float:z, Float:ox, Float:oy, Float:oz;
    FCNPC_GetPosition(a, x, y, z); FCNPC_GetPosition(o, ox, oy, oz);
    new Float:ang = -atan2(ox - x, oy - y); if (ang < 0.0) ang += 360.0;
    FCNPC_SetAngle(a, ang);
    ang += 180.0; if (ang >= 360.0) ang -= 360.0;
    FCNPC_SetAngle(o, ang);
}

stock Conv_Tick(c)
{
    if (gTick < C[c][cNext]) return;
    new t = C[c][cTalk], l = C[c][cLine];
    if (l >= 6 || !gTalk[t][l][0]) { Conv_Finish(c); return; }
    if (l == 0) Conv_Face(c);
    new sp = (l % 2 == 0) ? C[c][cA] : C[c][cB];
    Bot_Say(sp, gTalk[t][l]);
    FCNPC_ApplyAnimation(B[sp][bNpc], "PED", "IDLE_chat", 4.1, 0, 1, 1, 1, 1);
    C[c][cLine]++;
    C[c][cNext] = gTick + 6 + strlen(gTalk[t][l]) / 12;
}

stock Conv_Finish(c)
{
    new a = C[c][cA], o = C[c][cB];
    C[c][cUsed] = false;
    if (B[a][bUsed]) { B[a][bConv] = -1; B[a][bState] = BS_PAUSE; B[a][bUntil] = gTick + 1; }
    if (B[o][bUsed]) { B[o][bConv] = -1; B[o][bState] = BS_PAUSE; B[o][bUntil] = gTick + 3; }
}

stock Conv_End(b)
{
    new c = B[b][bConv];
    if (c != -1 && C[c][cUsed]) Conv_Finish(c);
}

// ---------------- реакция на игроков ----------------
forward Bots_OnPlayerText(playerid, const text[]);
public Bots_OnPlayerText(playerid, const text[])
{
    new Float:px, Float:py, Float:pz, Float:x, Float:y, Float:z, best = -1, Float:bd = 7.0;
    GetPlayerPos(playerid, px, py, pz);
    for (new b = 0; b < MAX_BOTS; b++)
    {
        if (!B[b][bUsed] || B[b][bKind] >= BK_DRIVER) continue;
        FCNPC_GetPosition(B[b][bNpc], x, y, z);
        new Float:d = Dist2D(px, py, x, y);
        if (d < bd) { bd = d; best = b; }
    }
    if (best == -1) return 0;
    new reply[128], pname[MAX_PLAYER_NAME], first[MAX_PLAYER_NAME];
    GetPlayerName(playerid, pname, sizeof pname);
    format(first, sizeof first, "%s", pname);
    new us = strfind(first, "_"); if (us > 0) first[us] = 0;
    if (strfind(text, B[best][bFirst], true) != -1 || strfind(text, "ривет", true) != -1 || strfind(text, "дравств", true) != -1 || strfind(text, "дарова", true) != -1 || strfind(text, "hi", true) == 0 || strfind(text, "ку", true) == 0)
        format(reply, sizeof reply, "%s, %s", gGreet[random(sizeof gGreet)], first);
    else if (strfind(text, "как дела", true) != -1 || strfind(text, "как ты", true) != -1)
        reply = (random(2)) ? ("Да нормально, живём потихоньку. А у тебя?") : ("Отлично! Работу вот нашёл.");
    else if (strfind(text, "работ", true) != -1)
        reply = "Работу ищешь? Набери /jobs — там всё есть. Я сам с грузчика начинал.";
    else if (strfind(text, "где", true) != -1)
        reply = "Открой карту или /gps, там всё отмечено. Я сам первое время путался.";
    else if (strfind(text, "бот", true) != -1)
        reply = "Сам ты бот, ха-ха.";
    else if (strfind(text, "пока", true) != -1)
        format(reply, sizeof reply, "Давай, %s, удачи!", first);
    else if (bd < 3.0 && random(3) == 0)
        reply = "Чего? Не расслышал.";
    else return 0;
    if (B[best][bConv] == -1 && B[best][bState] != BS_FLEE)
    {
        FCNPC_Stop(B[best][bNpc]);
        new Float:ang = -atan2(px - x, py - y); if (ang < 0.0) ang += 360.0;
        FCNPC_SetAngle(B[best][bNpc], ang);
        B[best][bState] = BS_PAUSE; B[best][bUntil] = gTick + 10;
    }
    SetTimerEx("Bot_DelayedSay", 1200 + random(1200), false, "is", best, reply);
    return 1;
}

forward Bot_DelayedSay(b, const text[]);
public Bot_DelayedSay(b, const text[]) Bot_Say(b, text);

public FCNPC_OnTakeDamage(npcid, issuerid, Float:amount, weaponid, bodypart)
{
    new b = gBotOf[npcid];
    if (b == -1 || !B[b][bUsed]) return 0;
    if (B[b][bKind] < BK_DRIVER && B[b][bState] != BS_FLEE)
    {
        Conv_End(b);
        B[b][bState] = BS_FLEE; B[b][bUntil] = gTick + 30;
        Bot_Say(b, gHurt[random(sizeof gHurt)]);
        Bot_Next(b);
    }
    if (issuerid != INVALID_PLAYER_ID) CallRemoteFunction("GM_BotAttacked", "ii", npcid, issuerid);
    return 0;
}

public FCNPC_OnDeath(npcid, killerid, reason)
{
    new b = gBotOf[npcid];
    if (b != -1) SetTimerEx("Bot_Revive", 6000, false, "i", b);
    return 1;
}
forward Bot_Revive(b);
public Bot_Revive(b) { if (B[b][bUsed]) { FCNPC_Respawn(B[b][bNpc]); B[b][bState] = BS_PAUSE; B[b][bUntil] = gTick + 3; } }

// ---------------- главный цикл ----------------
forward Bots_Tick();
public Bots_Tick()
{
    gTick++;
    for (new c = 0; c < MAX_CONV; c++) if (C[c][cUsed]) Conv_Tick(c);
    for (new b = 0; b < MAX_BOTS; b++)
    {
        if (!B[b][bUsed])
        {
            if (B[b][bRespawnAt] && gTick >= B[b][bRespawnAt]) { B[b][bRespawnAt] = 0; Bot_Create(b, Bot_KindFor(b)); }
            continue;
        }
        switch (B[b][bState])
        {
            case BS_PAUSE, BS_WAITCAR:
            {
                if (gTick >= B[b][bUntil]) { FCNPC_ClearAnimations(B[b][bNpc]); Bot_Next(b); }
            }
            case BS_WALK:
            {
                // приветствие игрока, проходящего мимо (редко)
                if (gTick % 4 == 0 && gTick - B[b][bLastGreet] > 600 && random(12) == 0) Bot_GreetNear(b);
            }
        }
    }
    // «текучка»: раз в ~4 минуты один пешеход уходит, на его место через минуту заходит другой
    if (gTick % 480 == 0)
    {
        new b = random(BOT_WALKERS);
        if (B[b][bUsed] && B[b][bConv] == -1) { Bot_Remove(b); B[b][bRespawnAt] = gTick + 60 + random(120); }
    }
}

stock Bot_KindFor(b)
{
    if (b >= BOT_WALKERS)
    {
        new d = b - BOT_WALKERS;
        return d == 0 ? BK_BUS : d < 3 ? BK_TAXI : BK_DRIVER;
    }
    if (b < 3) return BK_JOGGER;
    if (b < 5) return BK_SWEEPER;
    if (b < 8) return BK_PHONE;
    return BK_WALKER;
}

stock Bot_GreetNear(b)
{
    new Float:x, Float:y, Float:z;
    FCNPC_GetPosition(B[b][bNpc], x, y, z);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i) || IsPlayerNPC(i) || IsPlayerInAnyVehicle(i)) continue;
        if (!IsPlayerInRangeOfPoint(i, 3.5, x, y, z)) continue;
        B[b][bLastGreet] = gTick;
        Bot_Say(b, gGreet[random(sizeof gGreet)]);
        return;
    }
}

forward Bots_Spawn(step);
public Bots_Spawn(step)
{
    // создаём ботов постепенно, как будто игроки заходят на сервер
    if (step >= MAX_BOTS) return;
    Bot_Create(step, Bot_KindFor(step));
    SetTimerEx("Bots_Spawn", 1500 + random(2500), false, "i", step + 1);
}

public OnFilterScriptInit()
{
    for (new b = 0; b < MAX_BOTS; b++) { B[b][bUsed] = false; B[b][bConv] = -1; B[b][bVeh] = INVALID_VEHICLE_ID; }
    FCNPC_SetUpdateRate(80);
    gTimer = SetTimer("Bots_Tick", TICK_MS, true);
    SetTimerEx("Bots_Spawn", 8000, false, "i", 0);
    printf("[BOTS] Godjo bots: старт, ботов будет до %d", MAX_BOTS);
    return 1;
}

public OnFilterScriptExit()
{
    KillTimer(gTimer);
    for (new b = 0; b < MAX_BOTS; b++) Bot_Remove(b);
    return 1;
}

public OnVehicleDeath(vehicleid, killerid)
{
    for (new b = 0; b < MAX_BOTS; b++)
    {
        if (!B[b][bUsed] || B[b][bVeh] != vehicleid) continue;
        new kind = B[b][bKind];
        Bot_Remove(b);
        B[b][bRespawnAt] = gTick + 60;
        #pragma unused kind
    }
    return 1;
}

forward Bots_Count();
public Bots_Count()
{
    new n = 0;
    for (new b = 0; b < MAX_BOTS; b++) if (B[b][bUsed]) n++;
    return n;
}
