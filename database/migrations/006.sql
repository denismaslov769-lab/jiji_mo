-- 006: жители-агенты (filterscript godjo_ai). Аккаунт жителя помечен is_agent=1.
-- Таблицы ai_* filterscript создаёт и сам (CREATE IF NOT EXISTS) — здесь для полноты схемы.
ALTER TABLE accounts ADD COLUMN is_agent INTEGER DEFAULT 0;
CREATE TABLE IF NOT EXISTS ai_agents (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE COLLATE NOCASE,
 sex INTEGER, skin INTEGER, level INTEGER, ping INTEGER, traits TEXT, needs TEXT, money INTEGER DEFAULT 300,
 job INTEGER DEFAULT 0, work_from INTEGER, work_to INTEGER, wake INTEGER, sleep INTEGER, home INTEGER, work INTEGER,
 enabled INTEGER DEFAULT 1, last_logout INTEGER DEFAULT 0, created INTEGER);
CREATE TABLE IF NOT EXISTS ai_memory (id INTEGER PRIMARY KEY AUTOINCREMENT, agent INTEGER, kind INTEGER, who TEXT, value INTEGER, imp INTEGER, t INTEGER);
CREATE INDEX IF NOT EXISTS ai_memory_agent ON ai_memory(agent, t);
CREATE TABLE IF NOT EXISTS ai_relations (agent INTEGER, who TEXT COLLATE NOCASE, aff INTEGER DEFAULT 0, fam INTEGER DEFAULT 0, seen INTEGER, PRIMARY KEY (agent, who));
CREATE TABLE IF NOT EXISTS ai_sessions (id INTEGER PRIMARY KEY AUTOINCREMENT, agent INTEGER, login INTEGER, logout INTEGER DEFAULT 0, reason TEXT DEFAULT '');
