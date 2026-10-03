import json, math, heapq, random
random.seed(7)
raw=json.load(open('/data/work/nodes.json'))
N={k:(v[0],v[1],v[2]) for k,v in raw.items()}
VEH={k for k,v in raw.items() if v[3]}
PED={k for k,v in raw.items() if not v[3]}
ADJ={k:[l for l in v[5] if l in raw] for k,v in raw.items()}
def d2(a,b): return math.hypot(a[0]-b[0],a[1]-b[1])
# grid index
G={}
for k,(x,y,z) in N.items(): G.setdefault((int(x//50),int(y//50)),[]).append(k)
def nearest(x,y,kind,maxd=400,filt=None):
    best=None;bd=1e9
    r=0
    while r*50<=maxd:
        for gx in range(int(x//50)-r,int(x//50)+r+1):
            for gy in range(int(y//50)-r,int(y//50)+r+1):
                if max(abs(gx-int(x//50)),abs(gy-int(y//50)))!=r: continue
                for k in G.get((gx,gy),[]):
                    if (k in VEH)!=(kind=='veh'): continue
                    if filt and not filt(k): continue
                    dd=math.hypot(N[k][0]-x,N[k][1]-y)
                    if dd<bd: bd=dd;best=k
        if best and bd<r*50: break
        r+=1
    return best
def dijkstra(a,b):
    dist={a:0};prev={};pq=[(0,a)]
    while pq:
        d,u=heapq.heappop(pq)
        if u==b: break
        if d>dist[u]: continue
        for v in ADJ[u]:
            if v not in VEH: continue
            nd=d+d2(N[u],N[v])
            if nd<dist.get(v,1e18): dist[v]=nd;prev[v]=u;heapq.heappush(pq,(nd,v))
    if b not in dist: return None
    p=[b]
    while p[-1]!=a: p.append(prev[p[-1]])
    return p[::-1]
def route(wps,step=70,loop=True):
    ks=[nearest(x,y,'veh') for x,y in wps]
    path=[]
    seq=ks+([ks[0]] if loop else [])
    for i in range(len(seq)-1):
        p=dijkstra(seq[i],seq[i+1])
        if not p: raise Exception('no path %s->%s'%(wps[i],wps[(i+1)%len(wps)]))
        path+= p if not path else p[1:]
    # subsample
    out=[N[path[0]]];acc=0
    for i in range(1,len(path)):
        acc+=d2(N[path[i-1]],N[path[i]])
        if acc>=step: out.append(N[path[i]]);acc=0
    if d2(out[-1],N[path[-1]])>15: out.append(N[path[-1]])
    return out,path
def fmt(pts,name):
    s='stock const Float:%s[][3] = {\n    '%name
    s+=',\n    '.join('{%.1f,%.1f,%.1f}'%(x,y,z+1.0) for x,y,z in pts)
    return s+'\n};\n'
out=open('/data/work/gen_routes.inc','w')
out.write('// Сгенерировано tools/gen-routes.py из путевых узлов GTA SA (NODES*.DAT): все точки лежат на дорогах\n')
# exam: around Verona/Marina
ex,_=route([(1067,-1712),(1180,-1850),(1390,-1735),(1429,-1600),(1200,-1572)],step=90)
print('exam',len(ex)); out.write(fmt(ex,'gExamRoute'))
# LS city bus (from Unity depot)
bus,bpath=route([(1797,-1906),(1955,-1758),(2210,-1650),(2108,-1380),(1868,-1374),(1560,-1415),(1427,-1580),(1530,-1730),(1690,-1735)],step=60)
def ang(dx,dy): return (math.degrees(math.atan2(-dx,dy)))%360   # SA: 0 = север (+y), против часовой
# остановки на маршруте: узел дороги + тротуар рядом
def stops_on(path,names,mind=380):
    res=[];last=None;acc=0
    for i,k in enumerate(path):
        if last is not None: acc+=d2(N[path[i-1]],N[k])
        if last is not None and acc<mind: continue
        x,y,z=N[k]
        p=nearest(x,y,'ped',maxd=30,filt=lambda q: 3.5<=d2(N[q],(x,y))<=11 and abs(N[q][2]-z)<2.5)
        if not p: continue
        res.append((k,p)); last=k; acc=0
        if len(res)>=len(names): break
    return res
def write_route_with_stops(name,wps,stopnames,step=60):
    pts,path=route(wps,step=step)
    st=stops_on(path,stopnames)
    stopset={s[0]:j for j,s in enumerate(st)}
    # маршрут = точки + остановки, в порядке пути
    seq=[];acc=0;prev=None
    for i,k in enumerate(path):
        if prev is not None: acc+=d2(N[prev],N[k])
        if k in stopset: seq.append((k,stopset[k]));acc=0
        elif acc>=step or i==0: seq.append((k,-1));acc=0
        prev=k
    if seq[-1][0]!=path[-1]: seq.append((path[-1],-1))
    lines=[]
    for k,sj in seq:
        x,y,z=N[k]
        lines.append('{%.1f,%.1f,%.1f,%d.0}'%(x,y,z+1.0,sj))
    out.write('stock const Float:%s[][4] = {\n    '%name+',\n    '.join(lines)+'\n};\n')
    sl=[]
    for j,(k,p) in enumerate(st):
        px,py,pz=N[p]; rx,ry,rz=N[k]
        sl.append('{%.1f,%.1f,%.1f,%.1f}'%(px,py,pz+1.0,ang(rx-px,ry-py)))
    out.write('stock const Float:%sStops[][4] = {\n    '%name+',\n    '.join(sl)+'\n};\n')
    out.write('stock const %sStopName[][] = {%s};\n'%(name,','.join('"%s"'%n for n in stopnames[:len(st)])))
    print(name,len(seq),'stops',len(st))
write_route_with_stops('gBusCity',[(1797,-1906),(1955,-1758),(2210,-1650),(2108,-1380),(1868,-1374),(1560,-1415),(1427,-1580),(1530,-1730),(1690,-1735)],
  ['Айдлвуд','Гантон','Джефферсон','Глен-парк','Даунтаун','Маркет','Першинг-сквер','Коммерс','Литл-Мексико','Эль-Корона','Юнити'])
write_route_with_stops('gBusSuburb',[(1797,-1906),(1620,-1150),(1000,-940),(660,-560),(240,-160),(1300,260),(2330,40),(2250,-620),(2200,-1100)],
  ['Малхолланд','Темпл','Диллимор','Блуберри','Монтгомери','Паломино-Крик','Фланк','Лас-Колинас','Ист-Лос-Сантос','Айдлвуд'],step=90)
# такси: тротуары у дорог по ЛС
cand=[k for k in PED if 150<N[k][0]<2900 and -2750<N[k][1]<-900 and 0<N[k][2]<90]
random.shuffle(cand)
taxi=[]
for p in cand:
    x,y,z=N[p]
    if any(d2(N[p],N[q[0]])<190 for q in taxi): continue
    v=nearest(x,y,'veh',maxd=20,filt=lambda q: 4<=d2(N[q],(x,y))<=10 and abs(N[q][2]-z)<2)
    if not v: continue
    taxi.append((p,v))
    if len(taxi)>=64: break
lines=[]
for p,v in taxi:
    px,py,pz=N[p]; vx,vy,vz=N[v]
    lines.append('{%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f}'%(px,py,pz+1.0,ang(vx-px,vy-py),vx,vy,vz+1.0))
out.write('// такси/курьер/мусоровоз: {тротуар x,y,z, угол к дороге, дорога x,y,z}\nstock const Float:gStreetSpots[][7] = {\n    '+',\n    '.join(lines)+'\n};\n')
print('taxi',len(taxi))
# грузоперевозки: точки на дорогах
td=[(247.0,-245.0),(660.0,-560.0),(1374.0,272.0),(2290.0,30.0),(-90.0,-1180.0),(-1980,180),(2830,1300),(-2140,-2400),(-150,1100),(1650,2200)]
lines=[]
for x,y in td:
    k=nearest(x,y,'veh'); X,Y,Z=N[k]; lines.append('{%.1f,%.1f,%.1f}'%(X,Y,Z+1.0))
out.write('stock const Float:gTruckDest[][3] = {\n    '+',\n    '.join(lines)+'\n};\n')
static=['Блуберри','Диллимор','Монтгомери','Паломино-Крик','Флинт','Сан-Фиерро','Лас-Вентурас','Энджел-Пайн','Форт-Карсон','Лас-Вентурас (север)']
out.write('stock const gTruckDestName[][] = {%s};\n'%','.join('"%s"'%n for n in static))
out.close()
# --- локации квестов/NPC ---
L=[
('q_ashot2',1690,-1890),('q_zina',2440,-1660),('q_cat1',2380,-1720),('q_cat2',2290,-1640),('q_cat3',2230,-1720),
('q_taxidisp',1783,-1932),('q_busdisp',1766,-1890),('q_semyon',830,-2055),('q_og',2510,-1670),('q_unity_ticket',1745,-1945),
('q_ls_beach',370,-1800),('q_vinewood',1400,-840),('q_ls_airport',1960,-2290),('q_market',820,-1350),('q_glen',1970,-1200),
('q_shaw1',1480,-1745),('q_shaw2',2050,-1700),('q_shaw3',1180,-1330),
('q_blueberry',230,-150),('q_farm1',-60,30),('q_farm2',-120,60),('q_farm3',-20,100),('q_dillimore_mkt',660,-600),
('q_bobby',720,-460),('q_montgomery',1380,460),('q_palomino',2330,40),('q_angelpine',-2150,-2400),('q_chiliad',-2318,-1636),
('q_fortcarson',-150,1100),('q_verdant',400,2500),('q_vm_part1',360,2470),('q_vm_part2',420,2550),('q_vm_part3',300,2540),
('q_bayside',-2500,2300),('q_elq',-1500,2600),
('sf_station',-1975,120),('q_sf_garage',-2030,160),('q_sf_china',-2200,700),('q_sf_pier',-1650,1300),('q_sf_cityhall',-2700,380),
('q_sf_gant',-2680,1300),('q_sf_fin',-1800,900),('q_sf_hosp',-2660,630),('q_sf_china2',-2150,650),
('lv_station',2848,1290),('q_lv_dragons',2025,1010),('q_lv_calig',2180,1680),('q_lv_strip',2040,1300),('q_lv_airport',1680,1450),
('q_lv_police',2290,2430),('q_lv_light1',2100,1500),('q_lv_light2',2220,1880),('q_lv_light3',1950,2120),('q_lv_redsands',1800,850),('q_lv_mansion',2550,700),
('ls_station',1755,-1945),
]
sql=[]
for code,x,y in L:
    p=nearest(x,y,'ped',maxd=250,filt=lambda q: -50<N[q][2]<600)
    kind='ped'
    if not p or d2(N[p],(x,y))>120:
        p=nearest(x,y,'veh',maxd=400); kind='veh'
    X,Y,Z=N[p]
    print('%-16s %-4s snap %.0fm -> %.1f %.1f %.1f'%(code,kind,d2(N[p],(x,y)),X,Y,Z))
    sql.append((code,X,Y,Z+1.0))
json.dump(sql,open('/data/work/locs.json','w'))
# --- точки для пеших работ ---
def spots_near(cx,cy,R,kind,cnt,mind,zr=(-100,700)):
    c=[k for k in (PED if kind=='ped' else VEH) if d2(N[k],(cx,cy))<R and zr[0]<N[k][2]<zr[1]]
    random.shuffle(c); res=[]
    for k in c:
        if any(d2(N[k],N[q])<mind for q in res): continue
        res.append(k)
        if len(res)>=cnt: break
    return res
o=open('/data/work/gen_routes.inc','a')
for name,(cx,cy,R,kind,cnt,mind) in {'gJanitorSpots':(1479,-1650,140,'ped',14,18),'gMinerSpots':(588,872,160,'veh',12,15),
    'gFarmSpots':(-60,50,170,'veh',12,20),'gElecSpots':(2040,1300,650,'ped',18,60)}.items():
    r=spots_near(cx,cy,R,kind,cnt,mind)
    print(name,len(r))
    o.write('stock const Float:%s[][3] = {\n    '%name+',\n    '.join('{%.1f,%.1f,%.1f}'%(N[k][0],N[k][1],N[k][2]+1.0) for k in r)+'\n};\n')
o.close()
