from paths import P
import json, collections, statistics as st
rows=json.load(open(P('exp1_all.json')))
print(len(rows),'instances;', collections.Counter(r['n'] for r in rows))
def agg(key):
    g=collections.defaultdict(list)
    for r in rows: g[key(r)].append(r)
    return g
print('\n== by noise sigma (levels) : smallest complete k*, recall at fixed k, #MUS')
print('sigma | inst | #MUS med | k* median (min-max) | recall@k=1 | recall@k=2 | recall@k=3')
for s,g in sorted(agg(lambda r:r['sigma']).items()):
    ks=[r['kstar'] for r in g]
    print(f"{s:5} | {len(g):4} | {st.median([r['mus'] for r in g]):8.0f} | {st.median(ks):4.0f} ({min(ks)}-{max(ks)}) | {st.mean(r['rec1'] for r in g):.2f} | {st.mean(r['rec2'] for r in g):.2f} | {st.mean(r['rec3'] for r in g):.2f}")
print('\n== by estimate density rho')
for s,g in sorted(agg(lambda r:r['rho']).items()):
    ks=[r['kstar'] for r in g]
    print(f"rho={s:<5} inst={len(g):3} #MUS med={st.median([r['mus'] for r in g]):5.0f}  k* median={st.median(ks):.0f} ({min(ks)}-{max(ks)})  recall@k=1: {st.mean(r['rec1'] for r in g):.2f}  @2: {st.mean(r['rec2'] for r in g):.2f}")
print('\n== stability stopping rule: stop at first k whose result is unchanged for p further increments')
print('p | premature stops (incomplete result) | when premature: mean recall | when correct: mean overshoot (k_stop - k*)')
for p in ('1','2','3'):
    prem=[r for r in rows if r['stops'][p][1]<1]; ok=[r for r in rows if r['stops'][p][1]>=1]
    print(f"{p} | {len(prem)}/{len(rows)} = {100*len(prem)/len(rows):.1f}% | {st.mean(r['stops'][p][1] for r in prem) if prem else float('nan'):.3f} | {st.mean(r['stops'][p][0]-r['kstar'] for r in ok):.2f}")
print('\npremature-stop rate (p=1) by sigma:', {s:f"{100*sum(r['stops']['1'][1]<1 for r in g)/len(g):.0f}%" for s,g in sorted(agg(lambda r:r['sigma']).items())})
print('premature-stop rate (p=2) by sigma:', {s:f"{100*sum(r['stops']['2'][1]<1 for r in g)/len(g):.0f}%" for s,g in sorted(agg(lambda r:r['sigma']).items())})
