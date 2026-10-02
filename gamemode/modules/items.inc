// =====================================================================
//  Предметы и инвентарь (CEF: сетка слотов с весом, drag&drop)
// =====================================================================
enum { IT_MISC, IT_FOOD, IT_DRINK, IT_TOOL, IT_WEAPON, IT_MED, IT_SELL, IT_QUEST }
enum E_ITEMDEF { iName[32], iKey[14], iWeight, iStack, iType, iValue, iModel, iSell }
new const gItems[][E_ITEMDEF] =
{
    {"—",                   "none",     0,    0,  IT_MISC,   0,   0,    0},
    {"Пустая бутылка",      "bottle",   300,  20, IT_SELL,   0,   1486, 3},
    {"Шаурма",              "shawarma", 400,  5,  IT_FOOD,   40,  2769, 0},
    {"Бургер",              "burger",   350,  5,  IT_FOOD,   35,  2768, 0},
    {"Бутылка воды",        "water",    500,  5,  IT_DRINK,  10,  1484, 0},
    {"Чипсы",               "chips",    150,  10, IT_FOOD,   15,  2821, 0},
    {"Телефон",             "phone",    200,  1,  IT_TOOL,   0,   18868,0},
    {"Энергетик",           "energy",   330,  5,  IT_DRINK,  30,  1543, 0},
    {"Удочка",              "rod",      900,  1,  IT_TOOL,   0,   18632,0},
    {"Наживка",             "bait",     20,   50, IT_MISC,   0,   2045, 0},
    {"Карась",              "fish_s",   500,  20, IT_SELL,   0,   19630,15},
    {"Окунь",               "fish_m",   800,  20, IT_SELL,   0,   19630,35},
    {"Щука",                "fish_b",   1500, 10, IT_SELL,   0,   19630,90},
    {"Металлоискатель",     "detector", 1800, 1,  IT_TOOL,   0,   2045, 0},
    {"Лопата",              "shovel",   1500, 1,  IT_TOOL,   0,   337,  0},
    {"Старинная монета",    "coin",     20,   50, IT_SELL,   0,   1212, 120},
    {"Золотое кольцо",      "ring",     10,   20, IT_SELL,   0,   1212, 350},
    {"Ржавый хлам",         "junk",     700,  20, IT_SELL,   0,   2040, 6},
    {"Аптечка",             "medkit",   600,  5,  IT_MED,    50,  11738,0},
    {"Ремкомплект",         "repair",   2000, 3,  IT_TOOL,   0,   19921,0},
    {"Канистра (10 л)",     "canister", 3000, 2,  IT_TOOL,   10,  1650, 0},
    {"Руда",                "ore",      2000, 10, IT_SELL,   0,   905,  0},
    {"Пистолет 9мм",        "colt",     1100, 1,  IT_WEAPON, 22,  346,  0},
    {"Desert Eagle",        "deagle",   1800, 1,  IT_WEAPON, 24,  348,  0},
    {"Дробовик",            "shotgun",  3500, 1,  IT_WEAPON, 25,  349,  0},
    {"M4",                  "m4",       3500, 1,  IT_WEAPON, 31,  356,  0},
    {"AK-47",               "ak47",     4000, 1,  IT_WEAPON, 30,  355,  0},
    {"Дубинка",             "baton",    800,  1,  IT_WEAPON, 3,   334,  0},
    {"Патроны",             "ammo",     15,   500,IT_MISC,   0,   2061, 0},
    {"Бронежилет",          "armour",   2500, 2,  IT_MED,    100, 1242, 0},
    {"Письмо Дяди Вовы",    "letter",   10,   1,  IT_QUEST,  0,   2894, 0},
    {"Сигареты",            "cigs",     30,   20, IT_MISC,   0,   19896,0},
    {"Отмычка",             "lockpick", 50,   10, IT_TOOL,   0,   18634,0}
};

stock Item_ByKey(const key[])
{
    for (new i = 1; i < sizeof gItems; i++) if (StrEq(gItems[i][iKey], key)) return i;
    return 0;
}

new gInv[MAX_PLAYERS][MAX_INV_SLOTS][2];

stock Inv_Clear(playerid)
{
    for (new s = 0; s < MAX_INV_SLOTS; s++) { gInv[playerid][s][0] = 0; gInv[playerid][s][1] = 0; }
}

stock Inv_Load(playerid)
{
    Inv_Clear(playerid);
    new q[96];
    format(q, sizeof q, "SELECT slot,item,amount FROM items WHERE acc=%d", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    if (!r) return;
    for (new i = 0, n = db_num_rows(r); i < n; i++)
    {
        new s = DB_GetInt(r, "slot"), it = DB_GetInt(r, "item");
        if (s >= 0 && s < MAX_INV_SLOTS && it > 0 && it < sizeof gItems)
        {
            gInv[playerid][s][0] = it;
            gInv[playerid][s][1] = DB_GetInt(r, "amount");
        }
        db_next_row(r);
    }
    db_free_result(r);
}

stock Inv_SaveSlot(playerid, s)
{
    new q[160];
    if (gInv[playerid][s][0] == 0 || gInv[playerid][s][1] <= 0)
    {
        gInv[playerid][s][0] = 0; gInv[playerid][s][1] = 0;
        format(q, sizeof q, "DELETE FROM items WHERE acc=%d AND slot=%d", P[playerid][pID], s);
    }
    else format(q, sizeof q, "INSERT OR REPLACE INTO items VALUES (%d,%d,%d,%d)", P[playerid][pID], s, gInv[playerid][s][0], gInv[playerid][s][1]);
    DB_Exec(q);
}

stock Inv_Weight(playerid)
{
    new w = 0;
    for (new s = 0; s < MAX_INV_SLOTS; s++) if (gInv[playerid][s][0]) w += gItems[gInv[playerid][s][0]][iWeight] * gInv[playerid][s][1];
    return w;
}

stock Inv_Count(playerid, item)
{
    new c = 0;
    for (new s = 0; s < MAX_INV_SLOTS; s++) if (gInv[playerid][s][0] == item) c += gInv[playerid][s][1];
    return c;
}

// Добавляет предмет. Возвращает сколько реально добавилось.
stock Inv_Add(playerid, item, amount, bool:notify = true)
{
    if (item <= 0 || item >= sizeof gItems || amount <= 0) return 0;
    new free = (MAX_INV_WEIGHT - Inv_Weight(playerid));
    new w = gItems[item][iWeight];
    if (w > 0 && free / w < amount) amount = free / w;
    if (amount <= 0) { if (notify) Err(playerid, "Инвентарь переполнен — слишком тяжело."); return 0; }
    new left = amount, st = gItems[item][iStack];
    for (new s = 0; s < MAX_INV_SLOTS && left > 0; s++)
    {
        if (gInv[playerid][s][0] == item && gInv[playerid][s][1] < st)
        {
            new add = st - gInv[playerid][s][1];
            if (add > left) add = left;
            gInv[playerid][s][1] += add; left -= add;
            Inv_SaveSlot(playerid, s);
        }
    }
    for (new s = 0; s < MAX_INV_SLOTS && left > 0; s++)
    {
        if (gInv[playerid][s][0] == 0)
        {
            new add = (left > st) ? st : left;
            gInv[playerid][s][0] = item; gInv[playerid][s][1] = add; left -= add;
            Inv_SaveSlot(playerid, s);
        }
    }
    new added = amount - left;
    if (added < amount && notify) Err(playerid, "Не хватило места в инвентаре.");
    if (added > 0)
    {
        Inv_Refresh(playerid);
        Quest_CheckItems(playerid);
    }
    return added;
}

stock Inv_Remove(playerid, item, amount)
{
    if (Inv_Count(playerid, item) < amount) return 0;
    new left = amount;
    for (new s = MAX_INV_SLOTS - 1; s >= 0 && left > 0; s--)
    {
        if (gInv[playerid][s][0] != item) continue;
        new take = (gInv[playerid][s][1] > left) ? left : gInv[playerid][s][1];
        gInv[playerid][s][1] -= take; left -= take;
        Inv_SaveSlot(playerid, s);
    }
    Inv_Refresh(playerid);
    return 1;
}

stock Inv_Json(playerid, out[], size)
{
    format(out, size, "{\"w\":%d,\"wm\":%d,\"slots\":[", Inv_Weight(playerid), MAX_INV_WEIGHT);
    new t[48], first = 1;
    for (new s = 0; s < MAX_INV_SLOTS; s++)
    {
        if (!gInv[playerid][s][0]) continue;
        format(t, sizeof t, "%s[%d,%d,%d]", first ? "" : ",", s, gInv[playerid][s][0], gInv[playerid][s][1]);
        strcat(out, t, size);
        first = 0;
    }
    strcat(out, "]}", size);
}

stock Inv_Refresh(playerid)
{
    new j[1400];
    Inv_Json(playerid, j, sizeof j);
    UI_Data(playerid, "inv", j);
}

stock Inv_Open(playerid)
{
    new j[1400];
    Inv_Json(playerid, j, sizeof j);
    UI_Open(playerid, "inventory", j);
    return 1;
}

// Каталог предметов отправляется в UI один раз после входа
stock Items_SendCatalog(playerid)
{
    new j[4096], t[128];
    strcpy(j, "[");
    for (new i = 1; i < sizeof gItems; i++)
    {
        format(t, sizeof t, "%s{\"id\":%d,\"n\":\"%s\",\"k\":\"%s\",\"w\":%d,\"s\":%d,\"t\":%d}", i > 1 ? "," : "",
            i, gItems[i][iName], gItems[i][iKey], gItems[i][iWeight], gItems[i][iStack], gItems[i][iType]);
        strcat(j, t);
    }
    strcat(j, "]");
    UI_Data(playerid, "items", j);
}

// ---------- предметы на земле ----------
#define MAX_DROPS 200
enum E_DROP { bool:dUsed, dItem, dAmount, dObj, dPoint, dTime }
new gDrop[MAX_DROPS][E_DROP];

stock Drop_Create(item, amount, Float:x, Float:y, Float:z, vw, interior)
{
    #pragma unused interior
    for (new i = 0; i < MAX_DROPS; i++)
    {
        if (gDrop[i][dUsed]) continue;
        new lbl[80];
        format(lbl, sizeof lbl, "%s (%d)\n" H_GREY "[ALT] Поднять", gItems[item][iName], amount);
        gDrop[i][dUsed] = true; gDrop[i][dItem] = item; gDrop[i][dAmount] = amount;
        gDrop[i][dObj] = CreateObject(gItems[item][iModel], x, y, z - 0.9, 0.0, 0.0, float(random(360)));
        gDrop[i][dPoint] = Point_Add(PT_ITEM, i, x, y, z - 0.5, vw, lbl);
        gDrop[i][dTime] = gettime();
        return i;
    }
    return -1;
}

stock Drop_Destroy(i)
{
    if (!gDrop[i][dUsed]) return;
    DestroyObject(gDrop[i][dObj]);
    Point_Remove(gDrop[i][dPoint]);
    gDrop[i][dUsed] = false;
}

stock Drop_Pick(playerid, i)
{
    if (!gDrop[i][dUsed]) return;
    new got = Inv_Add(playerid, gDrop[i][dItem], gDrop[i][dAmount]);
    if (got <= 0) return;
    ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.0, 0, 0, 0, 0, 0);
    if (got >= gDrop[i][dAmount]) Drop_Destroy(i);
    else gDrop[i][dAmount] -= got;
    new s[96];
    format(s, sizeof s, "Вы подняли: %s x%d", gItems[gDrop[i][dItem]][iName], got);
    Ok(playerid, s);
}

stock Drops_Cleanup()
{
    new now = gettime();
    for (new i = 0; i < MAX_DROPS; i++) if (gDrop[i][dUsed] && now - gDrop[i][dTime] > 900) Drop_Destroy(i);
}

// ---------- действия из UI ----------
UI:inv(playerid, const a[])
{
    new act[12];
    CefField(a, 1, act);
    if (StrEq(act, "open")) return Inv_Open(playerid);
    new s1 = CefFieldInt(a, 2), s2 = CefFieldInt(a, 3);
    if (s1 < 0 || s1 >= MAX_INV_SLOTS) return 1;
    if (StrEq(act, "move"))
    {
        if (s2 < 0 || s2 >= MAX_INV_SLOTS || s1 == s2) return 1;
        new it1 = gInv[playerid][s1][0], am1 = gInv[playerid][s1][1];
        new it2 = gInv[playerid][s2][0], am2 = gInv[playerid][s2][1];
        if (it1 && it1 == it2 && am2 < gItems[it1][iStack])
        {
            new add = gItems[it1][iStack] - am2;
            if (add > am1) add = am1;
            gInv[playerid][s2][1] += add; gInv[playerid][s1][1] -= add;
        }
        else
        {
            gInv[playerid][s1][0] = it2; gInv[playerid][s1][1] = am2;
            gInv[playerid][s2][0] = it1; gInv[playerid][s2][1] = am1;
        }
        Inv_SaveSlot(playerid, s1); Inv_SaveSlot(playerid, s2);
        Inv_Refresh(playerid);
        return 1;
    }
    new item = gInv[playerid][s1][0];
    if (!item) return 1;
    if (StrEq(act, "use")) return Item_Use(playerid, s1);
    new amount = s2;
    if (amount <= 0 || amount > gInv[playerid][s1][1]) amount = gInv[playerid][s1][1];
    if (StrEq(act, "drop"))
    {
        if (gItems[item][iType] == IT_QUEST) return Err(playerid, "Этот предмет нельзя выбросить.");
        if (IsPlayerInAnyVehicle(playerid)) return Err(playerid, "Выйдите из транспорта.");
        new Float:x, Float:y, Float:z;
        GetPlayerPos(playerid, x, y, z);
        if (Drop_Create(item, amount, x, y, z, GetPlayerVirtualWorld(playerid), GetPlayerInterior(playerid)) == -1) return Err(playerid, "Здесь уже слишком много мусора.");
        gInv[playerid][s1][1] -= amount;
        Inv_SaveSlot(playerid, s1);
        Inv_Refresh(playerid);
        ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.0, 0, 0, 0, 0, 0);
        SendMe(playerid, "выбросил что-то на землю");
    }
    else if (StrEq(act, "give"))
    {
        new t = GetNearestPlayer(playerid, 3.0);
        if (t == INVALID_PLAYER_ID || !P[t][pLogged]) return Err(playerid, "Рядом никого нет.");
        if (gItems[item][iType] == IT_QUEST) return Err(playerid, "Этот предмет нельзя передать.");
        new got = Inv_Add(t, item, amount, false);
        if (got <= 0) return Err(playerid, "У игрока нет места в инвентаре.");
        Inv_Remove(playerid, item, got);
        new s[128];
        format(s, sizeof s, "Вы передали %s: %s x%d", PName(t), gItems[item][iName], got); Ok(playerid, s);
        format(s, sizeof s, "%s передал вам: %s x%d", PName(playerid), gItems[item][iName], got); Ok(t, s);
        format(s, sizeof s, "передал что-то %s", PName(t)); SendMe(playerid, s);
    }
    return 1;
}

stock Item_Use(playerid, s)
{
    new item = gInv[playerid][s][0];
    new type = gItems[item][iType];
    if (P[playerid][pCuffed]) return Err(playerid, "Вы в наручниках.");
    switch (type)
    {
        case IT_FOOD, IT_DRINK:
        {
            if (type == IT_FOOD)
            {
                P[playerid][pHunger] += float(gItems[item][iValue]);
                if (P[playerid][pHunger] > 100.0) P[playerid][pHunger] = 100.0;
                ApplyAnimation(playerid, "FOOD", "EAT_Burger", 4.0, 0, 0, 0, 0, 0);
                SendMe(playerid, "ест");
                Quest_Event(playerid, "eat", gItems[item][iKey]);
            }
            else
            {
                if (item == Item_ByKey("energy")) P[playerid][pFatigue] += 30.0;
                else { P[playerid][pHunger] += 5.0; P[playerid][pFatigue] += 10.0; }
                if (P[playerid][pFatigue] > 100.0) P[playerid][pFatigue] = 100.0;
                if (P[playerid][pHunger] > 100.0) P[playerid][pHunger] = 100.0;
                ApplyAnimation(playerid, "VENDING", "VEND_Drink2_P", 4.0, 0, 0, 0, 0, 0);
                SendMe(playerid, "пьёт");
            }
            gInv[playerid][s][1]--;
            Inv_SaveSlot(playerid, s);
            Inv_Refresh(playerid);
            UI_Hud(playerid);
        }
        case IT_MED:
        {
            if (item == Item_ByKey("armour"))
            {
                SetPlayerArmour(playerid, 100.0);
                SendMe(playerid, "надел бронежилет");
            }
            else
            {
                new Float:hp; GetPlayerHealth(playerid, hp);
                hp += float(gItems[item][iValue]); if (hp > 100.0) hp = 100.0;
                SetPlayerHealth(playerid, hp);
                SendMe(playerid, "использовал аптечку");
            }
            gInv[playerid][s][1]--; Inv_SaveSlot(playerid, s); Inv_Refresh(playerid);
        }
        case IT_WEAPON:
        {
            new ammoItem = Item_ByKey("ammo");
            new ammo = Inv_Count(playerid, ammoItem);
            new wep = gItems[item][iValue];
            if (wep == 3) ammo = 1;
            else if (ammo <= 0) return Err(playerid, "Нет патронов.");
            else Inv_Remove(playerid, ammoItem, ammo);
            GivePlayerWeapon(playerid, wep, ammo);
            gInv[playerid][s][1]--; Inv_SaveSlot(playerid, s); Inv_Refresh(playerid);
            SendMe(playerid, "достал оружие");
        }
        case IT_TOOL:
        {
            new key[14]; strcpy(key, gItems[item][iKey]);
            if (StrEq(key, "phone")) return Phone_Open(playerid);
            if (StrEq(key, "rod")) return Info(playerid, "Встаньте у края пирса и введите /fish.");
            if (StrEq(key, "detector")) return Treasure_Toggle(playerid);
            if (StrEq(key, "shovel")) return Treasure_Dig(playerid);
            if (StrEq(key, "repair")) return Veh_UseRepair(playerid, s);
            if (StrEq(key, "canister")) return Veh_UseCanister(playerid, s);
            if (StrEq(key, "lockpick")) return Info(playerid, "Отмычка пригодится полиции при обыске... или не только.");
        }
        case IT_QUEST:
        {
            if (item == Item_ByKey("letter"))
                UI_Prompt(playerid, "letter", "Письмо Дяди Вовы", "«Если читаешь это — значит, ты выбрался. Помни: дно — это не место, это состояние. Я горжусь тобой. В.»");
        }
        default: Info(playerid, "Этот предмет нельзя использовать.");
    }
    return 1;
}

PROMPT:letter(playerid, response, const text[]) { UI_Close(playerid); return 1; }

// ---------- хранилища (шкаф дома, склад организации, сейф семьи) ----------
enum { ST_HOUSE = 1, ST_ORG = 2, ST_FAMILY = 3 }
#define STORAGE_SLOTS 40
new gStorOpenType[MAX_PLAYERS], gStorOpenId[MAX_PLAYERS];

stock Storage_Open(playerid, otype, oid, const title[])
{
    gStorOpenType[playerid] = otype; gStorOpenId[playerid] = oid;
    Storage_Send(playerid, title, true);
}

stock Storage_Send(playerid, const title[] = "", bool:open = false)
{
    new j[3000], t[48], q[128], inv[1400];
    Inv_Json(playerid, inv, sizeof inv);
    format(j, sizeof j, "{\"title\":\"%s\",\"inv\":%s,\"slots\":[", JsonEsc(title), inv);
    format(q, sizeof q, "SELECT slot,item,amount FROM storage WHERE otype=%d AND oid=%d", gStorOpenType[playerid], gStorOpenId[playerid]);
    new DBResult:r = DB_Query(q);
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n; i++)
        {
            format(t, sizeof t, "%s[%d,%d,%d]", i ? "," : "", DB_GetInt(r, "slot"), DB_GetInt(r, "item"), DB_GetInt(r, "amount"));
            strcat(j, t);
            db_next_row(r);
        }
        db_free_result(r);
    }
    format(t, sizeof t, "],\"max\":%d}", STORAGE_SLOTS);
    strcat(j, t);
    if (open) UI_Open(playerid, "storage", j);
    else UI_Data(playerid, "storage", j);
}

UI:stor(playerid, const a[])
{
    new act[8], q[192];
    CefField(a, 1, act);
    new slot = CefFieldInt(a, 2), amount = CefFieldInt(a, 3);
    new ot = gStorOpenType[playerid], oid = gStorOpenId[playerid];
    if (!ot) return 1;
    if (StrEq(act, "put"))
    {
        if (slot < 0 || slot >= MAX_INV_SLOTS || !gInv[playerid][slot][0]) return 1;
        new item = gInv[playerid][slot][0];
        if (gItems[item][iType] == IT_QUEST) return Err(playerid, "Квестовый предмет нельзя убрать.");
        if (amount <= 0 || amount > gInv[playerid][slot][1]) amount = gInv[playerid][slot][1];
        // найти слот с тем же предметом или свободный
        format(q, sizeof q, "SELECT slot FROM storage WHERE otype=%d AND oid=%d AND item=%d", ot, oid, item);
        new ss = DB_ScalarInt(q, -1);
        if (ss == -1)
        {
            for (new i = 0; i < STORAGE_SLOTS; i++)
            {
                format(q, sizeof q, "SELECT COUNT(*) FROM storage WHERE otype=%d AND oid=%d AND slot=%d", ot, oid, i);
                if (DB_ScalarInt(q) == 0) { ss = i; break; }
            }
            if (ss == -1) return Err(playerid, "Хранилище заполнено.");
            format(q, sizeof q, "INSERT INTO storage VALUES (%d,%d,%d,%d,%d)", ot, oid, ss, item, amount);
        }
        else format(q, sizeof q, "UPDATE storage SET amount=amount+%d WHERE otype=%d AND oid=%d AND slot=%d", amount, ot, oid, ss);
        DB_Exec(q);
        gInv[playerid][slot][1] -= amount;
        Inv_SaveSlot(playerid, slot);
        Storage_Send(playerid);
    }
    else if (StrEq(act, "take"))
    {
        if (ot == ST_ORG && P[playerid][pOrgRank] < 2 && P[playerid][pOrg] > 0) return Err(playerid, "Брать со склада можно со 2 ранга.");
        format(q, sizeof q, "SELECT item,amount FROM storage WHERE otype=%d AND oid=%d AND slot=%d", ot, oid, slot);
        new DBResult:r = DB_Query(q), item = 0, have = 0;
        if (r) { if (db_num_rows(r)) { item = DB_GetInt(r, "item"); have = DB_GetInt(r, "amount"); } db_free_result(r); }
        if (!item) return 1;
        if (amount <= 0 || amount > have) amount = have;
        new got = Inv_Add(playerid, item, amount);
        if (got <= 0) return 1;
        if (got >= have) format(q, sizeof q, "DELETE FROM storage WHERE otype=%d AND oid=%d AND slot=%d", ot, oid, slot);
        else format(q, sizeof q, "UPDATE storage SET amount=amount-%d WHERE otype=%d AND oid=%d AND slot=%d", got, ot, oid, slot);
        DB_Exec(q);
        new l[96]; format(l, sizeof l, "storage %d:%d take %d x%d", ot, oid, item, got);
        Log("storage", PName(playerid), l);
        Storage_Send(playerid);
    }
    else if (StrEq(act, "close")) { gStorOpenType[playerid] = 0; }
    return 1;
}
