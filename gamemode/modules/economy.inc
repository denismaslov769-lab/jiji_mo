// =====================================================================
//  Экономика: наличные, банк, банкоматы, переводы, налоги, PayDay
// =====================================================================
new gLastPaydayHour = -1;
new gTreasury = 0;

stock Money_Give(playerid, amount, const reason[] = "")
{
    P[playerid][pCash] += amount;
    GivePlayerMoney(playerid, amount);
    if (amount >= 10000 && reason[0])
    {
        new l[96]; format(l, sizeof l, "+%d %s", amount, reason);
        Log("money", PName(playerid), l);
    }
    UI_Hud(playerid);
}

stock bool:Money_Take(playerid, amount, const reason[] = "")
{
    if (P[playerid][pCash] < amount) return false;
    P[playerid][pCash] -= amount;
    GivePlayerMoney(playerid, -amount);
    if (amount >= 10000 && reason[0])
    {
        new l[96]; format(l, sizeof l, "-%d %s", amount, reason);
        Log("money", PName(playerid), l);
    }
    UI_Hud(playerid);
    return true;
}

// Оплата: сначала наличными, иначе картой (если bankOk)
stock bool:Pay(playerid, amount, const reason[] = "", bool:bankOk = true)
{
    if (Money_Take(playerid, amount, reason)) return true;
    if (bankOk && P[playerid][pBank] >= amount)
    {
        P[playerid][pBank] -= amount;
        UI_Hud(playerid);
        Info(playerid, "Оплачено банковской картой.");
        return true;
    }
    new s[96]; format(s, sizeof s, "Недостаточно денег. Нужно %s.", FormatMoney(amount));
    Err(playerid, s);
    return false;
}

stock Bank_Open(playerid, bool:atm)
{
    new j[256];
    format(j, sizeof j, "{\"cash\":%d,\"bank\":%d,\"atm\":%s,\"name\":\"%s\",\"card\":\"4276 %04d %04d\"}",
        P[playerid][pCash], P[playerid][pBank], atm ? ("true") : ("false"), PName(playerid), 1000 + P[playerid][pID] % 9000, (P[playerid][pID] * 7919) % 10000);
    UI_Open(playerid, "bank", j);
}

stock Bank_Refresh(playerid)
{
    new j[96];
    format(j, sizeof j, "{\"cash\":%d,\"bank\":%d}", P[playerid][pCash], P[playerid][pBank]);
    UI_Data(playerid, "bank", j);
}

UI:bank(playerid, const a[])
{
    new act[12];
    CefField(a, 1, act);
    new amount = CefFieldInt(a, 2);
    if (amount <= 0 || amount > 100000000) return Err(playerid, "Неверная сумма.");
    if (StrEq(act, "dep"))
    {
        if (!Money_Take(playerid, amount)) return Err(playerid, "Недостаточно наличных.");
        P[playerid][pBank] += amount;
        Ok(playerid, "Деньги зачислены на счёт.");
    }
    else if (StrEq(act, "wd"))
    {
        if (P[playerid][pBank] < amount) return Err(playerid, "Недостаточно средств на счёте.");
        P[playerid][pBank] -= amount;
        Money_Give(playerid, amount);
        Ok(playerid, "Получите наличные.");
    }
    else if (StrEq(act, "tr"))
    {
        new to[MAX_PLAYER_NAME];
        CefField(a, 3, to);
        if (P[playerid][pLevel] < 2) return Err(playerid, "Переводы доступны со 2 уровня.");
        if (P[playerid][pBank] < amount) return Err(playerid, "Недостаточно средств на счёте.");
        new q[160];
        format(q, sizeof q, "SELECT id FROM accounts WHERE name='%s'", SqlEsc(to));
        new acc = DB_ScalarInt(q, 0);
        if (!acc || acc == P[playerid][pID]) return Err(playerid, "Получатель не найден.");
        new fee = amount / 100;
        P[playerid][pBank] -= amount;
        gTreasury += fee;
        new t = FindPlayerByAcc(acc);
        if (t != INVALID_PLAYER_ID)
        {
            P[t][pBank] += amount - fee;
            new s[128]; format(s, sizeof s, "Перевод от %s: %s", PName(playerid), FormatMoney(amount - fee)); Ok(t, s);
            UI_Hud(t);
        }
        else
        {
            format(q, sizeof q, "UPDATE accounts SET bank=bank+%d WHERE id=%d", amount - fee, acc);
            DB_Exec(q);
        }
        new l[96]; format(l, sizeof l, "transfer %d to %s", amount, to);
        Log("bank", PName(playerid), l);
        new s[128]; format(s, sizeof s, "Переведено %s (комиссия 1%%: %s).", FormatMoney(amount - fee), FormatMoney(fee));
        Ok(playerid, s);
    }
    Bank_Refresh(playerid);
    Quest_CheckItems(playerid);
    return 1;
}

// Раз в час (по реальным часам сервера)
stock Payday_Check()
{
    new h, m, s;
    gettime(h, m, s);
    if (m != 0 || h == gLastPaydayHour) return;
    gLastPaydayHour = h;
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsLogged(i)) continue;
        Payday_Player(i);
    }
    Houses_TaxTick();
    Biz_PaydayTick();
}

stock Payday_Player(playerid)
{
    SCM(playerid, C_WHITE, "_______________ " H_GOLD "PayDay" H_WHITE " _______________");
    if (P[playerid][pMinuteTick] < PAYDAY_MIN_PLAYED)
    {
        new s[128]; format(s, sizeof s, "Вы отыграли меньше %d минут за час — PayDay не начислен.", PAYDAY_MIN_PLAYED);
        SCM(playerid, C_GREY, s);
        P[playerid][pMinuteTick] = 0;
        return;
    }
    P[playerid][pMinuteTick] = 0;
    new salary = Org_Salary(playerid);
    new interest = P[playerid][pBank] / 1000; // 0.1% в час
    if (interest > 2000) interest = 2000;
    new tax = (salary * 13) / 100;
    new rent = 0;
    if (P[playerid][pRentHouse]) rent = House_RentPrice(P[playerid][pRentHouse]);
    P[playerid][pBank] += salary - tax + interest;
    gTreasury += tax;
    new s[144];
    format(s, sizeof s, "Зарплата: %s | Налог (13%%): -%s | Проценты по вкладу: +%s", FormatMoney(salary), FormatMoney(tax), FormatMoney(interest));
    SCM(playerid, C_WHITE, s);
    if (rent > 0)
    {
        if (P[playerid][pBank] >= rent)
        {
            P[playerid][pBank] -= rent;
            House_PayRent(P[playerid][pRentHouse], rent);
            format(s, sizeof s, "Аренда жилья: -%s", FormatMoney(rent)); SCM(playerid, C_WHITE, s);
        }
        else
        {
            SCM(playerid, C_RED, "Не хватило денег на аренду — договор расторгнут.");
            P[playerid][pRentHouse] = 0;
        }
    }
    if (P[playerid][pDebt] > 0)
    {   // рассрочка (автошкола и т.п.): до $100 за PayDay
        new part = P[playerid][pDebt] > 100 ? 100 : P[playerid][pDebt];
        if (part > P[playerid][pBank]) part = P[playerid][pBank];
        if (part > 0)
        {
            P[playerid][pBank] -= part; P[playerid][pDebt] -= part; gTreasury += part;
            P_SetInt(playerid, "debt", P[playerid][pDebt]);
            format(s, sizeof s, "Погашение рассрочки: -%s (осталось %s)", FormatMoney(part), FormatMoney(P[playerid][pDebt])); SCM(playerid, C_WHITE, s);
        }
    }
    format(s, sizeof s, "Баланс счёта: %s", FormatMoney(P[playerid][pBank])); SCM(playerid, C_GREEN, s);
    Lvl_AddExp(playerid, 1);
    Fam_Payday(playerid);
    UI_Hud(playerid);
    P_Save(playerid);
}

// Оплата услуги; если денег совсем нет — в рассрочку (списывается с PayDay)
stock bool:Debt_PayOrLoan(playerid, amount, const reason[])
{
    if (P[playerid][pCash] >= amount || P[playerid][pBank] >= amount) return Pay(playerid, amount, reason);
    if (P[playerid][pDebt] + amount > 1500) { Err(playerid, "Недостаточно денег, а лимит рассрочки ($1.500) исчерпан."); return false; }
    P[playerid][pDebt] += amount;
    P_SetInt(playerid, "debt", P[playerid][pDebt]);
    new s[144]; format(s, sizeof s, "Денег не хватает — оформлена рассрочка на %s. Сумма будет списываться с зарплаты (до $100 за PayDay).", FormatMoney(amount));
    Info(playerid, s);
    return true;
}
