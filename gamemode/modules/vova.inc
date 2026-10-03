// =====================================================================
//  Дядя Вова — квестовый персонаж (actor) с CEF-диалогами
// =====================================================================
#define VOVA_SKIN 137
#define VOVA_NUMBER "5550101"
new gVovaActor = INVALID_ACTOR_ID, gVovaGarageActor = INVALID_ACTOR_ID;
new gDlgNode[MAX_PLAYERS];

stock Vova_Create()
{
    new Float:x, Float:y, Float:z, Float:a;
    Loc_Get("vova", x, y, z, a);
    gVovaActor = CreateActor(VOVA_SKIN, x, y, z, a);
    SetActorInvulnerable(gVovaActor, true);
    ApplyActorAnimation(gVovaActor, "PED", "SEAT_idle", 4.1, 1, 0, 0, 0, 0);
    Point_Add(PT_VOVA, 0, x, y, z, 0, H_GOLD "Дядя Вова\n" H_GREY "[ALT] Поговорить");
    // бочка с огнём, ящики и прочий быт ночлежки — в mapping.inc
    Loc_Get("vova_garage", x, y, z, a);
    gVovaGarageActor = CreateActor(VOVA_SKIN, x + 1.5, y, z, a);
    SetActorInvulnerable(gVovaGarageActor, true);
    Point_Add(PT_VOVA, 1, x + 1.5, y, z, 0, H_GOLD "Дядя Вова\n" H_GREY "[ALT] Поговорить");
}

stock Dlg_Show(playerid, node, const text[], const o1[] = "", const o2[] = "", const o3[] = "", const o4[] = "")
{
    gDlgNode[playerid] = node;
    new j[1800];
    format(j, sizeof j, "{\"npc\":\"Дядя Вова\",\"node\":%d,\"skin\":%d,\"text\":\"%s\",\"options\":[", node, VOVA_SKIN, JsonEsc(text));
    new t[200], n = 0;
    if (o1[0]) { format(t, sizeof t, "%s{\"id\":1,\"t\":\"%s\"}", n++ ? "," : "", JsonEsc(o1)); strcat(j, t); }
    if (o2[0]) { format(t, sizeof t, "%s{\"id\":2,\"t\":\"%s\"}", n++ ? "," : "", JsonEsc(o2)); strcat(j, t); }
    if (o3[0]) { format(t, sizeof t, "%s{\"id\":3,\"t\":\"%s\"}", n++ ? "," : "", JsonEsc(o3)); strcat(j, t); }
    if (o4[0]) { format(t, sizeof t, "%s{\"id\":4,\"t\":\"%s\"}", n++ ? "," : "", JsonEsc(o4)); strcat(j, t); }
    strcat(j, "]}");
    UI_Open(playerid, "dialog", j);
    return 1;
}

stock Vova_Talk(playerid, which)
{
    new step = Quest_GetStep(playerid, 1);
    if (which == 1)
    {
        if (step == 20 && Quest_IsActive(playerid, 1)) return Vova_Final(playerid);
        return Dlg_Show(playerid, 0, "Ты рано пришёл. Здесь мы поговорим, когда придёт время.", "Ухожу");
    }
    if (step == 0)
    {
        new intro[300];
        switch (P[playerid][pOrigin])
        {
            case 1: intro = "Из деревни, да? По глазам вижу — город обещал тебе золотые горы. Мне он тоже много чего обещал. Садись к огню, не стесняйся.";
            case 2: intro = "Только откинулся? Не дёргайся, я не мент. Тут у каждого своё прошлое. Главное — какое будет будущее.";
            case 3: intro = "Бывший бизнесмен? Ха. Я когда-то держал свою мастерскую. Деньги приходят и уходят, а руки и голова остаются.";
            default: intro = "Ещё один местный, которого город выплюнул на обочину... Ничего, бывает. Садись к огню, погрейся.";
        }
        return Dlg_Show(playerid, 1, intro, "Кто вы?", "Мне нужна помощь", "Отстань, старик");
    }
    if (step == 999) return Dlg_Show(playerid, 0, "А, это ты! Заходи, когда будет время. Мне есть чем гордиться — тобой.", "Обнять старика", "До встречи");
    if (!Quest_IsActive(playerid, 1))
        return Dlg_Show(playerid, 20, "Решил передохнуть? Правильно. Но не задерживайся — жизнь сама себя не наладит.", "Продолжить историю", "Позже");
    if (step == 16) return Dlg_Show(playerid, 30, "Посмотри на себя: паспорт, работа, права, своя крыша. Месяц назад ты собирал бутылки. Теперь главный вопрос — кем ты станешь?", "Хочу служить государству", "Хочу своё дело", "Улица — мой дом", "Подумаю ещё");
    if (step == 20) return Dlg_Show(playerid, 0, "Встретимся в моём старом гараже в Уиллоуфилде. Там и поговорим. Я отметил место у тебя на карте.", "Еду");
    new si = Quest_StepIdx(1, step);
    new t[300];
    if (si != -1) format(t, sizeof t, "Ну что, как успехи? Сейчас твоя задача — %s. %s", gQS[si][qsTitle], gQS[si][qsHint]);
    else t = "Ну что, как успехи?";
    return Dlg_Show(playerid, 10, t, "Понял, иду", "Хочу отложить это", "Расскажи о себе");
}

stock Vova_Final(playerid)
{
    return Dlg_Show(playerid, 40, "Двадцать лет назад это был лучший гараж в Лос-Сантосе. Мой партнёр, Гоша, связался с мафией и повесил на меня их угнанные машины. Пять лет тюрьмы. Жена ушла, мастерскую отобрали. Я вышел никем — и остался под мостом.", "Почему ты не вернул всё?", "Мне жаль, Вова");
}

UI:dlg(playerid, const a[])
{
    new node = CefFieldInt(a, 1), opt = CefFieldInt(a, 2);
    if (node != gDlgNode[playerid]) return 1;
    switch (node)
    {
        case 0: UI_Close(playerid);
        case 1:
        {
            if (opt == 3) { UI_Close(playerid); return SCM(playerid, C_GREY, "Дядя Вова: «Ну-ну. Захочешь поесть — приходи»."); }
            Dlg_Show(playerid, 2, "Зови меня Дядя Вова. Двадцать лет крутил гайки, пока жизнь не открутила меня. Хочешь выбраться отсюда — слушай старика. Сначала — еда. Вокруг вокзала полно пустых бутылок. Собери десяток и сдай в пункт приёма. Копейки, но честные.", "Хорошо, займусь", "Мне это не нужно");
        }
        case 2:
        {
            UI_Close(playerid);
            if (opt == 1) { Quest_Start(playerid, 1); SCM(playerid, C_YELLOW, "Дядя Вова: «Бутылки валяются тут, под эстакадой, и у вокзала Юнити. Подходи и нажимай ALT. Сдашь — у магазина 24/7 за вокзалом»."); }
        }
        case 10:
        {
            if (opt == 2) { UI_Close(playerid); Quest_SetPaused(playerid, 1, true); }
            else if (opt == 3) Dlg_Show(playerid, 0, "Обо мне? Потом, малой. Когда сам встанешь на ноги — расскажу всё. Обещаю.", "Ловлю на слове");
            else UI_Close(playerid);
        }
        case 20:
        {
            UI_Close(playerid);
            if (opt == 1) Quest_SetPaused(playerid, 1, false);
        }
        case 30:
        {
            if (opt == 4) return UI_Close(playerid);
            new path[8];
            if (opt == 1) path = "gov"; else if (opt == 2) path = "biz"; else path = "street";
            Quest_SetPath(playerid, 1, path);
            Inv_Add(playerid, Item_ByKey("letter"), 1, false);
            if (opt == 1) Dlg_Show(playerid, 0, "Госслужба — дело хорошее. В мэрии есть отдел кадров, скажешь, что от Вовы. Вот письмо — там телефоны нужных людей.", "Спасибо");
            else if (opt == 2) Dlg_Show(playerid, 0, "Своё дело... Как я когда-то. Начни с капитала: положи на счёт пять тысяч. Деньги любят тех, кто их бережёт. Держи письмо.", "Спасибо");
            else Dlg_Show(playerid, 0, "Улица... Сам выбрал — сам и отвечай. У каждой банды есть вербовщик у штаба. Только не забывай, откуда ты пришёл. Вот письмо.", "Спасибо");
            Quest_Event(playerid, "talk", "vova_path");
        }
        case 40:
        {
            Dlg_Show(playerid, 41, "Потому что месть — это тоже дно, только глубже. Я все эти годы собирал по винтику одну машину. Для того, кто выберется. Сюда приходили многие. Выбрался — ты.", "Это... для меня?");
        }
        case 41:
        {
            UI_Close(playerid);
            new Float:x, Float:y, Float:z, Float:a;
            Loc_Get("vova_garage", x, y, z, a);
            Veh_CreateOwned(playerid, 466, 1, 1, x + 4.0, y + 4.0, z + 0.5, a, 0);
            SCM(playerid, C_YELLOW, "Дядя Вова: «Glendale. Старая, но живая — как я. Береги её. И себя береги»." );
            Quest_Event(playerid, "talk", "vova_final");
        }
    }
    return 1;
}

// Звонок Дяде Вове с телефона (глава 6)
stock Vova_Call(playerid)
{
    if (Quest_GetStep(playerid, 1) == 12 && Quest_IsActive(playerid, 1))
    {
        SCM(playerid, C_YELLOW, "Дядя Вова (телефон): «Алло? О, у тебя теперь телефон! Совсем как человек. Слушай, дальше тебе нужны права».");
        SCM(playerid, C_YELLOW, "Дядя Вова (телефон): «Автошкола у Конференц-центра. Сдашь — позвони, отпразднуем шаурмой».");
        Quest_Event(playerid, "call_vova", "*");
    }
    else SCM(playerid, C_YELLOW, "Дядя Вова (телефон): «Да-да, слышу. Всё хорошо? Ну и ладно. Заходи к огоньку».");
    return 1;
}
