from paths import P
import sys, json, time, os
from synth import *
n=int(sys.argv[1]); sigma=int(sys.argv[2]); out=f'exp1_n{n}.json'
rows=json.load(open(P(out))) if os.path.exists(P(out)) else []
done={(r['sigma'],r['rho'],r['seed']) for r in rows}
RMAX=16; t0=time.time()
for rho in (0.05,0.1,0.2,0.4):
    for seed in range(5):
        if (sigma,rho,seed) in done: continue
        if time.time()-t0>200: break
        I=gen(n,rho,sigma,1000*n+seed); full=oracle_pairs(I)
        if not full: continue
        hist=[]; found=set()
        for R in range(RMAX):
            for c in range(I.N): found |= oracle_pairs(I, ball(I,c,R).keys())
            hist.append(frozenset(found))
            if len(found)==len(full): break
        if len(found)!=len(full): rows.append(dict(n=n,sigma=sigma,rho=rho,seed=seed,incomplete=True,mus=len(full))); continue
        kstar=len(hist)-1; rec=[len(h)/len(full) for h in hist]; rr={}
        for p in (1,2,3):
            stop=None
            for k in range(len(hist)):
                ok=True
                for d in range(1,p+1):
                    if k+d>=len(hist):
                        if len(hist[k])!=len(full): ok=False
                    elif hist[k+d]!=hist[k]: ok=False; break
                if ok: stop=k; break
            rr[p]=(stop,len(hist[stop])/len(full))
        rows.append(dict(n=n,sigma=sigma,rho=rho,seed=seed,est=len(I.est),mus=len(full),kstar=kstar,
                         rec1=rec[min(1,kstar)],rec2=rec[min(2,kstar)],rec3=rec[min(3,kstar)],stops={p:rr[p] for p in rr}))
        json.dump(rows,open(P(out),'w'))
json.dump(rows,open(P(out),'w')); print('sigma',sigma,'rows now',len(rows),f'{time.time()-t0:.0f}s')
