// =====================================================================
//  Мост с filterscript godjo_ai (жители-агенты, «симуляция игроков»).
//  Жители — настоящие строки в accounts (is_agent = 1): имя занято,
//  войти под ним нельзя. Уровень/пинг — через GM_BotRegister,
//  реплики — GM_BotSay, нападения — GM_BotAttacked (bots_bridge.inc).
// =====================================================================

// житель зашёл: регистрируем/находим его аккаунт, возвращаем id (или -1)
forward GM_AgentAttach(npcid, const name[]);
public GM_AgentAttach(npcid, const name[])
{
    new q[256];
    format(q, sizeof q, "SELECT id FROM accounts WHERE name='%s' AND is_agent=1", SqlEsc(name));
    new id = DB_ScalarInt(q, 0);
    if (id) return id;
    format(q, sizeof q, "SELECT COUNT(*) FROM accounts WHERE name='%s'", SqlEsc(name));
    if (DB_ScalarInt(q, 0) > 0) return -1;   // имя занято живым игроком
    format(q, sizeof q, "INSERT INTO accounts (name,pass,salt,reg_date,is_agent) VALUES ('%s','','','%s',1)", SqlEsc(name), DateStr());
    DB_Exec(q);
    format(q, sizeof q, "SELECT id FROM accounts WHERE name='%s' AND is_agent=1", SqlEsc(name));
    return DB_ScalarInt(q, -1);
}

forward GM_AdminLevel(playerid);
public GM_AdminLevel(playerid) return IsLogged(playerid) ? P[playerid][pAdmin] : 0;

forward GM_IsLogged(playerid);
public GM_IsLogged(playerid) return IsLogged(playerid) ? 1 : 0;

// аккаунт жителя: войти под ним нельзя
stock bool:Agent_IsReservedName(const name[])
{
    new q[160];
    format(q, sizeof q, "SELECT COUNT(*) FROM accounts WHERE name='%s' AND is_agent=1", SqlEsc(name));
    return DB_ScalarInt(q, 0) > 0;
}
