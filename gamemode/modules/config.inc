// =====================================================================
//  Godjo Role Play — конфигурация
// =====================================================================
#define GM_NAME             "Godjo Role Play"
#define GM_VERSION          "2.2.0"
#define GM_SHORT            "Godjo RP"
#define DB_FILE             "godjo.db"

#define CEF_BROWSER_ID      7001
#define CEF_URL             "godjo/index.html"

#define MAX_LOGIN_TRIES     3
#define LOGIN_TIMEOUT_SEC   180
#define HASH_ROUNDS         512

#define START_CASH          50
#define PAYDAY_MIN_PLAYED   15
#define BANK_PERCENT        1     // 0.1%% в час умножается на 10 ниже
#define TAX_PROPERTY        2     // %% от цены дома раз в сутки делим на 24
#define FUEL_PRICE          3

#define MAX_INV_SLOTS       30
#define MAX_INV_WEIGHT      25000 // граммы

#define VW_PREVIEW_BASE     5000
#define VW_HOUSE_BASE       10000
#define VW_BIZ_BASE         20000

// ---- цвета ----
#define C_WHITE   0xFFFFFFFF
#define C_GREY    0xAFAFAFFF
#define C_RED     0xFF5A5AFF
#define C_GREEN   0x6EE07AFF
#define C_YELLOW  0xFFD34DFF
#define C_BLUE    0x5AA9FFFF
#define C_PURPLE  0xB98CFFFF
#define C_ORANGE  0xFF9F43FF
#define C_ADMIN   0xFF6B6BFF
#define C_RADIO   0x8FD3FFFF
#define C_FAMILY  0xF5A3FFFF
#define C_ME      0xC2A2DAFF

#define H_WHITE  "{FFFFFF}"
#define H_GOLD   "{FFC94D}"
#define H_GREEN  "{6EE07A}"
#define H_RED    "{FF5A5A}"
#define H_GREY   "{AFAFAF}"

// ---- общие константы ----
enum { CP_NONE, CP_JOB, CP_GPS, CP_QUEST }   // тип обычного чекпоинта игрока
#define HOSP_VW  7                            // виртуальный мир больницы (свой интерьер, gen_interiors.inc)
