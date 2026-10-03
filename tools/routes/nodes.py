import struct, glob, os, math, json
D='/tmp/fcnpc/FCNPC-2.0.11/scriptfiles/FCNPC/nodes'
veh=[]; ped=[]
links={}
allnodes={}
for i in range(64):
    b=open(f'{D}/NODES{i}.DAT','rb').read()
    n,nv,np_,nn,nl=struct.unpack_from('<5I',b,0)
    off=20
    nodes=[]
    for k in range(n):
        mem,zero,x,y,z,heur,lid,area,nid,width,flood,flags=struct.unpack_from('<IIhhhhHHHBBI',b,off); off+=28
        nodes.append((x/8,y/8,z/8,lid,flags&0xF,area,nid,width/8,flags))
    off+= nn*14
    lk=[]
    for k in range(nl):
        a,nidd=struct.unpack_from('<HH',b,off); off+=4; lk.append((a,nidd))
    for k,(x,y,z,lid,cnt,area,nid,w,fl) in enumerate(nodes):
        key=(i,k)
        allnodes[key]=dict(x=x,y=y,z=z,veh=k<nv,flags=fl)
        links[key]=[lk[lid+j] for j in range(cnt) if lid+j<len(lk)]
json.dump({'%d_%d'%k:[v['x'],v['y'],v['z'],1 if v['veh'] else 0,v['flags'],['%d_%d'%l for l in links[k]]] for k,v in allnodes.items()},open('/data/work/nodes.json','w'))
print(len(allnodes), sum(1 for v in allnodes.values() if v['veh']))
# sanity: nodes near autoschool
near=sorted(allnodes.items(),key=lambda kv:(kv[1]['x']-1072)**2+(kv[1]['y']+1713)**2)[:6]
for k,v in near: print(k,v,links[k][:4])
