from paths import P
import sys, json, os, time, statistics as st
from structures import *
gaps = int(sys.argv[1]); n = 24; out = f'exp4_g{gaps}.json'
rows = json.load(open(P(out))) if os.path.exists(P(out)) else []
done = {r['seed'] for r in rows}
STRUCT = [('Global (one part)', lambda I, s: parts_global(I)),
          ('LS: Parikh finest splitting', lambda I, s: parts_LS(I)),
          ('B: tiles 6, margin 0 (partition)', lambda I, s: parts_B(I, 6, 0)),
          ('B: tiles 4, margin 2', lambda I, s: parts_B(I, 4, 2)),
          ('B: tiles 6, margin 3', lambda I, s: parts_B(I, 6, 3)),
          ('C: r=0 k=3', lambda I, s: parts_C(I, 0, 3, s)),
          ('C: r=0 k=5', lambda I, s: parts_C(I, 0, 5, s)),
          ('C: r=0 k=8', lambda I, s: parts_C(I, 0, 8, s)),
          ('C: r=2 k=5', lambda I, s: parts_C(I, 2, 5, s))]
t0 = time.time()
for seed in range(12):
    if seed in done: continue
    if time.time()-t0 > 220: break
    I = gen_walled(n, 0.15, 1, 4000+seed, 2, gaps); full = oracle_pairs(I)
    if not full: continue
    rng = random.Random(seed); row = dict(seed=seed, mus=len(full))
    for name, f in STRUCT:
        parts = f(I, seed); fnd = found_in(I, parts); mult, covers = kpartition(parts, I.N)
        sizes = [len(p) for p in parts]
        row[name] = dict(recall=len(fnd & full)/len(full), exact=fnd == full, valid=valid_fraction(fnd, full, rng),
                         maxpart=max(sizes), meanpart=sum(sizes)/len(sizes), nparts=len(parts), mult=mult, covers=covers)
    rows.append(row); json.dump(rows, open(P(out), 'w'))
print('gaps', gaps, 'rows', len(rows), f'{time.time()-t0:.0f}s')
