// =====================================================================
//  NPC штата: квестовые персонажи и жители (таблица npcs).
//  Актёр + интерактивная точка (ALT). Разговор: сначала квестовые шаги,
//  потом предложение нового квеста, иначе — обычная реплика.
// =====================================================================
#define MAX_NPCS 64
enum E_NPC { nCode[24], nName[32], nSkin, nActor, nLoc[24], nGreet[160], nCity[16] }
new gNpc[MAX_NPCS][E_NPC], gNpcCount;

stock Npc_LoadAll()
{
    gNpcCount = 0;
    new DBResult:r = DB_Query("SELECT code,name,skin,loc,anim,greet FROM npcs");
    if (!r) return;
    for (new i = 0, n = db_num_rows(r); i < n && gNpcCount < MAX_NPCS; i++)
    {
        new k = gNpcCount;
        DB_GetStr(r, "code", gNpc[k][nCode], 24);
        DB_GetStr(r, "name", gNpc[k][nName], 32);
        gNpc[k][nSkin] = DB_GetInt(r, "skin");
        DB_GetStr(r, "loc", gNpc[k][nLoc], 24);
        DB_GetStr(r, "greet", gNpc[k][nGreet], 160);
        new anim[32]; DB_GetStr(r, "anim", anim);
        db_next_row(r);
        new Float:x, Float:y, Float:z, Float:a, it, vw;
        if (!Loc_Get(gNpc[k][nLoc], x, y, z, a, it, vw)) { printf("[NPC] нет точки %s", gNpc[k][nLoc]); continue; }
        new act = CreateActor(gNpc[k][nSkin], x, y, z, a);
        gNpc[k][nActor] = act;
        if (act != INVALID_ACTOR_ID)
        {
            SetActorVirtualWorld(act, vw);
            SetActorInvulnerable(act, true);
            Npc_Anim(act, anim);
        }
        new lbl[96]; format(lbl, sizeof lbl, H_GOLD "%s\n" H_GREY "[ALT] Поговорить", gNpc[k][nName]);
        Point_Add(PT_NPC, k, x, y, z, vw, lbl);
        gNpcCount++;
    }
    db_free_result(r);
    printf("[NPC] Персонажей: %d", gNpcCount);
}

stock Npc_Anim(act, const anim[])
{
    if (StrEq(anim, "sit")) ApplyActorAnimation(act, "PED", "SEAT_idle", 4.1, 1, 0, 0, 0, 0);
    else if (StrEq(anim, "smoke")) ApplyActorAnimation(act, "SMOKING", "M_smklean_loop", 4.1, 1, 0, 0, 0, 0);
    else if (StrEq(anim, "lean")) ApplyActorAnimation(act, "GANGS", "leanIDLE", 4.1, 1, 0, 0, 0, 0);
    else if (StrEq(anim, "look")) ApplyActorAnimation(act, "COP_AMBIENT", "Coplook_loop", 4.1, 1, 0, 0, 0, 0);
    else if (StrEq(anim, "fish")) ApplyActorAnimation(act, "SWORD", "sword_IDLE", 4.1, 1, 0, 0, 0, 0);
    else if (StrEq(anim, "phone")) ApplyActorAnimation(act, "PED", "phone_talk", 4.1, 1, 0, 0, 0, 0);
    else if (StrEq(anim, "work")) ApplyActorAnimation(act, "COP_AMBIENT", "Copbrowse_loop", 4.1, 1, 0, 0, 0, 0);
    else if (StrEq(anim, "arms")) ApplyActorAnimation(act, "COP_AMBIENT", "Coplook_loop", 4.1, 1, 0, 0, 0, 0);
}

stock Npc_Find(const code[])
{
    for (new i = 0; i < gNpcCount; i++) if (StrEq(gNpc[i][nCode], code)) return i;
    return -1;
}

stock bool:Npc_Loc(const code[], out[], size = sizeof out)
{
    new i = Npc_Find(code);
    if (i == -1) return false;
    strcpy(out, gNpc[i][nLoc], size);
    return true;
}

stock Npc_NameByCode(const code[], out[], size = sizeof out)
{
    new i = Npc_Find(code);
    if (i == -1) strcpy(out, "—", size); else strcpy(out, gNpc[i][nName], size);
}

stock Npc_Dialog(playerid, n, node, const text[], const o1[] = "", const o2[] = "", const o3[] = "")
{
    if (gNpc[n][nActor] != INVALID_ACTOR_ID)
    {   // NPC поворачивается к игроку
        new Float:x, Float:y, Float:z, Float:ax, Float:ay, Float:az;
        GetPlayerPos(playerid, x, y, z); GetActorPos(gNpc[n][nActor], ax, ay, az);
        new Float:ang = -atan2(x - ax, y - ay); if (ang < 0.0) ang += 360.0;
        SetActorFacingAngle(gNpc[n][nActor], ang);
        ApplyActorAnimation(gNpc[n][nActor], "PED", "IDLE_chat", 4.1, 0, 1, 1, 0, 0);
    }
    return Dlg_ShowAs(playerid, gNpc[n][nName], gNpc[n][nSkin], node, text, o1[0] ? o1 : ("Пока"), o2, o3);
}

stock Npc_Talk(playerid, n)
{
    if (n < 0 || n >= gNpcCount) return 1;
    SetPVarInt(playerid, "npcTalk", n);
    if (Quest_NpcTalk(playerid, n)) return 1;
    if (Quest_OfferFrom(playerid, n)) return 1;
    Quest_Event(playerid, "talk", gNpc[n][nCode]);
    if (!gNpc[n][nGreet][0]) return Npc_Dialog(playerid, n, 100, "Добрый день.", "До встречи");
    return Npc_Dialog(playerid, n, 100, gNpc[n][nGreet], "До встречи");
}

stock Npc_DlgResponse(playerid, node, opt)
{
    if (node == 100) return UI_Close(playerid);
    if (node >= 200 && node < 400) return Quest_DlgResponse(playerid, node, opt);
    return UI_Close(playerid);
}
