// =====================================================================
//  База данных: встроенный SQLite. Ничего настраивать не нужно —
//  файл scriptfiles/godjo.db создаётся сам при первом запуске.
// =====================================================================
new DB:gDB;

stock DB_Exec(const q[])
{
    new DBResult:r = db_query(gDB, q);
    if (r) db_free_result(r);
    #if defined GJ_DEBUG_SQL
    else printf("[SQL?] %.120s", q);
    #endif
}

stock DBResult:DB_Query(const q[])
{
    return db_query(gDB, q);
}

stock DB_GetInt(DBResult:r, const field[])
{
    new t[24];
    db_get_field_assoc(r, field, t, sizeof t);
    return strval(t);
}

stock Float:DB_GetFloat(DBResult:r, const field[])
{
    new t[24];
    db_get_field_assoc(r, field, t, sizeof t);
    return floatstr(t);
}

stock DB_GetStr(DBResult:r, const field[], dest[], size = sizeof dest)
{
    dest[0] = 0;
    db_get_field_assoc(r, field, dest, size);
    for (new i = 0; dest[i]; i++) dest[i] &= 0xFF;   // кириллица без знакового расширения
}

stock DB_ScalarInt(const q[], def = 0)
{
    new DBResult:r = db_query(gDB, q), v = def;
    if (r)
    {
        if (db_num_rows(r) > 0)
        {
            new t[24];
            db_get_field(r, 0, t, sizeof t);
            v = strval(t);
        }
        db_free_result(r);
    }
    return v;
}

stock DB_LastId()
{
    return DB_ScalarInt("SELECT last_insert_rowid()");
}

// Выполняет SQL-файл из scriptfiles построчно, собирая команды до ';'
stock DB_RunFile(const path[])
{
    new File:f = fopen(path, io_read);
    if (!f)
    {
        printf("[DB] Не найден файл %s", path);
        return 0;
    }
    new line[1024], stmt[4096], cnt = 0;
    stmt[0] = 0;
    while (fread(f, line))
    {
        // убрать перевод строки
        new len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        if (len >= 2 && line[0] == '-' && line[1] == '-') continue;
        if (len == 0) continue;
        if (stmt[0]) strcat(stmt, " ");
        strcat(stmt, line);
        if (line[len - 1] == ';')
        {
            DB_Exec(stmt);
            stmt[0] = 0;
            cnt++;
        }
    }
    fclose(f);
    printf("[DB] %s: выполнено команд: %d", path, cnt);
    return cnt;
}

stock DB_Init()
{
    gDB = db_open(DB_FILE);
    if (!gDB)
    {
        print("[DB] ОШИБКА: не удалось открыть базу данных!");
        return 0;
    }
    DB_Exec("PRAGMA journal_mode=WAL");
    DB_Exec("PRAGMA synchronous=NORMAL");
    DB_RunFile("godjo/schema.sql");
    if (DB_ScalarInt("SELECT COUNT(*) FROM meta WHERE k='seeded'") == 0)
    {
        DB_Exec("BEGIN");
        DB_RunFile("godjo/seed.sql");
        DB_Exec("INSERT INTO meta VALUES ('seeded','1')");
        DB_Exec("COMMIT");
        print("[DB] Стартовые данные загружены.");
    }
    DB_RunMigrations();
    return 1;
}

// Миграции: scriptfiles/godjo/migrations/001.sql, 002.sql ...
stock DB_RunMigrations()
{
    new ver = DB_ScalarInt("SELECT v FROM meta WHERE k='migration'", 0);
    new path[64], q[96];
    for (new i = ver + 1; i < 1000; i++)
    {
        format(path, sizeof path, "godjo/migrations/%03d.sql", i);
        if (!fexist(path)) break;
        DB_RunFile(path);
        format(q, sizeof q, "INSERT OR REPLACE INTO meta VALUES ('migration','%d')", i);
        DB_Exec(q);
        printf("[DB] Миграция %03d применена", i);
    }
}

stock Log(const type[], const who[], const text[])
{
    new q[400];
    format(q, sizeof q, "INSERT INTO logs (date,type,acc,text) VALUES ('%s','%s','%s','%s')", DateStr(), type, SqlEsc(who), SqlEsc(text));
    DB_Exec(q);
}
