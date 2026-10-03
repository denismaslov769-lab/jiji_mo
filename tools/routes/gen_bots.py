import json, math, random, sys
sys.argv=['x']
exec(open('/data/work/gen.py').read().split("out=open('/data/work/gen_routes.inc','w')")[0])
random.seed(11)
def walk(cx,cy,R,n=36):
    starts=[k for k in PED if d2(N[k],(cx,cy))<R and len([l for l in ADJ[k] if l in PED])>=2 and -50<N[k][2]<400]
    best=None
    for t in range(60):
        cur=random.choice(starts); path=[cur]; prev=None
        while len(path)<n:
            nx=[l for l in ADJ[cur] if l in PED and l!=prev and d2(N[l],(cx,cy))<R*1.4]
            if not nx: break
            prev=cur; cur=random.choice(nx)
            if cur in path[-6:]: break
            path.append(cur)
        if best is None or len(path)>len(best): best=path
        if len(best)>=n: break
    # прореживание: точки не ближе 5 м
    out=[N[best[0]]]
    for k in best[1:]:
        if d2(N[k],out[-1])>=5: out.append(N[k])
    return out
zones=[('ls',1760,-1900,140),('ls',1480,-1700,160),('ls',2450,-1680,120),('ls',1250,-1150,200),('ls',800,-1700,150),('ls',2100,-1750,150),('ls',1000,-1400,160),('ls',350,-1800,150),('ls',1850,-1450,150),('ls',2700,-1300,150),
       ('ls',690,-600,100),('ls',250,-150,100),('ls',1330,320,100),
       ('sf',-1980,140,150),('sf',-2200,700,150),('sf',-2700,380,150),('sf',-1800,900,150),
       ('lv',2050,1300,200),('lv',2200,1700,150),('lv',2820,1290,150),('lv',2000,2100,150),('lv',1650,1500,120)]
W=[]
for c,x,y,R in zones:
    for rep in range(2):
        p=walk(x,y,R)
        if len(p)>=8: W.append(p)
print('walk',len(W),[len(p) for p in W])
def lane(pts,off=2.6,loop=True):
    res=[]
    n=len(pts)
    for i in range(n):
        a=pts[i-1] if (i>0 or loop) else pts[i]; b=pts[(i+1)%n] if (i<n-1 or loop) else pts[i]
        dx,dy=b[0]-a[0],b[1]-a[1]; L=math.hypot(dx,dy) or 1
        res.append((pts[i][0]+dy/L*off,pts[i][1]-dx/L*off,pts[i][2]))
    return res
drives=[[(1797,-1906),(1955,-1758),(2210,-1650),(2108,-1380),(1868,-1374),(1560,-1415)],
 [(1427,-1580),(1530,-1730),(1690,-1735),(1820,-1830),(1430,-1870)],
 [(1060,-1712),(1180,-1850),(1390,-1735),(1429,-1600),(1200,-1572)],
 [(1300,-1150),(1600,-1150),(1860,-1180),(1600,-1300)],
 [(2210,-1650),(2440,-1660),(2640,-1660),(2340,-1380)],
 [(830,-1800),(1060,-1712),(800,-1400),(500,-1400)],
 [(-1980,140),(-2000,600),(-2150,700),(-2200,300)],
 [(-2700,380),(-2400,500),(-2700,700)],
 [(2050,1300),(2050,1800),(2200,1700),(2200,1300)],
 [(2820,1290),(2500,1250),(2300,1100),(2600,1050)]]
D=[]
for wps in drives:
    try:
        pts,_=route(wps,step=22)
        D.append(lane(pts))
    except Exception as e: print('skip',e)
print('drive',len(D),[len(p) for p in D])
o=open('/data/jiji_mo/gamemode/filterscripts/bots_paths.inc','w')
o.write('// Сгенерировано tools/gen-routes.py (bots): маршруты пешеходов по тротуарам и машин по полосам дорог\n')
flat=[];idx=[]
for p in W: idx.append((len(flat),len(p))); flat+=p
o.write('stock const Float:gWalkPt[][3] = {\n    '+',\n    '.join('{%.1f,%.1f,%.1f}'%(x,y,z+1.0) for x,y,z in flat)+'\n};\n')
o.write('stock const gWalkPath[][2] = {'+','.join('{%d,%d}'%a for a in idx)+'};\n')
city=[]
for (c,x,y,R) in zones:
    pass
# город каждого маршрута
wc=[]
k=0
for c,x,y,R in zones:
    for rep in range(2):
        pass
flat=[];idx=[]
for p in D: idx.append((len(flat),len(p))); flat+=p
o.write('stock const Float:gDrivePt[][3] = {\n    '+',\n    '.join('{%.1f,%.1f,%.1f}'%(x,y,z+1.0) for x,y,z in flat)+'\n};\n')
o.write('stock const gDrivePath[][2] = {'+','.join('{%d,%d}'%a for a in idx)+'};\n')
o.close()
