#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Пешеходный граф для godjo_ai из путевых узлов GTA SA (NODES*.DAT -> nodes.json,
см. tools/routes/nodes.py). Берём пешеходные узлы Лос-Сантоса, схлопываем цепочки
(узлы степени 2 прореживаются до шага STEP м), оставляем крупнейшую связную компоненту.
Выход: gamemode/filterscripts/ai/navgraph.inc
Запуск: NODES_JSON=/data/work/nodes.json python3 tools/ai/gen_navgraph.py
"""
import os, json, math
from collections import deque
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
raw = json.load(open(os.environ.get("NODES_JSON", "/data/work/nodes.json")))
BBOX = (40.0, 2990.0, -2850.0, -760.0)   # Лос-Сантос
STEP = 10.0
N = {}
for k, v in raw.items():
    if v[3]: continue
    x, y, z = v[0], v[1], v[2]
    if BBOX[0] <= x <= BBOX[1] and BBOX[2] <= y <= BBOX[3] and -5 < z < 200: N[k] = (x, y, z + 1.0)
ADJ = {k: set() for k in N}
for k in N:
    for l in raw[k][5]:
        if l in N and l != k: ADJ[k].add(l); ADJ[l].add(k)
# крупнейшая компонента
comp, best = {}, None
for s in N:
    if s in comp: continue
    q, members = deque([s]), [s]; comp[s] = s
    while q:
        u = q.popleft()
        for v in ADJ[u]:
            if v not in comp: comp[v] = s; q.append(v); members.append(v)
    if best is None or len(members) > len(best): best = members
keep = set(best)
d = lambda a, b: math.hypot(N[a][0] - N[b][0], N[a][1] - N[b][1])
# схлопывание цепочек: узлы степени != 2 — опорные; на цепочках оставляем узлы каждые STEP м
anchor = {k for k in keep if len(ADJ[k]) != 2}
if not anchor: anchor = {next(iter(keep))}
kept = set(anchor); edges = set(); visited = set()
for a in anchor:
    for nb in ADJ[a]:
        if (a, nb) in visited: continue
        prev, cur, last, acc = a, nb, a, 0.0
        visited.add((a, nb))
        while True:
            acc += d(prev, cur)
            if cur in anchor:
                edges.add(tuple(sorted((last, cur)))); visited.add((cur, prev)); break
            if acc >= STEP:
                kept.add(cur); edges.add(tuple(sorted((last, cur)))); last, acc = cur, 0.0
            nxt = [v for v in ADJ[cur] if v != prev]
            if not nxt: kept.add(cur); edges.add(tuple(sorted((last, cur)))); break
            prev, cur = cur, nxt[0]
            if cur == a and last == a: break
edges = {e for e in edges if e[0] != e[1]}
order = sorted(kept, key=lambda k: (N[k][0], N[k][1])); idx = {k: i for i, k in enumerate(order)}
adj = [[] for _ in order]
for a, b in edges: adj[idx[a]].append(idx[b]); adj[idx[b]].append(idx[a])
out = ["// СГЕНЕРИРОВАНО tools/ai/gen_navgraph.py из путевых узлов GTA SA — не править руками.",
       "// Пешеходный граф Лос-Сантоса: %d узлов, %d рёбер (высота = уровень ног)." % (len(order), len(edges)),
       "#define NAV_NODES %d" % len(order), "#define NAV_EDGES %d" % (2 * len(edges)),
       "stock const Float:gNavPt[NAV_NODES][3] = {"]
out.append(",\n".join("{%.1f,%.1f,%.1f}" % N[k] for k in order)); out.append("};")
starts, flat = [], []
for i in range(len(order)): starts.append(len(flat)); flat += sorted(set(adj[i]))
starts.append(len(flat))
def wrap(vals, per=40):
    return ",\n".join(",".join(map(str, vals[i:i + per])) for i in range(0, len(vals), per))
out.append("stock const gNavStart[NAV_NODES + 1] = {\n%s\n};" % wrap(starts))
out.append("stock const gNavAdj[NAV_EDGES] = {\n%s\n};" % wrap(flat))
open(os.path.join(ROOT, "gamemode/filterscripts/ai/navgraph.inc"), "w", encoding="utf-8").write("\n".join(out) + "\n")
lens = [d(a, b) for a, b in edges]
print("пеш. узлов ЛС %d, компонента %d, в графе %d узлов, %d рёбер, ребро ср %.1f м, макс %.1f м" %
      (len(N), len(keep), len(order), len(edges), sum(lens) / len(lens), max(lens)))
