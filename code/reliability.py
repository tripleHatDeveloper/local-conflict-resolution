from paths import P
from graded import *
import random
strat = strategies()
for f in (0.5, 2.0):
    strat[f'graded, contextual, sigma mis-estimated x{f}'] = (lambda f: (lambda I, j: weight(p_err(I.sig[j]*f))))(f)
per = collections.defaultdict(list)
for seed in range(300):
    I = gen_two_sources(16, 20000+seed); ev = evaluate(I, strat)
    if not ev: continue
    for k, v in ev.items(): per[k].append(v)
def f1(rows):
    r = sum(x[0] for x in rows); tp = sum(x[1] for x in rows); wd = sum(x[2] for x in rows)
    return 0 if tp == 0 else 2*(tp/r)*(tp/wd)/((tp/r)+(tp/wd))
n = len(per['min-cardinality (no reliabilities)']); print(n, 'instances')
for k, v in per.items(): print(f'{k:52} F1 = {f1(v):.3f}')
rng = random.Random(0); a = per['graded, contextual weights']
for base in ('fixed priority: trust expert', 'fixed priority: trust satellite', 'min-cardinality (no reliabilities)'):
    b = per[base]; diffs = []
    for _ in range(2000):
        idx = [rng.randrange(n) for _ in range(n)]
        diffs.append(f1([a[i] for i in idx]) - f1([b[i] for i in idx]))
    diffs.sort(); print(f'F1(contextual) - F1({base}): {f1(a)-f1(b):+.3f}  95% bootstrap CI [{diffs[50]:+.3f}, {diffs[1949]:+.3f}]')
