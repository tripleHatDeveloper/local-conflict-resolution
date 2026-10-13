from paths import P
from certify import *
rows=[]
for sigma in (1,2,3):
    for rho in (0.1,0.2):
        for seed in range(8):
            I=gen(16,rho,sigma,7000+seed); full=oracle_pairs(I)
            if not full: continue
            ss=build_sat(I,range(I.N))
            a=certified_revision(I,None,False,0,ss)      # pure global CEGAR, no locality
            b=certified_revision(I,2,True,0,ss)          # local discovery + certificate
            rows.append((sigma,len(full),a['sat_calls'],a['missed'],b['sat_calls'],b['missed'],b['k']))
print(len(rows),'instances')
print('sigma | n | global CEGAR (no locality): mean SAT calls, mean conflicts learned | local k0=2 + certificate: mean calls, mean conflicts learned by feedback, mean final k')
for s in (1,2,3):
    g=[r for r in rows if r[0]==s]
    print(f'{s:5} | {len(g):2} | {st.mean(r[2] for r in g):6.1f} calls, {st.mean(r[3] for r in g):5.1f} | {st.mean(r[4] for r in g):5.1f} calls, {st.mean(r[5] for r in g):4.1f}, k={st.mean(r[6] for r in g):.1f}')
print('overall mean final k:', round(st.mean(r[6] for r in rows), 2)); print('overall: global CEGAR',f'{st.mean(r[2] for r in rows):.1f}','calls vs local+certificate',f'{st.mean(r[4] for r in rows):.1f}','calls ; mean #MUS',f'{st.mean(r[1] for r in rows):.0f}')
