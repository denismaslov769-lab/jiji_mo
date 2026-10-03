// =====================================================================
//  Главное меню /mm (как на Arizona RP): плитки разделов на CEF,
//  а также быстрые команды для новичков.
// =====================================================================
new Float:gTmpX, Float:gTmpY, Float:gTmpZ;

CMD:mm(playerid, const params[])
{
    new j[1400], org[48], fam[40], ph[12], job[32];
    strcpy(org, Org_Name(P[playerid][pOrg]));
    if (P[playerid][pFamily]) strcpy(fam, Fam_Name(P[playerid][pFamily])); else fam = "—";
    if (P[playerid][pPhone][0]) strcpy(ph, P[playerid][pPhone]); else ph = "—";
    strcpy(job, Job_Name(P[playerid][pJob]));
    new active = 0, done = 0;
    for (new k = 0; k < MAX_PQ; k++) { if (!gPQ[playerid][k][pqQuest]) continue; if (gPQ[playerid][k][pqStatus] == 2) done++; else active++; }
    format(j, sizeof j, "{\"name\":\"%s\",\"id\":%d,\"lvl\":%d,\"exp\":%d,\"need\":%d,\"cash\":%d,\"bank\":%d,\"debt\":%d,\"org\":\"%s\",\"fam\":\"%s\",\"job\":\"%s\",\"phone\":\"%s\",\"hours\":%d,\"pass\":%d,\"med\":%d,\"lic\":%d,\"qa\":%d,\"qd\":%d,\"qt\":%d,\"online\":%d,\"warns\":%d,\"skin\":%d}",
        PName(playerid), playerid, P[playerid][pLevel], P[playerid][pExp], Lvl_Need(P[playerid][pLevel]), P[playerid][pCash], P[playerid][pBank], P[playerid][pDebt],
        JsonEsc(org), JsonEsc(fam), JsonEsc(job), ph, P[playerid][pPlayed] / 60, HasPassport(playerid), P[playerid][pMedcard], P[playerid][pLicDrive],
        active, done, gQICount, Iter_OnlineAll(), P[playerid][pWarns], P[playerid][pSkin]);
    UI_Open(playerid, "mm", j);
    return 1;
}
CMD:menu(playerid, const params[]) return cmd_mm(playerid, params);
CMD:mn(playerid, const params[]) return cmd_mm(playerid, params);

UI:mm(playerid, const a[])
{
    new act[16];
    CefField(a, 1, act);
    UI_Close(playerid);
    if (StrEq(act, "inv")) return Inv_Open(playerid);
    if (StrEq(act, "phone")) return Phone_Open(playerid);
    if (StrEq(act, "quest")) return cmd_quest(playerid, "\1");
    if (StrEq(act, "jobs")) return cmd_jobs(playerid, "\1");
    if (StrEq(act, "gps")) return cmd_gps(playerid, "\1");
    if (StrEq(act, "ach")) return cmd_ach(playerid, "\1");
    if (StrEq(act, "cars")) return cmd_cars(playerid, "\1");
    if (StrEq(act, "anims")) return cmd_anims(playerid, "\1");
    if (StrEq(act, "rules")) return cmd_rules(playerid, "\1");
    if (StrEq(act, "help")) return cmd_help(playerid, "\1");
    if (StrEq(act, "cmds")) return cmd_cmds(playerid, "\1");
    if (StrEq(act, "settings")) return cmd_settings(playerid, "\1");
    if (StrEq(act, "pass")) return Docs_Show(playerid, playerid, "passport");
    if (StrEq(act, "medc")) return Docs_Show(playerid, playerid, "med");
    if (StrEq(act, "lic")) return Docs_Show(playerid, playerid, "lic");
    if (StrEq(act, "report")) { UI_Prompt(playerid, "mmreport", "Связь с администрацией", "Опишите вопрос или жалобу. Администратор ответит в чат.", true, "Ваше сообщение", "Отправить"); return 1; }
    if (StrEq(act, "travel"))
    {
        new st = Travel_Nearest(playerid);
        if (st == -1) { Loc_Get("ls_station", gTmpX, gTmpY, gTmpZ); SetPlayerCheckpoint(playerid, gTmpX, gTmpY, gTmpZ, 3.0); P[playerid][pCP] = CP_GPS; return Info(playerid, "Ближайшая касса — вокзал Юнити, отмечена на карте."); }
        return Travel_Desk(playerid, st);
    }
    return 1;
}

PROMPT:mmreport(playerid, resp, const text[])
{
    UI_Close(playerid);
    if (resp != 1 || isnull(text)) return 1;
    return cmd_report(playerid, text);
}

CMD:cmds(playerid, const params[])
{
    Menu_Begin("Команды сервера", "Основные команды · полный список в COMMANDS.md");
    Menu_Add(1, "/mm (/menu)", "Главное меню: персонаж, документы, квесты, работа", -1, "home");
    Menu_Add(2, "/quest", "Квесты: отслеживать, отложить, новые истории", -1, "star");
    Menu_Add(3, "/gps · /jobs", "Навигатор по городу и список работ", -1, "pin");
    Menu_Add(4, "/n [текст]", "Вопрос в чат новичков (видят все)", -1, "help");
    Menu_Add(5, "/me /do /try", "Отыгровка действий", -1, "chat");
    Menu_Add(6, "/s /w /b", "Крик, шёпот, OOC-чат", -1, "chat");
    Menu_Add(7, "/hi [id]", "Пожать руку игроку рядом", -1, "handshake");
    Menu_Add(8, "/pass /med /lic [id]", "Показать документы", -1, "idcard");
    Menu_Add(9, "/phone · /number · /sms", "Телефон (клавиша P)", -1, "phone");
    Menu_Add(10, "/stats · /ach", "Статистика и достижения", -1, "trophy");
    Menu_Add(11, "/anims · /stopanim", "Анимации", -1, "party");
    Menu_Add(12, "/cars · /engine · /lock · /fill", "Транспорт", -1, "car");
    Menu_Add(13, "/report", "Связь с администрацией", -1, "info");
    Menu_Add(14, "/cancel", "Сбросить метку на карте", -1, "x");
    Menu_Send(playerid, "cmds");
    return 1;
}
MENU:cmds(playerid, item, const extra[]) return UI_Close(playerid);

// чат новичков: вопрос видят все, отвечать могут игроки 3+ уровня и админы
CMD:n(playerid, const params[])
{
    if (isnull(params)) return Err(playerid, "/n [вопрос]");
    if (P[playerid][pMute] > 0) return Err(playerid, "У вас молчанка.");
    if (gettime() - GetPVarInt(playerid, "nCD") < 20) return Err(playerid, "Не чаще раза в 20 секунд.");
    if (P[playerid][pLevel] > 3 && P[playerid][pAdmin] == 0 && GetPVarInt(playerid, "nAnswer") == 0)
    {
        SetPVarInt(playerid, "nAnswer", 1);
        Info(playerid, "Вы отвечаете в чат новичков как опытный игрок. Будьте вежливы.");
    }
    SetPVarInt(playerid, "nCD", gettime());
    new s[160];
    format(s, sizeof s, "[Новичкам] %s %s[%d]: %s", P[playerid][pAdmin] ? ("Администратор") : P[playerid][pLevel] <= 3 ? ("Новичок") : ("Игрок"), PName(playerid), playerid, params);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++) if (IsLogged(i)) SCM(i, 0x9EE6A0FF, s);
    return 1;
}

CMD:hi(playerid, const params[])
{
    new t = isnull(params) ? GetNearestPlayer(playerid, 2.5) : GetParamPlayer(params, 0);
    if (t == INVALID_PLAYER_ID || t == playerid || !IsPlayerConnected(t)) return Err(playerid, "Рядом никого нет. /hi [id]");
    new Float:x, Float:y, Float:z; GetPlayerPos(t, x, y, z);
    if (!IsPlayerInRangeOfPoint(playerid, 2.5, x, y, z)) return Err(playerid, "Подойдите ближе.");
    ApplyAnimation(playerid, "GANGS", "hndshkfa_swt", 4.1, 0, 0, 0, 0, 0);
    if (!IsPlayerNPC(t)) ApplyAnimation(t, "GANGS", "hndshkfa_swt", 4.1, 0, 0, 0, 0, 0);
    new s[96]; format(s, sizeof s, "пожал руку %s", PName(t));
    SendMe(playerid, s);
    return 1;
}

CMD:cancel(playerid, const params[])
{
    if (P[playerid][pCP] == CP_JOB) return Err(playerid, "Это рабочая метка — закончите смену: /jobend");
    DisablePlayerCheckpoint(playerid); P[playerid][pCP] = CP_NONE;
    Quest_UpdateTracker(playerid);
    return Ok(playerid, "Метка сброшена.");
}

CMD:sms(playerid, const params[]) return Phone_Open(playerid);
