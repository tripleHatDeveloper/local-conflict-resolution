from paths import P
from synth import *
import time, math, networkx as nx, sys
def count_min_covers(edges, cap=2_000_000):
    """#minimal vertex covers of the conflict graph = #maximal independent sets = maximal cliques of the complement."""
    G=nx.Graph(); G.add_edges_from(edges); tot=1.0; capped=False; comps=[]
    for comp in nx.connected_components(G):
        H=G.subgraph(comp); Hc=nx.complement(H); c=0
        for _ in nx.find_cliques(Hc):
            c+=1
            if c>=cap: capped=True; break
        comps.append((len(comp),c)); tot+=0; tot*=c
    return tot, comps, capped
print('n  | #est | #MUS | comps (largest) | #global minimal repairs | global SAT-MARCO (MUS+MCS, 60s cap) | local+decomposition')
for n,rho,sigma in ((8,0.15,2),(12,0.15,2),(16,0.15,2),(24,0.15,2),(32,0.15,2)):
    I=gen(n,rho,sigma,42); full=oracle_pairs(I); edges=[tuple(sorted(m)) for m in full]
    tot,comps,capped=count_min_covers(edges)
    ncomp=len(comps); big=max(c[0] for c in comps) if comps else 0
    cnt=f'{tot:.3g}'+('+' if capped else '')
    t=time.time()
    if n<=24:
        S,sel=build_sat(I,range(I.N)); mus,mcs,ok=marco(S,sel,want_mcs=True,tmax=60); tg=time.time()-t
        g=f'{len(mcs)} repairs in {tg:.1f}s'+('' if ok else ' (TIMEOUT, incomplete)')
    else: g='skipped (build too large)'
    t=time.time(); found=set(); kk=0
    for R in range(0,16):
        for c in range(I.N): found|=oracle_pairs(I,ball(I,c,R).keys())
        if len(found)==len(full): kk=R; break
    tl=time.time()-t
    print(f'{n:2} | {len(I.est):4} | {len(full):4} | {ncomp} ({big}) | {cnt} | {g} | k*={kk}: {tl:.1f}s, repairs kept as product of {ncomp} lists')
