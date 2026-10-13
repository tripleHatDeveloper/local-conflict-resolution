from paths import P
from synth import *
import sys, random, statistics as st, collections, json
rng=random.Random(7)
def random_minimal_cover(edges):
    cover=set(x for e in edges for x in e); order=list(cover); rng.shuffle(order)
    for v in order:
        trial=cover-{v}
        if all(e[0] in trial or e[1] in trial for e in edges): cover=trial
    return cover
res=collections.defaultdict(list)   # (sigma,k) -> list of validity fractions
SIGMAS = tuple(int(x) for x in sys.argv[1].split(',')) if len(sys.argv) > 1 else (1, 2, 3, 4)
for sigma in SIGMAS:
    for rho in (0.1,0.2):
        for seed in range(6):
            I=gen(16,rho,sigma,5000+seed); full=oracle_pairs(I)
            if not full: continue
            found=set(); fe=[tuple(sorted(m)) for m in full]
            for R in range(0,12):
                for c in range(I.N): found |= oracle_pairs(I, ball(I,c,R).keys())
                if not found: continue
                ed=[tuple(sorted(m)) for m in found]; ok=0; T=20
                for _ in range(T):
                    C=random_minimal_cover(ed); ok+= all(a in C or b in C for a,b in fe)
                res[(sigma,R)].append(ok/T)
                if len(found)==len(full):
                    for RR in range(R+1,12): res[(sigma,RR)].append(1.0)
                    break
print('Fraction of locally computed minimal repairs that are valid (repair ALL true conflicts), by k')
print('k  | '+' | '.join(f'sigma={s}' for s in SIGMAS))
for k in range(0,11):
    cells=[]
    for s in SIGMAS:
        v=res.get((s,k)); cells.append(f'{st.mean(v):.2f} (n={len(v)})' if v else '  -  ')
    print(f'{k:2} | '+' | '.join(cells))
