from paths import P
import time,sys,json
from mus import *
from pysat.solvers import Glucose4
def shrink(seed):
    cur=list(seed)
    # core refinement
    sat(cur); 
    c=set(rsel[l] for l in S.get_core()); cur=[n for n in cur if n in c]
    i=0
    while i<len(cur):
        trial=cur[:i]+cur[i+1:]
        if sat(trial):
            i+=1
        else:
            c=set(rsel[l] for l in S.get_core()); cur=[n for n in trial if n in c]; i=0 if False else min(i,len(cur))
    return sorted(cur)
def grow(seed):
    cur=list(seed)
    for n in E:
        if n in cur: continue
        if sat(cur+[n]): cur.append(n)
    return sorted(cur)
M=Glucose4(); X={n:k+1 for k,n in enumerate(E)}
for n in E: M.add_clause([X[n],-X[n]])
try: M.set_phases([X[n] for n in E])
except Exception as e: print('no phases',e)
muses=[];mcses=[]
t0=time.time(); it=0
while M.solve():
    m=M.get_model(); seed=[n for n in E if m[X[n]-1]>0]
    it+=1
    if sat(seed):
        mss=grow(seed); mcs=[n for n in E if n not in mss]
        mcses.append(mcs); M.add_clause([X[n] for n in mcs])
    else:
        mus=shrink(seed); muses.append(mus); M.add_clause([-X[n] for n in mus])
    if time.time()-t0>600: print('TIMEOUT'); break
else:
    print('complete enumeration')
print(f'{len(muses)} MUSes, {len(mcses)} MCSes, {it} iterations, {time.time()-t0:.1f}s')
json.dump({'muses':muses,'mcses':mcses},open(P('marco.json'),'w'))
