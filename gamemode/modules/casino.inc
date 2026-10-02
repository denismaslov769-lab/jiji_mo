// =====================================================================
//  Казино «Четыре Дракона»: рулетка и слоты (серверный RNG)
// =====================================================================
#define CASINO_MIN_BET   100
#define CASINO_MAX_BET   50000
#define CASINO_MIN_LVL   3

new const gRouletteRed[] = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36};

stock bool:Roulette_IsRed(n)
{
    for (new i = 0; i < sizeof gRouletteRed; i++) if (gRouletteRed[i] == n) return true;
    return false;
}

stock Casino_Open(playerid)
{
    if (P[playerid][pLevel] < CASINO_MIN_LVL) return Err(playerid, "Охрана: «Вход в казино с 3 уровня».");
    if (!HasPassport(playerid)) return Err(playerid, "Охрана: «Без паспорта не пускаем».");
    new j[160];
    format(j, sizeof j, "{\"cash\":%d,\"min\":%d,\"max\":%d}", P[playerid][pCash], CASINO_MIN_BET, CASINO_MAX_BET);
    UI_Open(playerid, "casino", j);
    return 1;
}

stock bool:Casino_TakeBet(playerid, amount)
{
    if (amount < CASINO_MIN_BET || amount > CASINO_MAX_BET)
    {
        new s[96]; format(s, sizeof s, "Ставка от %s до %s.", FormatMoney(CASINO_MIN_BET), FormatMoney(CASINO_MAX_BET));
        Err(playerid, s); return false;
    }
    if (!IsPlayerAtLoc(playerid, "casino_table", 6.0)) { Err(playerid, "Подойдите к столу."); return false; }
    if (gettime() - P[playerid][pCasinoBet] < 3) { Err(playerid, "Подождите окончания раунда."); return false; }
    if (!Money_Take(playerid, amount, "casino_bet")) { Err(playerid, "Недостаточно наличных."); return false; }
    P[playerid][pCasinoBet] = gettime();
    gTreasury += amount / 50; // налог на игорный бизнес
    return true;
}

UI:casino(playerid, const a[])
{
    new act[16]; CefField(a, 1, act);
    if (StrEq(act, "roulette"))
    {
        // casino|roulette|type|value|amount
        new type[10]; CefField(a, 2, type);
        new val = CefFieldInt(a, 3), amount = CefFieldInt(a, 4);
        if (!Casino_TakeBet(playerid, amount)) return 1;
        new n = random(37), mult = 0;
        if (StrEq(type, "red"))        mult = (n && Roulette_IsRed(n)) ? 2 : 0;
        else if (StrEq(type, "black")) mult = (n && !Roulette_IsRed(n)) ? 2 : 0;
        else if (StrEq(type, "even"))  mult = (n && n % 2 == 0) ? 2 : 0;
        else if (StrEq(type, "odd"))   mult = (n % 2 == 1) ? 2 : 0;
        else if (StrEq(type, "low"))   mult = (n >= 1 && n <= 18) ? 2 : 0;
        else if (StrEq(type, "high"))  mult = (n >= 19) ? 2 : 0;
        else if (StrEq(type, "dozen")) mult = (val >= 1 && val <= 3 && n && (n - 1) / 12 + 1 == val) ? 3 : 0;
        else if (StrEq(type, "num"))   mult = (val >= 0 && val <= 36 && n == val) ? 36 : 0;
        new win = amount * mult;
        if (win > 0) Money_Give(playerid, win, "casino_win");
        new j[160];
        format(j, sizeof j, "{\"game\":\"roulette\",\"n\":%d,\"red\":%d,\"win\":%d,\"bet\":%d,\"cash\":%d}", n, Roulette_IsRed(n) ? 1 : 0, win, amount, P[playerid][pCash]);
        UI_Data(playerid, "casino_result", j);
        if (mult == 36) { Ach_Give(playerid, "jackpot", "Фортуна улыбнулась"); new s[128]; format(s, sizeof s, "сорвал куш на рулетке (%s)", FormatMoney(win)); SendMe(playerid, s); }
        Log("casino", PName(playerid), j);
        return 1;
    }
    if (StrEq(act, "slots"))
    {
        new amount = CefFieldInt(a, 2);
        if (!Casino_TakeBet(playerid, amount)) return 1;
        // символы: 0 вишня,1 лимон,2 слива,3 колокол,4 BAR,5 семёрка
        new r[3];
        for (new i = 0; i < 3; i++)
        {
            new x = random(100);
            r[i] = x < 30 ? 0 : x < 52 ? 1 : x < 70 ? 2 : x < 84 ? 3 : x < 95 ? 4 : 5;
        }
        new mult = 0;
        if (r[0] == r[1] && r[1] == r[2])
        {
            static const tri[] = {5, 8, 10, 15, 25, 100};
            mult = tri[r[0]];
        }
        else if (r[0] == 0 && r[1] == 0) mult = 2;
        else if (r[0] == r[1] || r[1] == r[2]) mult = 1;
        new win = amount * mult;
        if (win > 0) Money_Give(playerid, win, "casino_win");
        new j[160];
        format(j, sizeof j, "{\"game\":\"slots\",\"reels\":[%d,%d,%d],\"win\":%d,\"bet\":%d,\"cash\":%d}", r[0], r[1], r[2], win, amount, P[playerid][pCash]);
        UI_Data(playerid, "casino_result", j);
        if (mult == 100) Ach_Give(playerid, "jackpot", "Фортуна улыбнулась");
        return 1;
    }
    if (StrEq(act, "close")) UI_Close(playerid, "casino");
    return 1;
}
