// =====================================================================
//  Утилиты: строки, параметры команд, команды, протокол CEF
// =====================================================================
#define isnull(%1) ((!(%1[0])) || (((%1[0]) == '\1') && (!(%1[1]))))
#define CMD:%1(%2) forward cmd_%1(%2); public cmd_%1(%2)
#define SCM SendClientMessage

stock strcpy(dest[], const src[], size = sizeof dest)
{
    dest[0] = 0;
    strcat(dest, src, size);
}

stock StrToLower(s[])
{
    for (new i = 0; s[i]; i++) s[i] = tolower(s[i]);
}

stock bool:StrEq(const a[], const b[])
{
    if (!a[0] && !b[0]) return true;
    if (!a[0] || !b[0]) return false;
    return strcmp(a, b, false) == 0;
}

// Достаёт idx-й (с нуля) токен через пробел
stock GetParam(const src[], idx, dest[], size = sizeof dest)
{
    dest[0] = 0;
    if (isnull(src)) return 0;
    new i = 0, cur = 0, len = strlen(src);
    while (i < len)
    {
        while (i < len && src[i] == ' ') i++;
        if (i >= len) break;
        new st = i;
        while (i < len && src[i] != ' ') i++;
        if (cur == idx)
        {
            new n = i - st;
            if (n >= size) n = size - 1;
            for (new k = 0; k < n; k++) dest[k] = src[st + k];
            dest[n] = 0;
            return 1;
        }
        cur++;
    }
    return 0;
}

// Остаток строки начиная с idx-го токена
stock GetRest(const src[], idx, dest[], size = sizeof dest)
{
    dest[0] = 0;
    if (isnull(src)) return 0;
    new i = 0, cur = 0, len = strlen(src);
    while (i < len)
    {
        while (i < len && src[i] == ' ') i++;
        if (i >= len) break;
        if (cur == idx)
        {
            strcat(dest, src[i], size);
            return 1;
        }
        while (i < len && src[i] != ' ') i++;
        cur++;
    }
    return 0;
}

stock GetParamInt(const src[], idx, def = -1)
{
    new t[16];
    if (!GetParam(src, idx, t)) return def;
    if (!IsNumeric(t)) return def;
    return strval(t);
}

stock bool:IsNumeric(const s[])
{
    if (!s[0]) return false;
    new i = (s[0] == '-') ? 1 : 0;
    if (!s[i]) return false;
    for (; s[i]; i++) if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

// id игрока из параметра (число или часть ника)
stock GetParamPlayer(const src[], idx)
{
    new t[MAX_PLAYER_NAME];
    if (!GetParam(src, idx, t)) return INVALID_PLAYER_ID;
    if (IsNumeric(t))
    {
        new id = strval(t);
        if (IsPlayerConnected(id)) return id;
        return INVALID_PLAYER_ID;
    }
    new name[MAX_PLAYER_NAME];
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i)) continue;
        GetPlayerName(i, name, sizeof name);
        if (strfind(name, t, true) != -1) return i;
    }
    return INVALID_PLAYER_ID;
}

stock PName(playerid)
{
    new n[MAX_PLAYER_NAME];
    GetPlayerName(playerid, n, sizeof n);
    return n;
}

// Имя_Фамилия: обе части с заглавной, латиница, 3..20
stock bool:IsRpName(const name[])
{
    new len = strlen(name), us = -1, cnt = 0;
    if (len < 6 || len > 20) return false;
    for (new i = 0; i < len; i++)
    {
        new c = name[i];
        if (c == '_') { us = i; cnt++; continue; }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) return false;
    }
    if (cnt != 1 || us < 2 || us > len - 3) return false;
    if (!(name[0] >= 'A' && name[0] <= 'Z')) return false;
    if (!(name[us + 1] >= 'A' && name[us + 1] <= 'Z')) return false;
    for (new i = 1; i < len; i++)
    {
        if (i == us || i == us + 1) continue;
        if (name[i] >= 'A' && name[i] <= 'Z') return false;
    }
    return true;
}

// Экранирование для SQL (одинарные кавычки)
stock SqlEsc(const src[])
{
    new out[600], j = 0;
    for (new i = 0; src[i] && j < sizeof(out) - 2; i++)
    {
        if (src[i] == '\'') out[j++] = '\'';
        out[j++] = src[i];
    }
    out[j] = 0;
    return out;
}

// Экранирование для JSON
stock JsonEsc(const src[])
{
    new out[600], j = 0;
    for (new i = 0; src[i] && j < sizeof(out) - 3; i++)
    {
        new c = src[i] & 0xFF;   // строки из SQLite приходят со знаком (кириллица < 0)
        if (c == '"' || c == '\\') { out[j++] = '\\'; out[j++] = c; }
        else if (c == '\n') { out[j++] = '\\'; out[j++] = 'n'; }
        else if (c < 32) continue;
        else out[j++] = c;
    }
    out[j] = 0;
    return out;
}

// --- протокол CEF: поля разделены '|', экранированы %25 %20 %7C ---
stock CefField(const src[], idx, dest[], size = sizeof dest)
{
    dest[0] = 0;
    new cur = 0, i = 0, j = 0, len = strlen(src);
    while (i <= len)
    {
        if (src[i] == '|' || src[i] == 0)
        {
            if (cur == idx) { dest[j] = 0; return 1; }
            cur++;
            if (src[i] == 0) break;
            i++;
            continue;
        }
        if (cur == idx && j < size - 1)
        {
            if (src[i] == '%' && i + 2 < len)
            {
                new h1 = HexVal(src[i + 1]), h2 = HexVal(src[i + 2]);
                if (h1 >= 0 && h2 >= 0) { dest[j++] = h1 * 16 + h2; i += 3; continue; }
            }
            dest[j++] = src[i];
        }
        i++;
    }
    dest[j] = 0;
    return 0;
}

stock CefFieldInt(const src[], idx)
{
    new t[16];
    CefField(src, idx, t);
    return strval(t);
}

stock HexVal(c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

stock FormatMoney(amount)
{
    new s[24], tmp[16], len, j = 0, neg = amount < 0;
    format(tmp, sizeof tmp, "%d", neg ? -amount : amount);
    len = strlen(tmp);
    if (neg) s[j++] = '-';
    s[j++] = '$';
    for (new i = 0; i < len; i++)
    {
        s[j++] = tmp[i];
        if ((len - i - 1) % 3 == 0 && i != len - 1) s[j++] = '.';
    }
    s[j] = 0;
    return s;
}

stock Float:DistBetween(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2)
{
    return floatsqroot((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2));
}

stock GetNearestPlayer(playerid, Float:range)
{
    new Float:x, Float:y, Float:z, best = INVALID_PLAYER_ID, Float:bd = range;
    GetPlayerPos(playerid, x, y, z);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (i == playerid || !IsPlayerConnected(i)) continue;
        if (GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;
        new Float:d = GetPlayerDistanceFromPoint(i, x, y, z);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

stock ProxMsg(playerid, Float:range, color, const msg[])
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
    {
        if (!IsPlayerConnected(i)) continue;
        if (GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;
        if (GetPlayerDistanceFromPoint(i, x, y, z) <= range) SCM(i, color, msg);
    }
}

stock SendMe(playerid, const action[])
{
    new s[144];
    format(s, sizeof s, "%s %s", PName(playerid), action);
    ProxMsg(playerid, 20.0, C_ME, s);
}

stock Rand(min, max) return min + random(max - min + 1);

stock GetTimestamp() return gettime();

stock DateStr()
{
    new s[24], y, m, d, h, mi, se;
    getdate(y, m, d); gettime(h, mi, se);
    format(s, sizeof s, "%04d-%02d-%02d %02d:%02d", y, m, d, h, mi);
    return s;
}

stock RandomString(dest[], len)
{
    static const chars[] = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    for (new i = 0; i < len; i++) dest[i] = chars[random(sizeof(chars) - 1)];
    dest[len] = 0;
}
