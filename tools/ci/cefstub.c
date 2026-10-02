// Stub of samp-cef natives for sandbox testing only
typedef int cell;
typedef struct tagAMX AMX;
typedef cell (*AMX_NATIVE)(AMX*, cell*);
typedef struct { const char *name; AMX_NATIVE func; } AMX_NATIVE_INFO;
static void **amxf; static void (*logprintf)(const char*, ...);
static cell n_ok(AMX*a, cell*p){ return 1; }
static cell n_emit(AMX*a, cell*p){ return 1; }
static AMX_NATIVE_INFO natives[] = {
 {"cef_create_browser", n_ok},{"cef_destroy_browser", n_ok},{"cef_hide_browser", n_ok},{"cef_emit_event", n_emit},
 {"cef_subscribe", n_ok},{"cef_player_has_plugin", n_ok},{"cef_create_ext_browser", n_ok},{"cef_append_to_object", n_ok},
 {"cef_remove_from_object", n_ok},{"cef_toggle_dev_tools", n_ok},{"cef_set_audio_settings", n_ok},{"cef_focus_browser", n_ok},
 {"cef_always_listen_keys", n_ok},{"cef_load_url", n_ok},{"cef_on_player_connect", n_ok},{"cef_on_player_disconnect", n_ok},{0,0}};
unsigned int Supports(){ return 0x0200 | 0x10000; }
int Load(void **pp){ amxf = (void**)pp[0x10]; logprintf = (void(*)(const char*,...))pp[0]; logprintf("cef stub loaded"); return 1; }
void Unload(){}
int AmxLoad(AMX*amx){ int (*reg)(AMX*, const AMX_NATIVE_INFO*, int) = (int(*)(AMX*, const AMX_NATIVE_INFO*, int))amxf[33]; return reg(amx, natives, -1); }
int AmxUnload(AMX*amx){ return 0; }
