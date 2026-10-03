// =====================================================================
//   Godjo Role Play — игровой мод для SA-MP 0.3.7
//   Все системы написаны с нуля. Кодировка исходников: UTF-8
//   (при сборке перекодируется в Windows-1251 — см. tools/build-gamemode.sh)
// =====================================================================
#pragma warning disable 239
#pragma warning disable 214
#pragma dynamic 65536

#include <a_samp>
#undef MAX_PLAYERS
#define MAX_PLAYERS 100
#include <cef>

#include "modules/config.inc"
#include "modules/util.inc"
#include "modules/db.inc"
#include "modules/player.inc"
#include "modules/ui.inc"
#include "modules/locations.inc"
#include "modules/items.inc"
#include "modules/accounts.inc"
#include "modules/economy.inc"
#include "modules/needs.inc"
#include "modules/levels.inc"
#include "modules/documents.inc"
#include "modules/quests.inc"
#include "modules/vova.inc"
#include "modules/bottles.inc"
#include "modules/shops.inc"
#include "modules/jobs.inc"
#include "modules/autoschool.inc"
#include "modules/vehicles.inc"
#include "modules/rent.inc"
#include "modules/houses.inc"
#include "modules/business.inc"
#include "modules/orgs.inc"
#include "modules/police.inc"
#include "modules/hospital.inc"
#include "modules/phone.inc"
#include "modules/families.inc"
#include "modules/casino.inc"
#include "modules/fishing.inc"
#include "modules/treasure.inc"
#include "modules/admin.inc"
#include "modules/mapping.inc"
#include "modules/extras.inc"
#include "modules/main.inc"

main() {}
