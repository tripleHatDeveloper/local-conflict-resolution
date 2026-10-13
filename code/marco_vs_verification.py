from paths import P
from certify import *
rows=[]
for sigma in (1,2,3):
    for seed in range(2):
        I=gen(16,0.2,sigma,7000+seed); full=oracle_pairs(I)
        if not full: continue
        S,sel=build_sat(I,range(I.N)); C=Counted(S)
        t=time.time(); mus,_,ok=marco(C,sel,want_mcs=False,tmax=20); tg=time.time()-t
        ss=build_sat(I,range(I.N)); t=time.time(); r=certified_revision(I,2,True,0,ss); tc=time.time()-t
        rows.append((sigma,len(full),C.calls,ok,tg,r['sat_calls'],tc))
print('sigma | #MUS | global MARCO(MUS only): SAT calls, time, complete | certified loop (k0=2, k grows): SAT calls, time')
for r in rows: print(f'{r[0]:5} | {r[1]:4} | {r[2]:6} calls {r[4]:5.1f}s {"complete" if r[3] else "TIMEOUT"} | {r[5]:3} calls {r[6]:.2f}s')
print('median calls ratio global/certified:', st.median(r[2]/r[5] for r in rows))
