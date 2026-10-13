from paths import P
from synth import *
import time, json, statistics as st, collections
rows=[]
SEEDS={16:10,32:5}
RMAX={16:32,32:16}
t0=time.time()
for n in (16,32):
    for sigma in (1,2,3,4):
        for rho in (0.05,0.1,0.2,0.4):
            for seed in range(SEEDS[n]):
                I=gen(n,rho,sigma,1000*n+seed)
                full=oracle_pairs(I)
                if not full: continue
                hist=[]; found=set()
                for R in range(0,RMAX[n]):
                    for c in range(I.N): found |= oracle_pairs(I, ball(I,c,R).keys())
                    hist.append(frozenset(found))
                    if len(found)==len(full): break
                kstar=len(hist)-1 if len(found)==len(full) else None
                if kstar is None: continue
                rec=[len(h)/len(full) for h in hist]
                rr={}
                for p in (1,2,3):
                    # stop at first k with hist[k]==hist[k+1]==...==hist[k+p]  (result stable for p increments)
                    stop=None
                    for k in range(len(hist)):
                        ok=True
                        for d in range(1,p+1):
                            if k+d>=len(hist):  # need to compute beyond k*: result already complete => identical afterwards
                                if len(hist[k])!=len(full): ok=False
                            elif hist[k+d]!=hist[k]: ok=False; break
                        if ok: stop=k; break
                    rr[p]=(stop, len(hist[stop])/len(full))
                rows.append(dict(n=n,sigma=sigma,rho=rho,seed=seed,est=len(I.est),mus=len(full),kstar=kstar,
                                 rec1=rec[min(1,kstar)],rec2=rec[min(2,kstar)],rec3=rec[min(3,kstar)],
                                 stops={p:rr[p] for p in rr}))
    json.dump(rows,open(P('exp1.json'),'w'))
    print(f'n={n} done, {time.time()-t0:.0f}s',flush=True)
json.dump(rows,open(P('exp1.json'),'w'))
print(len(rows),'instances with >=1 conflict')
