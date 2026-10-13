from paths import P
import sys, json, random, statistics as st, collections
from structures import *
n = 24; res = collections.defaultdict(lambda: collections.defaultdict(list))
def tiles_containing(I, s, m, i):
    return [p for p in parts_B(I, s, m) if i in p]
for gaps in (0, 1):
    for seed in range(12):
        I = gen_walled(n, 0.15, 1, 4000+seed, 2, gaps); full = oracle_pairs(I)
        if not full: continue
        rng = random.Random(seed); inv = [j for j in range(len(I.est)) if any(j in m for m in full)]
        sample = rng.sample(inv, min(25, len(inv))); comps = components(I)
        Bs = {'B: tiles 4, margin 2': parts_B(I, 4, 2), 'B: tiles 6, margin 3': parts_B(I, 6, 3)}
        C2 = parts_C(I, 2, 5, seed)
        for j in sample:
            i = I.est[j][1]; true_j = {m for m in full if j in m}
            cfgs = {'Global': [frozenset(range(I.N))],
                    'LS (component of the new info)': [c for c in comps if i in c],
                    'C r=0 k=3': [multi_ball(I, [i], 3)], 'C r=0 k=5': [multi_ball(I, [i], 5)], 'C r=0 k=8': [multi_ball(I, [i], 8)],
                    'C r=2 k=5 (cover of the core containing it)': [p for p in C2 if i in p][:1],
                    'B: tiles 4, margin 2': [p for p in Bs['B: tiles 4, margin 2'] if i in p],
                    'B: tiles 6, margin 3': [p for p in Bs['B: tiles 6, margin 3'] if i in p]}
            for nm, ps in cfgs.items():
                fnd = {m for p in ps for m in oracle_pairs(I, p) if j in m}
                res[gaps][nm].append((len(fnd & true_j)/len(true_j), len(frozenset().union(*ps)) if ps else 0))
for gaps, lab in ((0, '3 separate basins'), (1, 'ONE pass between basins')):
    print(f'\n=== new estimate arrives; regime: {lab}')
    print(f'{"structure (part revised)":46} | parcels touched | conflicts of the new estimate found | fully complete')
    for nm, v in res[gaps].items():
        print(f'{nm:46} | {st.mean(x[1] for x in v):15.0f} | {st.mean(x[0] for x in v):35.3f} | {100*sum(x[0]==1 for x in v)/len(v):5.0f}%')
