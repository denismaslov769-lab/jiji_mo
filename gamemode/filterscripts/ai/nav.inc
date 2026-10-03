// =====================================================================
//  godjo_ai — навигация: ближайший узел, A* по пешеходному графу
//  (граф — navgraph.inc, сгенерирован из путевых узлов GTA SA)
// =====================================================================
static Float:sG[NAV_NODES], sFrom[NAV_NODES], sStamp[NAV_NODES], sClosed[NAV_NODES], sHeap[NAV_NODES], Float:sHeapF[NAV_NODES], sHeapN, sRun;

stock Nav_Nearest(Float:x, Float:y, Float:maxd = 200.0)
{
    new best = -1, Float:bd = maxd;
    for (new i = 0; i < NAV_NODES; i++)
    {
        new Float:dx = gNavPt[i][0] - x;
        if (dx > bd || dx < -bd) continue;
        new Float:dy = gNavPt[i][1] - y;
        if (dy > bd || dy < -bd) continue;
        new Float:d = floatsqroot(dx * dx + dy * dy);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

// случайный узел в радиусе [rmin, rmax] от узла
stock Nav_RandomNear(node, Float:rmin, Float:rmax)
{
    if (node < 0 || node >= NAV_NODES) return 0;
    new pick = -1, seen = 0, Float:x = gNavPt[node][0], Float:y = gNavPt[node][1];
    // узлы отсортированы по X — сразу отсекаем по полосе, внутри неё равновероятный выбор
    for (new i = 0; i < NAV_NODES; i++)
    {
        new Float:dx = gNavPt[i][0] - x;
        if (dx < -rmax || dx > rmax) continue;
        new Float:d = AI_Dist2(x, y, gNavPt[i][0], gNavPt[i][1]);
        if (d < rmin || d > rmax) continue;
        seen++;
        if (random(seen) == 0) pick = i;
    }
    return pick == -1 ? node : pick;
}

static stock Heap_Push(n, Float:f)
{
    new i = sHeapN++;
    while (i > 0)
    {
        new p = (i - 1) / 2;
        if (sHeapF[p] <= f) break;
        sHeap[i] = sHeap[p]; sHeapF[i] = sHeapF[p]; i = p;
    }
    sHeap[i] = n; sHeapF[i] = f;
}

static stock Heap_Pop()
{
    new top = sHeap[0];
    sHeapN--;
    if (sHeapN > 0)
    {
        new n = sHeap[sHeapN], Float:f = sHeapF[sHeapN], i = 0;
        for (;;)
        {
            new c = i * 2 + 1;
            if (c >= sHeapN) break;
            if (c + 1 < sHeapN && sHeapF[c + 1] < sHeapF[c]) c++;
            if (sHeapF[c] >= f) break;
            sHeap[i] = sHeap[c]; sHeapF[i] = sHeapF[c]; i = c;
        }
        sHeap[i] = n; sHeapF[i] = f;
    }
    return top;
}

// A*: пишет путь (без стартового узла) в out[], возвращает длину или -1.
// Длинные пути обрезаются до max узлов — агент перепланирует по прибытии.
stock Nav_Path(from, to, out[], max = sizeof out, maxExpand = NAV_NODES)
{
    if (from < 0 || to < 0) return -1;
    if (from == to) return 0;
    sRun++;
    sHeapN = 0;
    sG[from] = 0.0; sFrom[from] = -1; sStamp[from] = sRun; sClosed[from] = 0;
    Heap_Push(from, 0.0);
    new Float:tx = gNavPt[to][0], Float:ty = gNavPt[to][1], expanded = 0, bool:found = false;
    while (sHeapN > 0 && expanded < maxExpand)
    {
        new u = Heap_Pop();
        if (sClosed[u] == sRun) continue;
        sClosed[u] = sRun;
        if (u == to) { found = true; break; }
        expanded++;
        for (new e = gNavStart[u], ee = gNavStart[u + 1]; e < ee; e++)
        {
            new v = gNavAdj[e];
            if (sClosed[v] == sRun) continue;
            new Float:g = sG[u] + AI_Dist2(gNavPt[u][0], gNavPt[u][1], gNavPt[v][0], gNavPt[v][1]);
            if (sStamp[v] == sRun && g >= sG[v]) continue;
            sStamp[v] = sRun; sG[v] = g; sFrom[v] = u;
            if (sHeapN < NAV_NODES) Heap_Push(v, g + AI_Dist2(gNavPt[v][0], gNavPt[v][1], tx, ty));
        }
    }
    if (!found) return -1;
    new len = 0;
    for (new c = to; c != from && c != -1; c = sFrom[c]) len++;
    new skip = len > max ? len - max : 0, n = len - skip, k = len - 1;
    // разворачиваем: идём от цели к старту, кладём с конца; «лишний» хвост у цели отбрасываем
    for (new c = to; c != from && c != -1; c = sFrom[c])
    {
        if (k < n) out[k] = c;
        k--;
    }
    return n;
}
