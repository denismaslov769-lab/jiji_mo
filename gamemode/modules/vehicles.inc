// =====================================================================
//  Личный транспорт: автосалон, ключи, топливо, тюнинг, штрафстоянка
// =====================================================================
#define MAX_OWNED_CARS 3
enum E_VEH { vDB, vOwner, Float:vFuel, bool:vLocked, vModel, vC1, vC2, vNitro, vWheels, vPrice }
new gV[MAX_VEHICLES][E_VEH];

static const gDealerModel[] = {466, 401, 410, 418, 404, 405, 445, 507, 547, 550, 579, 400, 560, 562, 411, 462, 521};
static const gDealerName[][] = {"Glendale", "Bravura", "Manana", "Moonbeam", "Perenniel", "Sentinel", "Admiral", "Elegant", "Primo", "Sunrise", "Huntley", "Landstalker", "Sultan", "Elegy", "Infernus", "Faggio", "FCR-900"};
static const gDealerPrice[] = {9000, 12000, 7500, 14000, 10000, 28000, 26000, 30000, 18000, 22000, 60000, 35000, 120000, 110000, 450000, 3500, 40000};

stock Veh_Reset(v)
{
    gV[v][vDB] = 0; gV[v][vOwner] = 0; gV[v][vFuel] = 40.0; gV[v][vLocked] = false;
    gV[v][vModel] = 0; gV[v][vNitro] = 0; gV[v][vWheels] = 0;
}

stock Veh_GetFuel(v) return (v > 0 && v < MAX_VEHICLES) ? floatround(gV[v][vFuel]) : 0;
stock Veh_SetFuel(v, fuel) { if (v > 0 && v < MAX_VEHICLES) gV[v][vFuel] = float(fuel); }

stock Veh_Spawn(DBResult:r)
{
    new model = DB_GetInt(r, "model");
    new v = CreateVehicle(model, DB_GetFloat(r, "x"), DB_GetFloat(r, "y"), DB_GetFloat(r, "z"), DB_GetFloat(r, "a"), DB_GetInt(r, "c1"), DB_GetInt(r, "c2"), -1);
    if (v == INVALID_VEHICLE_ID) return v;
    Veh_Reset(v);
    gV[v][vDB] = DB_GetInt(r, "id"); gV[v][vOwner] = DB_GetInt(r, "owner"); gV[v][vModel] = model;
    gV[v][vFuel] = DB_GetFloat(r, "fuel"); gV[v][vLocked] = DB_GetInt(r, "locked") != 0;
    gV[v][vC1] = DB_GetInt(r, "c1"); gV[v][vC2] = DB_GetInt(r, "c2");
    gV[v][vNitro] = DB_GetInt(r, "nitro"); gV[v][vWheels] = DB_GetInt(r, "wheels"); gV[v][vPrice] = DB_GetInt(r, "price");
    Veh_ApplyTune(v);
    Veh_ApplyLock(v);
    new plate[16]; format(plate, sizeof plate, "GJ %04d", gV[v][vDB]);
    SetVehicleNumberPlate(v, plate);
    return v;
}

stock Veh_ApplyTune(v)
{
    if (gV[v][vNitro]) AddVehicleComponent(v, 1010);
    if (gV[v][vWheels]) AddVehicleComponent(v, gV[v][vWheels]);
}

stock Veh_ApplyLock(v)
{
    new e, l, al, d, bn, bt, o;
    GetVehicleParamsEx(v, e, l, al, d, bn, bt, o);
    SetVehicleParamsEx(v, e, l, al, gV[v][vLocked] ? 1 : 0, bn, bt, o);
}

stock Veh_LoadForPlayer(playerid)
{
    new q[128];
    format(q, sizeof q, "SELECT * FROM vehicles WHERE owner=%d AND impound=0", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    if (!r) return;
    for (new i = 0, n = db_num_rows(r); i < n; i++) { Veh_Spawn(r); db_next_row(r); }
    db_free_result(r);
}

stock Veh_SaveOne(v)
{
    if (!gV[v][vDB]) return;
    new Float:x, Float:y, Float:z, Float:a, q[300];
    GetVehiclePos(v, x, y, z); GetVehicleZAngle(v, a);
    format(q, sizeof q, "UPDATE vehicles SET x=%.2f,y=%.2f,z=%.2f,a=%.1f,fuel=%.1f,locked=%d,c1=%d,c2=%d,nitro=%d,wheels=%d WHERE id=%d",
        x, y, z, a, gV[v][vFuel], gV[v][vLocked] ? 1 : 0, gV[v][vC1], gV[v][vC2], gV[v][vNitro], gV[v][vWheels], gV[v][vDB]);
    DB_Exec(q);
}

stock Veh_UnloadForPlayer(playerid)
{
    for (new v = 1; v < MAX_VEHICLES; v++)
    {
        if (gV[v][vDB] && gV[v][vOwner] == P[playerid][pID])
        {
            Veh_SaveOne(v);
            DestroyVehicle(v);
            Veh_Reset(v);
        }
    }
}

stock Veh_CountOwned(playerid)
{
    new q[96]; format(q, sizeof q, "SELECT COUNT(*) FROM vehicles WHERE owner=%d", P[playerid][pID]);
    return DB_ScalarInt(q);
}

stock Veh_CreateOwned(playerid, model, c1, c2, Float:x, Float:y, Float:z, Float:a, price)
{
    new q[300];
    format(q, sizeof q, "INSERT INTO vehicles (owner,model,c1,c2,x,y,z,a,fuel,locked,price) VALUES (%d,%d,%d,%d,%.2f,%.2f,%.2f,%.1f,60,0,%d)", P[playerid][pID], model, c1, c2, x, y, z, a, price);
    DB_Exec(q);
    new id = DB_LastId();
    format(q, sizeof q, "SELECT * FROM vehicles WHERE id=%d", id);
    new DBResult:r = DB_Query(q), v = INVALID_VEHICLE_ID;
    if (r) { if (db_num_rows(r)) v = Veh_Spawn(r); db_free_result(r); }
    Ach_Give(playerid, "car", "Свои колёса");
    return v;
}

stock bool:Veh_HasKey(playerid, v)
{
    if (!gV[v][vDB]) return false;
    if (gV[v][vOwner] == P[playerid][pID]) return true;
    new owner = FindPlayerByAcc(gV[v][vOwner]);
    if (owner != INVALID_PLAYER_ID && P[owner][pFamily] && P[owner][pFamily] == P[playerid][pFamily]) return true;
    return false;
}

stock Veh_Nearest(playerid, Float:range = 5.0)
{
    new best = INVALID_VEHICLE_ID, Float:bd = range, Float:x, Float:y, Float:z;
    for (new v = 1; v < MAX_VEHICLES; v++)
    {
        if (!GetVehicleModel(v)) continue;
        GetVehiclePos(v, x, y, z);
        new Float:d = GetPlayerDistanceFromPoint(playerid, x, y, z);
        if (d < bd) { bd = d; best = v; }
    }
    return best;
}

CMD:lock(playerid, const params[])
{
    new v = IsPlayerInAnyVehicle(playerid) ? GetPlayerVehicleID(playerid) : Veh_Nearest(playerid, 6.0);
    if (v == INVALID_VEHICLE_ID || !Veh_HasKey(playerid, v)) return Err(playerid, "Рядом нет вашего транспорта.");
    gV[v][vLocked] = !gV[v][vLocked];
    Veh_ApplyLock(v);
    PlayerPlaySound(playerid, 1145, 0.0, 0.0, 0.0);
    GameTextForPlayer(playerid, gV[v][vLocked] ? ("~r~LOCKED") : ("~g~UNLOCKED"), 1500, 6);
    SendMe(playerid, gV[v][vLocked] ? ("закрыл транспорт") : ("открыл транспорт"));
    return 1;
}

CMD:engine(playerid, const params[])
{
    new v = GetPlayerVehicleID(playerid);
    if (!v || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Err(playerid, "Вы не за рулём.");
    new e, l, al, d, bn, bt, o;
    GetVehicleParamsEx(v, e, l, al, d, bn, bt, o);
    if (e != 1 && gV[v][vFuel] <= 0.0) return Err(playerid, "Бак пуст.");
    if (e != 1 && gV[v][vDB] && !Veh_HasKey(playerid, v)) return Err(playerid, "У вас нет ключей от этого транспорта.");
    SetVehicleParamsEx(v, e == 1 ? 0 : 1, l, al, d, bn, bt, o);
    return 1;
}

CMD:park(playerid, const params[])
{
    new v = GetPlayerVehicleID(playerid);
    if (!v || gV[v][vOwner] != P[playerid][pID]) return Err(playerid, "Вы не в своём транспорте.");
    Veh_SaveOne(v);
    return Ok(playerid, "Место парковки сохранено.");
}

CMD:cars(playerid, const params[])
{
    Menu_Begin("Мой транспорт", "Найти, продать государству (50%)");
    new q[128]; format(q, sizeof q, "SELECT id,model,impound,price FROM vehicles WHERE owner=%d", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n; i++)
        {
            new t[48], d[48], model = DB_GetInt(r, "model");
            format(t, sizeof t, "%s (GJ %04d)", Veh_ModelName(model), DB_GetInt(r, "id"));
            format(d, sizeof d, "%s", DB_GetInt(r, "impound") ? ("На штрафстоянке") : ("Найти на карте"));
            Menu_Add(DB_GetInt(r, "id"), t, d, -1, "car");
            db_next_row(r);
        }
        db_free_result(r);
    }
    if (!gMBCount) Menu_Add(0, "Нет транспорта", "Автосалон в районе Коронадо");
    Menu_Send(playerid, "cars");
    return 1;
}

MENU:cars(playerid, item, const extra[])
{
    UI_Close(playerid);
    if (!item) return 1;
    if (StrEq(extra, "sell"))
    {
        new q[128]; format(q, sizeof q, "SELECT price FROM vehicles WHERE id=%d AND owner=%d", item, P[playerid][pID]);
        new price = DB_ScalarInt(q, -1);
        if (price < 0) return 1;
        for (new v = 1; v < MAX_VEHICLES; v++) if (gV[v][vDB] == item) { DestroyVehicle(v); Veh_Reset(v); }
        format(q, sizeof q, "DELETE FROM vehicles WHERE id=%d", item); DB_Exec(q);
        Money_Give(playerid, price / 2, "sell car");
        return Ok(playerid, "Транспорт продан государству.");
    }
    for (new v = 1; v < MAX_VEHICLES; v++)
    {
        if (gV[v][vDB] != item) continue;
        new Float:x, Float:y, Float:z; GetVehiclePos(v, x, y, z);
        SetPlayerCheckpoint(playerid, x, y, z, 3.0); P[playerid][pCP] = CP_GPS;
        return Ok(playerid, "Транспорт отмечен на карте.");
    }
    return Info(playerid, "Транспорт на штрафстоянке. Заберите его там.");
}

CMD:sellcar(playerid, const params[])
{
    new v = GetPlayerVehicleID(playerid);
    if (!v || gV[v][vOwner] != P[playerid][pID]) return Err(playerid, "Сядьте в свой транспорт.");
    new s[128]; format(s, sizeof s, "Продать %s государству за %s?", Veh_ModelName(gV[v][vModel]), FormatMoney(gV[v][vPrice] / 2));
    SetPVarInt(playerid, "sellcar", gV[v][vDB]);
    UI_Prompt(playerid, "sellcar", "Продажа транспорта", s, false, "", "Продать");
    return 1;
}

PROMPT:sellcar(playerid, response, const text[])
{
    UI_Close(playerid);
    if (response) menu_cars(playerid, GetPVarInt(playerid, "sellcar"), "sell");
    return 1;
}

stock Veh_ModelName(model)
{
    new t[24]; t = "Транспорт";
    for (new i = 0; i < sizeof gDealerModel; i++) if (gDealerModel[i] == model) strcpy(t, gDealerName[i]);
    return t;
}

// ---------- автосалон ----------
stock Dealer_Open(playerid)
{
    Menu_Begin("Автосалон «Godjo Motors»", "Каталог транспорта");
    for (new i = 0; i < sizeof gDealerModel; i++)
    {
        new d[32]; format(d, sizeof d, "Модель %d", gDealerModel[i]);
        Menu_Add(i, gDealerName[i], d, gDealerPrice[i], "car");
    }
    Menu_Add(100, "Тюнинг", "Сядьте в свою машину рядом с салоном", -1, "wrench");
    Menu_Send(playerid, "dealer");
    return 1;
}

MENU:dealer(playerid, item, const extra[])
{
    if (item == 100) { UI_Close(playerid); return Tune_Open(playerid); }
    if (item < 0 || item >= sizeof gDealerModel) return 1;
    UI_Close(playerid);
    if (!P[playerid][pLicDrive] && gDealerModel[item] != 462) return Err(playerid, "Нужны водительские права.");
    if (Veh_CountOwned(playerid) >= MAX_OWNED_CARS) return Err(playerid, "У вас максимальное число транспорта (3).");
    if (!Pay(playerid, gDealerPrice[item], "dealer")) return 1;
    new Float:x, Float:y, Float:z, Float:a;
    Loc_Get("dealer_spawn", x, y, z, a);
    new v = Veh_CreateOwned(playerid, gDealerModel[item], random(126), random(126), x, y, z, a, gDealerPrice[item]);
    if (v != INVALID_VEHICLE_ID) PutPlayerInVehicle(playerid, v, 0);
    new s[96]; format(s, sizeof s, "Поздравляем с покупкой %s! /lock — замок, /park — парковка.", gDealerName[item]);
    Ok(playerid, s);
    return 1;
}

static const gTuneColors[] = {0, 1, 3, 6, 79, 86, 126, 2};
stock Tune_Open(playerid)
{
    new v = GetPlayerVehicleID(playerid);
    if (!v || gV[v][vOwner] != P[playerid][pID]) return Err(playerid, "Сядьте за руль своего транспорта.");
    if (!IsPlayerAtLoc(playerid, "dealer", 30.0)) return Err(playerid, "Тюнинг доступен только у автосалона.");
    Menu_Begin("Тюнинг", Veh_ModelName(gV[v][vModel]));
    Menu_Add(1, "Покраска", "Случайный цвет из палитры", 500, "paint");
    Menu_Add(2, "Закись азота x2", "", 2000, "fire");
    Menu_Add(3, "Диски «Offroad»", "", 1500, "wheel");
    Menu_Add(4, "Диски «Shadow»", "", 1500, "wheel");
    Menu_Add(5, "Диски «Mega»", "", 1800, "wheel");
    Menu_Add(6, "Ремонт кузова", "", 300, "wrench");
    Menu_Send(playerid, "tune");
    return 1;
}

MENU:tune(playerid, item, const extra[])
{
    new v = GetPlayerVehicleID(playerid);
    if (!v || gV[v][vOwner] != P[playerid][pID]) return 1;
    static const prices[] = {0, 500, 2000, 1500, 1500, 1800, 300};
    if (item < 1 || item > 6) return 1;
    if (!Pay(playerid, prices[item], "tune")) return 1;
    switch (item)
    {
        case 1: { gV[v][vC1] = gTuneColors[random(sizeof gTuneColors)]; gV[v][vC2] = gV[v][vC1]; ChangeVehicleColor(v, gV[v][vC1], gV[v][vC2]); }
        case 2: { gV[v][vNitro] = 1; AddVehicleComponent(v, 1010); }
        case 3: { gV[v][vWheels] = 1025; AddVehicleComponent(v, 1025); }
        case 4: { gV[v][vWheels] = 1073; AddVehicleComponent(v, 1073); }
        case 5: { gV[v][vWheels] = 1074; AddVehicleComponent(v, 1074); }
        case 6: RepairVehicle(v);
    }
    Veh_SaveOne(v);
    PlayerPlaySound(playerid, 1133, 0.0, 0.0, 0.0);
    Ok(playerid, "Готово!");
    return 1;
}

// ---------- топливо ----------
stock Veh_FuelTick()
{
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsLogged(i) || GetPlayerState(i) != PLAYER_STATE_DRIVER) continue;
        new v = GetPlayerVehicleID(i), model = GetVehicleModel(v);
        if (model == 509 || model == 481 || model == 510) continue; // велосипеды
        new e, l, al, d, bn, bt, o;
        GetVehicleParamsEx(v, e, l, al, d, bn, bt, o);
        if (e != 1) continue;
        new Float:vx, Float:vy, Float:vz; GetVehicleVelocity(v, vx, vy, vz);
        new Float:sp = floatsqroot(vx * vx + vy * vy + vz * vz) * 180.0;
        gV[v][vFuel] -= 0.004 + sp * 0.00025;
        if (gV[v][vFuel] <= 0.0)
        {
            gV[v][vFuel] = 0.0;
            SetVehicleParamsEx(v, 0, l, al, d, bn, bt, o);
            Err(i, "Закончилось топливо! Заправьтесь на АЗС или используйте канистру.");
        }
    }
}

CMD:fill(playerid, const params[])
{
    new v = GetPlayerVehicleID(playerid);
    if (!v || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Err(playerid, "Вы не за рулём.");
    new biz = 0;
    if (IsPlayerAtLoc(playerid, "gas1", 15.0)) biz = 2;
    else if (IsPlayerAtLoc(playerid, "gas2", 15.0)) biz = 3;
    if (!biz) return Err(playerid, "Вы не на АЗС.");
    new liters = 100 - floatround(gV[v][vFuel], floatround_floor);
    if (liters <= 0) return Info(playerid, "Бак полон.");
    if (!Biz_HasStock(biz)) return Err(playerid, "На АЗС закончилось топливо.");
    new price = liters * Biz_FuelPrice(biz);
    if (!Pay(playerid, price, "fuel")) return 1;
    Biz_Sale(biz, price);
    gV[v][vFuel] = 100.0;
    new s[96]; format(s, sizeof s, "Заправлено %d л на %s.", liters, FormatMoney(price));
    return Ok(playerid, s);
}

stock Veh_UseCanister(playerid, slot)
{
    new v = Veh_Nearest(playerid, 4.0);
    if (v == INVALID_VEHICLE_ID || IsPlayerInAnyVehicle(playerid)) return Err(playerid, "Подойдите к транспорту (выйдите из него).");
    gV[v][vFuel] += 10.0; if (gV[v][vFuel] > 100.0) gV[v][vFuel] = 100.0;
    gInv[playerid][slot][1]--; Inv_SaveSlot(playerid, slot); Inv_Refresh(playerid);
    ApplyAnimation(playerid, "BD_FIRE", "wash_up", 4.0, 0, 0, 0, 0, 0);
    return Ok(playerid, "Залито 10 литров.");
}

stock Veh_UseRepair(playerid, slot)
{
    new v = Veh_Nearest(playerid, 4.0);
    if (v == INVALID_VEHICLE_ID || IsPlayerInAnyVehicle(playerid)) return Err(playerid, "Подойдите к транспорту (выйдите из него).");
    RepairVehicle(v);
    gInv[playerid][slot][1]--; Inv_SaveSlot(playerid, slot); Inv_Refresh(playerid);
    ApplyAnimation(playerid, "CAR", "Fixn_Car_Loop", 4.0, 0, 0, 0, 0, 0);
    return Ok(playerid, "Транспорт отремонтирован.");
}

// ---------- штрафстоянка ----------
stock Veh_Impound(v, const reason[])
{
    if (!gV[v][vDB]) { SetVehicleToRespawn(v); return; }
    new q[96]; format(q, sizeof q, "UPDATE vehicles SET impound=1 WHERE id=%d", gV[v][vDB]); DB_Exec(q);
    new owner = FindPlayerByAcc(gV[v][vOwner]);
    if (owner != INVALID_PLAYER_ID) { new s[128]; format(s, sizeof s, "Ваш транспорт отправлен на штрафстоянку (%s).", reason); Err(owner, s); }
    DestroyVehicle(v); Veh_Reset(v);
}

stock Impound_Open(playerid)
{
    Menu_Begin("Штрафстоянка", "Возврат транспорта: $500");
    new q[128]; format(q, sizeof q, "SELECT id,model FROM vehicles WHERE owner=%d AND impound=1", P[playerid][pID]);
    new DBResult:r = DB_Query(q);
    if (r)
    {
        for (new i = 0, n = db_num_rows(r); i < n; i++)
        {
            new t[48]; format(t, sizeof t, "%s (GJ %04d)", Veh_ModelName(DB_GetInt(r, "model")), DB_GetInt(r, "id"));
            Menu_Add(DB_GetInt(r, "id"), t, "", 500, "car");
            db_next_row(r);
        }
        db_free_result(r);
    }
    if (!gMBCount) Menu_Add(0, "Ваших машин здесь нет", "");
    Menu_Send(playerid, "impound");
    return 1;
}

MENU:impound(playerid, item, const extra[])
{
    UI_Close(playerid);
    if (!item) return 1;
    if (!Pay(playerid, 500, "impound")) return 1;
    new Float:x, Float:y, Float:z, Float:a, q[200];
    Loc_Get("impound_spawn", x, y, z, a);
    format(q, sizeof q, "UPDATE vehicles SET impound=0,x=%.2f,y=%.2f,z=%.2f,a=%.1f WHERE id=%d AND owner=%d", x, y, z + 0.5, a, item, P[playerid][pID]);
    DB_Exec(q);
    format(q, sizeof q, "SELECT * FROM vehicles WHERE id=%d", item);
    new DBResult:r = DB_Query(q);
    if (r) { if (db_num_rows(r)) { new v = Veh_Spawn(r); RepairVehicle(v); } db_free_result(r); }
    gTreasury += 500;
    return Ok(playerid, "Транспорт ждёт вас у выезда.");
}

stock Veh_OnEnter(playerid, vehicleid, ispassenger)
{
    if (!ispassenger && gV[vehicleid][vDB] && gV[vehicleid][vLocked] && !Veh_HasKey(playerid, vehicleid))
    {
        ClearAnimations(playerid);
        Err(playerid, "Транспорт закрыт.");
    }
}

stock Veh_OnStateDriver(playerid, v)
{
    if (!P[playerid][pLicDrive] && v != P[playerid][pExamVeh] && GetVehicleModel(v) != 462 && GetVehicleModel(v) != 509 && GetVehicleModel(v) != 510 && GetVehicleModel(v) != 481)
        Info(playerid, "Внимание: вы управляете без прав. Полиция может выписать штраф.");
    new e, l, al, d, bn, bt, o;
    GetVehicleParamsEx(v, e, l, al, d, bn, bt, o);
    if (gV[v][vFuel] > 0.0 && (!gV[v][vDB] || Veh_HasKey(playerid, v))) SetVehicleParamsEx(v, 1, l, al, d, bn, bt, o);
    else if (gV[v][vDB] && !Veh_HasKey(playerid, v)) { SetVehicleParamsEx(v, 0, l, al, d, bn, bt, o); Info(playerid, "У вас нет ключей — двигатель не заводится."); }
}
