// =====================================================================
//  Магазины: ларёк с едой, 24/7, одежда и аксессуары
// =====================================================================
#define BIZ_KIOSK 4
#define BIZ_247   1
#define BIZ_CLOTH 5

stock Kiosk_Open(playerid)
{
    Menu_Begin("Шаурма у Ашота", "«Свежая, почти вчерашняя!»");
    Menu_Add(1, "Шаурма", "+40 к сытости", 25, "food");
    Menu_Add(2, "Чипсы", "+15 к сытости", 10, "food");
    Menu_Add(3, "Бутылка воды", "+10 к бодрости", 8, "drink");
    Menu_Send(playerid, "kiosk");
    return 1;
}

MENU:kiosk(playerid, item, const extra[])
{
    new it, price;
    switch (item) { case 1: { it = Item_ByKey("shawarma"); price = 25; } case 2: { it = Item_ByKey("chips"); price = 10; } case 3: { it = Item_ByKey("water"); price = 8; } default: return 1; }
    if (!Biz_HasStock(BIZ_KIOSK)) return Err(playerid, "Ашот: «Всё раскупили, приходи позже!»");
    if (P[playerid][pCash] < price) return Err(playerid, "Ашот: «Без денег не кормлю, дорогой». Нужны наличные.");
    if (Inv_Add(playerid, it, 1) <= 0) return 1;
    Money_Take(playerid, price);
    Biz_Sale(BIZ_KIOSK, price);
    ApplyAnimation(playerid, "INT_SHOP", "shop_pay", 4.0, 0, 0, 0, 0, 0);
    Ok(playerid, "Покупка в инвентаре (I).");
    Quest_Event(playerid, "buy_food", gItems[it][iKey]);
    return 1;
}

static const g247Items[][] = {"phone","water","chips","burger","energy","rod","bait","medkit","repair","canister","shovel","cigs"};
static const g247Price[] = {400, 10, 12, 30, 25, 300, 20, 150, 250, 120, 150, 15};
static const g247Amount[] = {1, 1, 1, 1, 1, 1, 10, 1, 1, 1, 1, 1};

stock Shop247_Open(playerid)
{
    Menu_Begin("Магазин 24/7", "Всё необходимое");
    new t[48];
    for (new i = 0; i < sizeof g247Price; i++)
    {
        new it = Item_ByKey(g247Items[i]);
        if (g247Amount[i] > 1) format(t, sizeof t, "%s x%d", gItems[it][iName], g247Amount[i]); else strcpy(t, gItems[it][iName]);
        Menu_Add(i, t, "", g247Price[i], g247Items[i]);
    }
    Menu_Send(playerid, "shop247");
    return 1;
}

MENU:shop247(playerid, item, const extra[])
{
    if (item < 0 || item >= sizeof g247Price) return 1;
    new it = Item_ByKey(g247Items[item]);
    if (StrEq(g247Items[item], "phone") && Inv_Count(playerid, it) > 0) return Err(playerid, "У вас уже есть телефон.");
    if (!Biz_HasStock(BIZ_247)) return Err(playerid, "Продавец: «Товар закончился, ждём поставку».");
    if (!Pay(playerid, g247Price[item], "247")) return 1;
    if (Inv_Add(playerid, it, g247Amount[item]) <= 0) { Money_Give(playerid, g247Price[item]); return 1; }
    Biz_Sale(BIZ_247, g247Price[item]);
    new s[96]; format(s, sizeof s, "Куплено: %s", gItems[it][iName]);
    Ok(playerid, s);
    if (StrEq(g247Items[item], "phone")) Info(playerid, "Телефон открывается клавишей P или командой /phone.");
    return 1;
}

// ---------- одежда ----------
static const gClothM[] = {2, 7, 14, 15, 17, 19, 20, 21, 22, 23, 24, 25, 26, 29, 46, 47, 48, 60, 72, 101, 170, 180, 184, 185, 186, 188, 206, 240, 250, 258, 259, 299};
static const gClothF[] = {9, 11, 12, 13, 40, 41, 55, 56, 69, 76, 91, 93, 141, 148, 150, 169, 190, 191, 192, 193, 211, 214, 215, 216, 219, 224, 225, 226, 233};
static const gAccModel[] = {18955, 18891, 19006, 18970, 18968, 19022, 18947, 19033, 18640, 19554};
static const gAccName[][] = {"Кепка", "Бандана", "Очки", "Шляпа", "Панама", "Очки-авиаторы", "Котелок", "Солнцезащитные очки", "Причёска «Афро»", "Шапка-бини"};
static const gAccPrice[] = {150, 120, 200, 350, 180, 400, 450, 250, 500, 160};

stock Cloth_Price(idx) return 300 + idx * 120;

stock Binco_Open(playerid)
{
    Menu_Begin("Одежда «Ганни»", "Выберите раздел");
    Menu_Add(1, "Одежда", "Смена образа (скина)", -1, "shirt");
    Menu_Add(2, "Аксессуары", "Шляпы, очки, банданы", -1, "hat");
    Menu_Add(3, "Снять аксессуары", "", -1, "x");
    Menu_Send(playerid, "binco");
    return 1;
}

MENU:binco(playerid, item, const extra[])
{
    new t[32];
    if (item == 1)
    {
        Menu_Begin("Одежда", P[playerid][pSex] ? ("Женская коллекция") : ("Мужская коллекция"));
        if (P[playerid][pSex]) for (new i = 0; i < sizeof gClothF; i++) { format(t, sizeof t, "Образ #%d", gClothF[i]); Menu_Add(gClothF[i], t, "", Cloth_Price(i), "skin"); }
        else for (new i = 0; i < sizeof gClothM; i++) { format(t, sizeof t, "Образ #%d", gClothM[i]); Menu_Add(gClothM[i], t, "", Cloth_Price(i), "skin"); }
        Menu_Send(playerid, "cloth");
    }
    else if (item == 2)
    {
        Menu_Begin("Аксессуары", "Надеваются сразу");
        for (new i = 0; i < sizeof gAccModel; i++) Menu_Add(i, gAccName[i], "", gAccPrice[i], "hat");
        Menu_Send(playerid, "acc");
    }
    else if (item == 3)
    {
        P[playerid][pAccSlot0] = 0; P[playerid][pAccSlot1] = 0;
        Acc_Apply(playerid);
        UI_Close(playerid);
    }
    return 1;
}

MENU:cloth(playerid, item, const extra[])
{
    new idx = -1;
    if (P[playerid][pSex]) { for (new i = 0; i < sizeof gClothF; i++) if (gClothF[i] == item) idx = i; }
    else { for (new i = 0; i < sizeof gClothM; i++) if (gClothM[i] == item) idx = i; }
    if (idx == -1) return 1;
    if (!Pay(playerid, Cloth_Price(idx), "clothes")) return 1;
    Biz_Sale(BIZ_CLOTH, Cloth_Price(idx));
    P[playerid][pSkin] = item;
    if (!P[playerid][pDuty]) SetPlayerSkin(playerid, item);
    P_SetInt(playerid, "skin", item);
    Ok(playerid, "Отличный выбор! Новый образ ваш.");
    return 1;
}

MENU:acc(playerid, item, const extra[])
{
    if (item < 0 || item >= sizeof gAccModel) return 1;
    if (!Pay(playerid, gAccPrice[item], "accessory")) return 1;
    // очки во второй слот, головные уборы — в первый
    new model = gAccModel[item];
    if (model == 19006 || model == 19022 || model == 19033) P[playerid][pAccSlot1] = model; else P[playerid][pAccSlot0] = model;
    Acc_Apply(playerid);
    Ok(playerid, "Аксессуар надет. Сохранится после перезахода.");
    return 1;
}

stock Acc_Apply(playerid)
{
    RemovePlayerAttachedObject(playerid, 0);
    RemovePlayerAttachedObject(playerid, 1);
    if (P[playerid][pAccSlot0]) SetPlayerAttachedObject(playerid, 0, P[playerid][pAccSlot0], 2, 0.12, 0.0, 0.0, 0.0, 90.0, 90.0);
    if (P[playerid][pAccSlot1]) SetPlayerAttachedObject(playerid, 1, P[playerid][pAccSlot1], 2, 0.09, 0.035, 0.0, 90.0, 90.0, 0.0);
}
