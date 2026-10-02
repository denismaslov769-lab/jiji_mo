// =====================================================================
//  Обновление 2.0: анимации, мини-игры, погода, подсказки, транспорт
// =====================================================================
#define TIP_INTERVAL_MIN   12      // подсказки в чат раз в N минут
#define WEATHER_CHANGE_MIN 30      // смена погоды раз в N минут

new bool:gBelt[MAX_PLAYERS];
new bool:gAnimLoop[MAX_PLAYERS];
new gWeather = 1, gWeatherTick, gTipTick, gTipIdx, gWeatherLock;

static const gGoodWeather[] = {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 17, 18};
static const gTips[][] = {
    "Анимации: /anims — сесть, лечь, станцевать. Остановить: /stopanim",
    "Новые автомобили в автосалоне Godjo Motors: от Lada Priora до Bugatti Chiron!",
    "Пристегнитесь: /belt снижает урон в ДТП.",
    "Спорите? Решите честно: /dice или /coin.",
    "Передать деньги игроку рядом: /pay [id] [сумма].",
    "Капот, багажник, фары: /hood /trunk /lights.",
    "Площадь Godjo у станции Unity — место встреч и мероприятий.",
    "Нашли баг или нарушителя? /report [текст].",
    "Правила сервера: /rules. Список команд: /help.",
    "Лаунчер Godjo сам обновляет сборку — запускайте игру через него."
};

stock Extras_Init()
{
    new h, m, s; gettime(h, m, s);
    SetWorldTime(h);
    gWeather = gGoodWeather[random(sizeof gGoodWeather)];
    SetWeather(gWeather);
    Map_Build();
}

stock Extras_Reset(playerid) { gBelt[playerid] = false; gAnimLoop[playerid] = false; }

stock Extras_Minute()
{
    new h, m, s; gettime(h, m, s);
    SetWorldTime(h);                                    // время суток в игре = время сервера
    if (gWeatherLock > 0) gWeatherLock--;
    if (++gWeatherTick >= WEATHER_CHANGE_MIN && !gWeatherLock)
    {
        gWeatherTick = 0;
        gWeather = gGoodWeather[random(sizeof gGoodWeather)];
        SetWeather(gWeather);
    }
    if (++gTipTick >= TIP_INTERVAL_MIN)
    {
        gTipTick = 0;
        new t[160]; format(t, sizeof t, H_GOLD "[Подсказка] " H_WHITE "%s", gTips[gTipIdx]);
        for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i)) SendClientMessage(i, C_WHITE, t);
        gTipIdx = (gTipIdx + 1) % sizeof gTips;
    }
}

stock Extras_OnSpawn(playerid)
{
    SetPlayerWeather(playerid, gWeather);
    PreloadAnims(playerid);
}

stock PreloadAnims(playerid)
{
    static const libs[][] = {"PED", "BEACH", "DANCING", "ON_LOOKERS", "GANGS", "MISC", "SMOKING", "CRACK", "INT_HOUSE", "COP_AMBIENT"};
    for (new i = 0; i < sizeof libs; i++) ApplyAnimation(playerid, libs[i], "null", 0.0, 0, 0, 0, 0, 0);
}

// ремень: возвращаем часть урона от ДТП (weaponid 49 — наезд/удар машиной, 54 — падение/столкновение)
stock Extras_OnTakeDamage(playerid, Float:amount, weaponid)
{
    if (gBelt[playerid] && IsPlayerInAnyVehicle(playerid) && (weaponid == 49 || weaponid == 54))
    {
        new Float:hp; GetPlayerHealth(playerid, hp);
        SetPlayerHealth(playerid, hp + amount * 0.6);
    }
}

stock Extras_OnExitVehicle(playerid)
{
    if (gBelt[playerid]) { gBelt[playerid] = false; SendMe(playerid, "отстегнул ремень безопасности"); }
}

// ---------------------------------------------------------------- анимации
enum E_ANIM { anLib[16], anName[24], anTitle[32], bool:anLoop }
static const gAnims[][E_ANIM] = {
    {"PED", "SEAT_idle", "Сесть на стул", true},
    {"BEACH", "ParkSit_M_loop", "Сесть на землю", true},
    {"BEACH", "Lay_Bac_Loop", "Лечь на спину", true},
    {"BEACH", "SitnWait_loop_W", "Лечь на бок", true},
    {"DANCING", "dnce_M_a", "Танец 1", true},
    {"DANCING", "dnce_M_b", "Танец 2", true},
    {"DANCING", "dnce_M_c", "Танец 3", true},
    {"DANCING", "DAN_Loop_A", "Танец 4", true},
    {"ON_LOOKERS", "wave_loop", "Помахать", true},
    {"PED", "handsup", "Руки вверх", false},
    {"COP_AMBIENT", "Coplook_loop", "Скрестить руки", true},
    {"GANGS", "leanIDLE", "Облокотиться", true},
    {"SMOKING", "M_smklean_loop", "Курить у стены", true},
    {"MISC", "plyr_shkhead", "Покачать головой", false},
    {"GANGS", "prtial_gngtlkA", "Жестикулировать", true},
    {"CRACK", "crckidle2", "Плохо себя чувствовать", true},
    {"INT_HOUSE", "LOU_Loop", "Отдыхать на диване", true},
    {"ON_LOOKERS", "shout_01", "Кричать", false}
};

stock bool:Anim_CanPlay(playerid)
{
    if (IsPlayerInAnyVehicle(playerid)) { Err(playerid, "Нельзя в транспорте."); return false; }
    if (P[playerid][pCuffed] || P[playerid][pInjured]) { Err(playerid, "Сейчас нельзя."); return false; }
    return true;
}

stock Anim_Play(playerid, idx)
{
    if (idx < 0 || idx >= sizeof gAnims || !Anim_CanPlay(playerid)) return 1;
    ApplyAnimation(playerid, gAnims[idx][anLib], gAnims[idx][anName], 4.1, gAnims[idx][anLoop] ? 1 : 0, 0, 0, gAnims[idx][anLoop] ? 1 : 0, 0, 1);
    gAnimLoop[playerid] = gAnims[idx][anLoop];
    if (gAnimLoop[playerid]) Info(playerid, "Остановить анимацию: /stopanim");
    return 1;
}

CMD:anims(playerid, const params[])
{
    Menu_Begin("Анимации", "Остановить: /stopanim");
    for (new i = 0; i < sizeof gAnims; i++) Menu_Add(i, gAnims[i][anTitle], "", -1, "chat");
    Menu_Send(playerid, "anims");
    return 1;
}
MENU:anims(playerid, item, const extra[]) { UI_Close(playerid); return Anim_Play(playerid, item); }

CMD:sit(playerid, const params[]) return Anim_Play(playerid, 1);
CMD:lay(playerid, const params[]) return Anim_Play(playerid, 2);
CMD:wave(playerid, const params[]) return Anim_Play(playerid, 8);
CMD:handsup(playerid, const params[]) return Anim_Play(playerid, 9);
CMD:dance(playerid, const params[])
{
    new n = GetParamInt(params, 0, 1);
    if (n < 1 || n > 4) return Err(playerid, "/dance [1-4]");
    return Anim_Play(playerid, 3 + n);
}
CMD:stopanim(playerid, const params[])
{
    if (IsPlayerInAnyVehicle(playerid)) return 1;
    ClearAnimations(playerid, 1);
    gAnimLoop[playerid] = false;
    return 1;
}

// ---------------------------------------------------------------- мини-игры и полезное
CMD:dice(playerid, const params[])
{
    new s[96]; format(s, sizeof s, "бросил кубик — выпало %d", random(6) + 1);
    SendMe(playerid, s); return 1;
}
CMD:coin(playerid, const params[])
{
    SendMe(playerid, random(2) ? ("подбросил монетку — выпал орёл") : ("подбросил монетку — выпала решка"));
    return 1;
}
CMD:id(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/id [часть ника]");
    new name[MAX_PLAYER_NAME], found = 0, s[96];
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i) || IsPlayerNPC(i)) continue;
        GetPlayerName(i, name, sizeof name);
        if (strfind(name, params, true) == -1) continue;
        format(s, sizeof s, "[%d] %s — уровень %d, пинг %d", i, name, P[i][pLevel], GetPlayerPing(i));
        SCM(playerid, C_WHITE, s);
        if (++found >= 8) break;
    }
    if (!found) Err(playerid, "Никого не найдено.");
    return 1;
}
CMD:online(playerid, const params[])
{
    new n = 0, a = 0;
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i)) { n++; if (P[i][pAdmin]) a++; }
    new s[96]; format(s, sizeof s, "Сейчас в игре: %d из %d (администраторов: %d).", n, GetMaxPlayers(), a);
    Info(playerid, s); return 1;
}
CMD:pay(playerid, const params[])
{
    new t = GetParamPlayer(params, 0), sum = GetParamInt(params, 1, 0);
    if (t == INVALID_PLAYER_ID || sum <= 0) return Err(playerid, "/pay [id] [сумма]");
    if (t == playerid || !IsLogged(t)) return Err(playerid, "Неверный игрок.");
    if (sum > 50000) return Err(playerid, "За раз можно передать не больше $50 000.");
    if (P[playerid][pLevel] < 2) return Err(playerid, "Передавать деньги можно со 2 уровня.");
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z);
    if (!IsPlayerInRangeOfPoint(playerid, 3.0, x, y, z)) return Err(playerid, "Игрок слишком далеко.");
    if (!Money_Take(playerid, sum, "pay")) return Err(playerid, "Недостаточно наличных.");
    Money_Give(t, sum, "pay");
    new s[96]; format(s, sizeof s, "передал деньги %s", PName(t)); SendMe(playerid, s);
    format(s, sizeof s, "Вы получили %s от %s.", FormatMoney(sum), PName(playerid)); Ok(t, s);
    return 1;
}
CMD:rules(playerid, const params[])
{
    Menu_Begin("Правила Godjo Role Play", "Незнание правил не освобождает от ответственности");
    Menu_Add(1, "DM запрещён", "Нельзя убивать без RP-причины", -1, "info");
    Menu_Add(2, "DB запрещён", "Нельзя давить игроков транспортом", -1, "info");
    Menu_Add(3, "MG запрещён", "OOC-информация не используется в IC", -1, "info");
    Menu_Add(4, "PG запрещён", "Отыгрывайте страх и боль реалистично", -1, "info");
    Menu_Add(5, "Читы и баги", "Бан без предупреждения", -1, "info");
    Menu_Add(6, "Уважение", "Оскорбления родных и администрации — мут/бан", -1, "info");
    Menu_Send(playerid, "noop");
    return 1;
}

// ---------------------------------------------------------------- транспорт
stock Veh_Toggle(playerid, what)
{
    new v = GetPlayerVehicleID(playerid);
    if (!v)
    {
        new Float:x, Float:y, Float:z, Float:best = 4.0;
        for (new i = 1; i < MAX_VEHICLES; i++)
        {
            if (!GetVehicleModel(i)) continue;
            GetVehiclePos(i, x, y, z);
            new Float:d = GetPlayerDistanceFromPoint(playerid, x, y, z);
            if (d < best) { best = d; v = i; }
        }
    }
    if (!v) return Err(playerid, "Рядом нет транспорта.");
    if (gV[v][vOwner] && gV[v][vOwner] != P[playerid][pID] && GetPlayerVehicleID(playerid) != v) return Err(playerid, "Это не ваш транспорт.");
    new e, l, al, d, bn, bt, o;
    GetVehicleParamsEx(v, e, l, al, d, bn, bt, o);
    switch (what)
    {
        case 0: { bn = (bn == 1) ? 0 : 1; SendMe(playerid, bn ? ("открыл капот") : ("закрыл капот")); }
        case 1: { bt = (bt == 1) ? 0 : 1; SendMe(playerid, bt ? ("открыл багажник") : ("закрыл багажник")); }
        case 2:
        {
            if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Err(playerid, "Нужно быть за рулём.");
            l = (l == 1) ? 0 : 1;
        }
    }
    SetVehicleParamsEx(v, e, l, al, d, bn, bt, o);
    return 1;
}
CMD:hood(playerid, const params[]) return Veh_Toggle(playerid, 0);
CMD:trunk(playerid, const params[]) return Veh_Toggle(playerid, 1);
CMD:lights(playerid, const params[]) return Veh_Toggle(playerid, 2);
CMD:belt(playerid, const params[])
{
    if (!IsPlayerInAnyVehicle(playerid)) return Err(playerid, "Вы не в транспорте.");
    gBelt[playerid] = !gBelt[playerid];
    SendMe(playerid, gBelt[playerid] ? ("пристегнул ремень безопасности") : ("отстегнул ремень безопасности"));
    return 1;
}
CMD:eject(playerid, const params[])
{
    if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Err(playerid, "Нужно быть за рулём.");
    new t = GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || t == playerid || GetPlayerVehicleID(t) != GetPlayerVehicleID(playerid)) return Err(playerid, "/eject [id пассажира]");
    RemovePlayerFromVehicle(t);
    new s[80]; format(s, sizeof s, "высадил из машины %s", PName(t)); SendMe(playerid, s);
    return 1;
}

// ---------------------------------------------------------------- админ: погода и время
CMD:setweather(playerid, const params[])
{
    if (!AdmCheck(playerid, 2)) return 1;
    new w = GetParamInt(params, 0, -1);
    if (w < 0 || w > 45) return Err(playerid, "/setweather [0-45] — погода фиксируется на 60 минут");
    gWeather = w; gWeatherLock = 60; gWeatherTick = 0;
    SetWeather(w);
    new s[80]; format(s, sizeof s, "Администратор %s сменил погоду.", PName(playerid));
    SendClientMessageToAll(C_ADMIN, s);
    return 1;
}
CMD:settime(playerid, const params[])
{
    if (!AdmCheck(playerid, 3)) return 1;
    new h = GetParamInt(params, 0, -1);
    if (h < 0 || h > 23) return Err(playerid, "/settime [0-23] (до следующей минуты сервера)");
    SetWorldTime(h);
    return 1;
}
