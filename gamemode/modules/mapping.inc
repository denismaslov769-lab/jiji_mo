// =====================================================================
//  Свой маппинг: «Площадь Godjo» у станции Unity (точка спавна)
//  Координаты можно подвинуть — все объекты в одном месте.
// =====================================================================
#define MAP_X 1765.0
#define MAP_Y -1915.0
#define MAP_Z 13.38

stock Map_Obj(model, Float:x, Float:y, Float:z, Float:rx = 0.0, Float:ry = 0.0, Float:rz = 0.0)
    return CreateObject(model, MAP_X + x, MAP_Y + y, MAP_Z + z, rx, ry, rz, 300.0);

// табличка с текстом (своя «текстура»: SetObjectMaterialText на пустом объекте 19482)
stock Map_Sign(Float:x, Float:y, Float:z, Float:rz, text[], size = 70, color = 0xFFFFC94D, bg = 0xCC0E0F14)
{
    new o = Map_Obj(19482, x, y, z, 0.0, 0.0, rz);
    SetObjectMaterialText(o, text, 0, OBJECT_MATERIAL_SIZE_512x256, "Arial", size, 1, color, bg, OBJECT_MATERIAL_TEXT_ALIGN_CENTER);
    return o;
}

stock Map_Build()
{
    // большой билборд над площадью
    Map_Sign(0.0, -14.0, 8.0, 0.0, "GODJO ROLE PLAY", 80);
    Map_Sign(0.0, -14.05, 6.0, 0.0, "Добро пожаловать в штат!", 46, 0xFFFFFFFF);
    // неоновая подсветка
    Map_Obj(18647, -6.0, -13.8, 5.2, 0.0, 90.0, 0.0);
    Map_Obj(18647, 6.0, -13.8, 5.2, 0.0, 90.0, 0.0);
    Map_Obj(18649, -6.0, -13.8, 9.6, 0.0, 90.0, 0.0);
    Map_Obj(18649, 6.0, -13.8, 9.6, 0.0, 90.0, 0.0);
    // фонари и лавочки по периметру
    for (new i = 0; i < 4; i++)
    {
        Map_Obj(1290, -10.0 + i * 6.6, -11.0, 0.0);
        Map_Obj(1280, -8.5 + i * 6.6, -10.0, 0.45, 0.0, 0.0, 180.0);
    }
    // пальмы
    Map_Obj(621, -12.0, -16.0, -0.4);
    Map_Obj(621, 12.0, -16.0, -0.4);
    // танцпол для мероприятий
    Map_Obj(19129, 0.0, -6.0, -0.35);
    // указатели
    Map_Sign(-14.0, 0.0, 2.6, 90.0, "Автосалон  >>  /gps", 40, 0xFFFFFFFF);
    Map_Sign(14.0, 0.0, 2.6, 270.0, "Работа и места: /gps  •  Помощь: /help", 34, 0xFFFFFFFF);
    Create3DTextLabel(H_GOLD "Площадь Godjo\n" H_GREY "/anims — анимации, /dance — танцы", C_WHITE, MAP_X, MAP_Y - 6.0, MAP_Z + 1.0, 25.0, 0, 1);
}
