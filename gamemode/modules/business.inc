// =====================================================================
//  Бизнесы: 24/7, АЗС, ларёк, магазин одежды. Закупка товара, доход владельца
// =====================================================================
#define MAX_BIZ 32
enum E_BIZ { bID, bType, bName[48], bOwner, bPrice, Float:bX, Float:bY, Float:bZ, bStock, bBalance, bMarkup, bPoint }
new gB[MAX_BIZ][E_BIZ], gBCount;

stock Biz_Load()
{
    gBCount = 0;
    new DBResult:r = DB_Query("SELECT * FROM businesses ORDER BY id");
    if (!r) return;
    for (new i = 0, n = db_num_rows(r); i < n && gBCount < MAX_BIZ; i++)
    {
        new k = gBCount++;
        gB[k][bID] = DB_GetInt(r, "id"); gB[k][bType] = DB_GetInt(r, "type");
        DB_GetStr(r, "name", gB[k][bName], 48);
        gB[k][bOwner] = DB_GetInt(r, "owner"); gB[k][bPrice] = DB_GetInt(r, "price");
        gB[k][bX] = DB_GetFloat(r, "x") + 1.8; gB[k][bY] = DB_GetFloat(r, "y"); gB[k][bZ] = DB_GetFloat(r, "z");
        gB[k][bStock] = DB_GetInt(r, "stock"); gB[k][bBalance] = DB_GetInt(r, "balance"); gB[k][bMarkup] = DB_GetInt(r, "markup");
        gB[k][bPoint] = Point_Add(PT_BIZ, k, gB[k][bX], gB[k][bY], gB[k][bZ], 0, "", 1272);
        Biz_UpdateLabel(k);
        db_next_row(r);
    }
    db_free_result(r);
}

stock Biz_Idx(id) { for (new k = 0; k < gBCount; k++) if (gB[k][bID] == id) return k; return -1; }

stock Biz_UpdateLabel(k)
{
    new s[200];
    if (gB[k][bOwner]) format(s, sizeof s, H_GOLD "Бизнес: %s\n" H_WHITE "Владелец: %s\nТовар: %d ед.\n" H_GREY "[ALT] Управление", gB[k][bName], AccName(gB[k][bOwner]), gB[k][bStock]);
    else format(s, sizeof s, H_GOLD "Бизнес: %s\n" H_GREEN "Продаётся: %s\n" H_GREY "[ALT] Подробнее", gB[k][bName], FormatMoney(gB[k][bPrice]));
    Point_SetLabel(gB[k][bPoint], s);
}

stock Biz_Save(k)
{
    new q[200];
    format(q, sizeof q, "UPDATE businesses SET owner=%d,stock=%d,balance=%d,markup=%d WHERE id=%d", gB[k][bOwner], gB[k][bStock], gB[k][bBalance], gB[k][bMarkup], gB[k][bID]);
    DB_Exec(q);
}

stock bool:Biz_HasStock(id) { new k = Biz_Idx(id); return k == -1 || !gB[k][bOwner] || gB[k][bStock] > 0; }

stock Biz_FuelPrice(id)
{
    new k = Biz_Idx(id);
    if (k == -1 || !gB[k][bOwner]) return FUEL_PRICE;
    return (FUEL_PRICE * gB[k][bMarkup]) / 100;
}

stock Biz_Sale(id, amount)
{
    new k = Biz_Idx(id);
    if (k == -1) return;
    if (!gB[k][bOwner]) { gTreasury += amount / 10; return; }
    gB[k][bStock]--;
    if (gB[k][bStock] < 0) gB[k][bStock] = 0;
    gB[k][bBalance] += amount;
    Biz_Save(k);
    if (gB[k][bStock] % 25 == 0) Biz_UpdateLabel(k);
}

stock Biz_Door(playerid, k)
{
    new d[96]; format(d, sizeof d, "Касса: %s | Товар: %d", FormatMoney(gB[k][bBalance]), gB[k][bStock]);
    Menu_Begin(gB[k][bName], gB[k][bOwner] == P[playerid][pID] ? d : ("Информация о бизнесе"));
    if (!gB[k][bOwner]) Menu_Add(1, "Купить бизнес", "Доступно с 5 уровня", gB[k][bPrice], "key");
    else if (gB[k][bOwner] == P[playerid][pID])
    {
        Menu_Add(2, "Снять выручку", "", gB[k][bBalance], "cash");
        Menu_Add(3, "Заказать товар (+100 ед.)", "Поставка оплачивается из кассы", 200, "box");
        if (gB[k][bType] == 2) Menu_Add(4, "Изменить цену топлива", "Наценка 50-300%", -1, "fuel");
        Menu_Add(5, "Продать государству", "60% от цены", gB[k][bPrice] * 6 / 10, "x");
    }
    else Menu_Add(0, "Владелец", AccName(gB[k][bOwner]));
    SetPVarInt(playerid, "bizK", k);
    Menu_Send(playerid, "biz");
    return 1;
}

MENU:biz(playerid, item, const extra[])
{
    new k = GetPVarInt(playerid, "bizK");
    if (k < 0 || k >= gBCount) return 1;
    UI_Close(playerid);
    switch (item)
    {
        case 1:
        {
            if (gB[k][bOwner]) return 1;
            if (P[playerid][pLevel] < 5) return Err(playerid, "Покупка бизнеса доступна с 5 уровня.");
            new q[96]; format(q, sizeof q, "SELECT COUNT(*) FROM businesses WHERE owner=%d", P[playerid][pID]);
            if (DB_ScalarInt(q) > 0) return Err(playerid, "У вас уже есть бизнес.");
            if (!Pay(playerid, gB[k][bPrice], "biz buy")) return 1;
            gB[k][bOwner] = P[playerid][pID]; gB[k][bBalance] = 0;
            Biz_Save(k); Biz_UpdateLabel(k);
            Ok(playerid, "Поздравляем, вы владелец бизнеса!");
            Quest_Event(playerid, "biz_buy", "*");
        }
        case 2:
        {
            if (gB[k][bOwner] != P[playerid][pID] || gB[k][bBalance] <= 0) return 1;
            Money_Give(playerid, gB[k][bBalance], "biz withdraw");
            gB[k][bBalance] = 0; Biz_Save(k);
        }
        case 3:
        {
            if (gB[k][bOwner] != P[playerid][pID]) return 1;
            if (gB[k][bBalance] < 200) return Err(playerid, "В кассе недостаточно денег для закупки.");
            gB[k][bBalance] -= 200; gB[k][bStock] += 100; Biz_Save(k); Biz_UpdateLabel(k);
            Ok(playerid, "Товар заказан и доставлен.");
        }
        case 4: UI_Prompt(playerid, "bizmarkup", "Цена топлива", "Введите наценку в процентах (50-300). 100 — базовая цена.", true, "100");
        case 5:
        {
            if (gB[k][bOwner] != P[playerid][pID]) return 1;
            Money_Give(playerid, gB[k][bPrice] * 6 / 10 + gB[k][bBalance], "biz sell");
            gB[k][bOwner] = 0; gB[k][bBalance] = 0; Biz_Save(k); Biz_UpdateLabel(k);
            Ok(playerid, "Бизнес продан государству.");
        }
    }
    return 1;
}

PROMPT:bizmarkup(playerid, response, const text[])
{
    UI_Close(playerid);
    if (!response) return 1;
    new k = GetPVarInt(playerid, "bizK");
    if (k < 0 || k >= gBCount || gB[k][bOwner] != P[playerid][pID]) return 1;
    new v = strval(text);
    if (v < 50 || v > 300) return Err(playerid, "Допустимо 50-300%.");
    gB[k][bMarkup] = v; Biz_Save(k);
    return Ok(playerid, "Цена изменена.");
}

stock Biz_PaydayTick()
{
    for (new k = 0; k < gBCount; k++)
    {
        if (!gB[k][bOwner]) continue;
        new tax = gB[k][bBalance] / 100;
        gB[k][bBalance] -= tax; gTreasury += tax;
        Biz_Save(k);
    }
}
