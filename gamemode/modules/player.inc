// =====================================================================
//  Данные игрока
// =====================================================================
enum E_PLAYER
{
    pID, bool:pLogged, bool:pCef, pLoginTries, pLoginTimer, pPass[65], pSalt[17],
    pAdmin, pSex, pSkin, pAge, pOrigin, pLevel, pExp, pCash, pBank,
    Float:pHunger, Float:pFatigue, pPhone[12], pOrg, pOrgRank, pFamily, pFamilyRank,
    pJob, pWanted, pJail, pMute, pPlayed, pLicDrive, pMedcard, pRentHouse, pFishSkill,
    pInjured, pCharDone, pOrgSkin, pWarns, pTheory, pAccSlot0, pAccSlot1, pSpawnAt,
    pEmail[64], pRefBy[MAX_PLAYER_NAME],
    // runtime
    pScreen[16], pCreateStep, pPreviewTimer, Float:pPreviewAng, pPreviewIdx,
    pAfkTicks, pMinuteTick, bool:pDuty, bool:pCuffed, pCuffedBy, bool:pOnJob, pJobStep, pJobCount, pJobVeh,
    pJobTimer, bool:pCarry, pCP, pRaceCP, pQueueNo, pQueueTimer, bool:pQueueCalled, pExamVeh, pExamStep,
    Float:pExamHealth, pCallTo, bool:pInCall, pFishTimer, pFishState, pTreasureTarget, pLastPM,
    pReportCD, pSpecTarget, pLastDamageBy, pInjuredTimer, pTaxiFare, pTaxiDriver, pFrisked, pPrevInt,
    pPrevVW, pHouseIn, pBizIn, pAntiSpam, pInvite, pInviteType, pFamilyInvite, pCasinoBet,
    pChatTick, pSaveTick, pShiftDone, pDetector
};
new P[MAX_PLAYERS][E_PLAYER];

stock P_Reset(playerid)
{
    new empty[E_PLAYER];
    P[playerid] = empty;
    P[playerid][pLoginTimer] = -1;
    P[playerid][pPreviewTimer] = -1;
    P[playerid][pJobTimer] = -1;
    P[playerid][pQueueTimer] = -1;
    P[playerid][pFishTimer] = -1;
    P[playerid][pInjuredTimer] = -1;
    P[playerid][pCallTo] = INVALID_PLAYER_ID;
    P[playerid][pCuffedBy] = INVALID_PLAYER_ID;
    P[playerid][pSpecTarget] = INVALID_PLAYER_ID;
    P[playerid][pTaxiDriver] = INVALID_PLAYER_ID;
    P[playerid][pInvite] = 0;
    P[playerid][pJobVeh] = INVALID_VEHICLE_ID;
    P[playerid][pExamVeh] = INVALID_VEHICLE_ID;
    P[playerid][pHouseIn] = 0;
    P[playerid][pBizIn] = 0;
    P[playerid][pHunger] = 100.0;
    P[playerid][pFatigue] = 100.0;
}

stock P_Load(playerid, DBResult:r)
{
    P[playerid][pID] = DB_GetInt(r, "id");
    DB_GetStr(r, "pass", P[playerid][pPass], 65);
    DB_GetStr(r, "salt", P[playerid][pSalt], 17);
    DB_GetStr(r, "email", P[playerid][pEmail], 64);
    DB_GetStr(r, "ref_by", P[playerid][pRefBy], MAX_PLAYER_NAME);
    DB_GetStr(r, "phone", P[playerid][pPhone], 12);
    P[playerid][pAdmin] = DB_GetInt(r, "admin");
    P[playerid][pSex] = DB_GetInt(r, "sex");
    P[playerid][pSkin] = DB_GetInt(r, "skin");
    P[playerid][pAge] = DB_GetInt(r, "age");
    P[playerid][pOrigin] = DB_GetInt(r, "origin");
    P[playerid][pLevel] = DB_GetInt(r, "level");
    P[playerid][pExp] = DB_GetInt(r, "exp");
    P[playerid][pCash] = DB_GetInt(r, "cash");
    P[playerid][pBank] = DB_GetInt(r, "bank");
    P[playerid][pHunger] = DB_GetFloat(r, "hunger");
    P[playerid][pFatigue] = DB_GetFloat(r, "fatigue");
    P[playerid][pOrg] = DB_GetInt(r, "org");
    P[playerid][pOrgRank] = DB_GetInt(r, "org_rank");
    P[playerid][pFamily] = DB_GetInt(r, "family");
    P[playerid][pFamilyRank] = DB_GetInt(r, "family_rank");
    P[playerid][pJob] = DB_GetInt(r, "job");
    P[playerid][pWanted] = DB_GetInt(r, "wanted");
    P[playerid][pJail] = DB_GetInt(r, "jail");
    P[playerid][pMute] = DB_GetInt(r, "mute");
    P[playerid][pPlayed] = DB_GetInt(r, "played");
    P[playerid][pLicDrive] = DB_GetInt(r, "lic_drive");
    P[playerid][pMedcard] = DB_GetInt(r, "medcard");
    P[playerid][pRentHouse] = DB_GetInt(r, "rent_house");
    P[playerid][pFishSkill] = DB_GetInt(r, "fish_skill");
    P[playerid][pInjured] = DB_GetInt(r, "injured");
    P[playerid][pCharDone] = DB_GetInt(r, "char_done");
    P[playerid][pOrgSkin] = DB_GetInt(r, "org_skin");
    P[playerid][pWarns] = DB_GetInt(r, "warns");
    P[playerid][pTheory] = DB_GetInt(r, "theory");
    P[playerid][pAccSlot0] = DB_GetInt(r, "acc_slot0");
    P[playerid][pAccSlot1] = DB_GetInt(r, "acc_slot1");
    P[playerid][pSpawnAt] = DB_GetInt(r, "spawn_at");
}

stock P_Save(playerid, bool:withPos = true)
{
    if (!P[playerid][pLogged] || !P[playerid][pID]) return;
    new q[1024], Float:hp, Float:ar;
    GetPlayerHealth(playerid, hp);
    GetPlayerArmour(playerid, ar);
    format(q, sizeof q, "UPDATE accounts SET admin=%d,sex=%d,skin=%d,age=%d,origin=%d,level=%d,exp=%d,cash=%d,bank=%d,hunger=%.1f,fatigue=%.1f,health=%.1f,armour=%.1f,phone='%s',org=%d,org_rank=%d,family=%d,family_rank=%d",
        P[playerid][pAdmin], P[playerid][pSex], P[playerid][pSkin], P[playerid][pAge], P[playerid][pOrigin],
        P[playerid][pLevel], P[playerid][pExp], P[playerid][pCash], P[playerid][pBank],
        P[playerid][pHunger], P[playerid][pFatigue], hp, ar, P[playerid][pPhone],
        P[playerid][pOrg], P[playerid][pOrgRank], P[playerid][pFamily], P[playerid][pFamilyRank]);
    format(q, sizeof q, "%s,job=%d,wanted=%d,jail=%d,mute=%d,played=%d,lic_drive=%d,medcard=%d,rent_house=%d,fish_skill=%d,injured=%d,char_done=%d,org_skin=%d,warns=%d,theory=%d,acc_slot0=%d,acc_slot1=%d,spawn_at=%d",
        q, P[playerid][pJob], P[playerid][pWanted], P[playerid][pJail], P[playerid][pMute], P[playerid][pPlayed],
        P[playerid][pLicDrive], P[playerid][pMedcard], P[playerid][pRentHouse], P[playerid][pFishSkill],
        P[playerid][pInjured], P[playerid][pCharDone], P[playerid][pOrgSkin], P[playerid][pWarns], P[playerid][pTheory],
        P[playerid][pAccSlot0], P[playerid][pAccSlot1], P[playerid][pSpawnAt]);
    if (withPos && GetPlayerState(playerid) != PLAYER_STATE_SPECTATING && GetPlayerState(playerid) != PLAYER_STATE_NONE)
    {
        new Float:x, Float:y, Float:z, Float:a;
        GetPlayerPos(playerid, x, y, z);
        GetPlayerFacingAngle(playerid, a);
        format(q, sizeof q, "%s,x=%.2f,y=%.2f,z=%.2f,a=%.1f,interior=%d,vw=%d", q, x, y, z, a, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid));
    }
    format(q, sizeof q, "%s WHERE id=%d", q, P[playerid][pID]);
    DB_Exec(q);
}

stock P_SetInt(playerid, const field[], value)
{
    new q[128];
    format(q, sizeof q, "UPDATE accounts SET %s=%d WHERE id=%d", field, value, P[playerid][pID]);
    DB_Exec(q);
}

stock bool:IsLogged(playerid) return IsPlayerConnected(playerid) && P[playerid][pLogged];

stock FindPlayerByAcc(accid)
{
    for (new i = 0, j = GetPlayerPoolSize(); i <= j; i++)
        if (IsPlayerConnected(i) && P[i][pLogged] && P[i][pID] == accid) return i;
    return INVALID_PLAYER_ID;
}

stock AccName(accid)
{
    new n[MAX_PLAYER_NAME], q[96];
    n = "—";
    format(q, sizeof q, "SELECT name FROM accounts WHERE id=%d", accid);
    new DBResult:r = DB_Query(q);
    if (r)
    {
        if (db_num_rows(r)) db_get_field(r, 0, n, sizeof n);
        db_free_result(r);
    }
    return n;
}
