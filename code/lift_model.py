from paths import P
# Builds the lifted model (difference constraints) from the clause files.
import re, collections, json
idx = {}
for l in open(P('clauses-bloc1_zeta.corresp')):
    n, i = l.split(); m = re.match(r'(C\d+)([pm])_(\d+)$', n); idx[int(i)] = (m[1], m[2], int(m[3]))
cls = [list(map(int, l.split()[1:])) for l in open(P('clauses-bloc1_zeta')).read().split('\n')[1:] if l.strip()]
lab = {}
for l in open(P('clauses-bloc1_zeta.set')).read().split('\n')[1:]:
    t = l.split()
    if len(t) == 2: lab[int(t[0])] = t[1]
edges = {}
for l in open(P('voisinage1')):
    m = re.match(r'\((C\d+),(C\d+),(\d+)\);', l.strip())
    if m: edges[(m[1], m[2])] = int(m[3])
# 1. domains from all positive long clauses
dom = {}
for n, cl in enumerate(cls, 1):
    if len(cl) > 1 and all(x > 0 for x in cl):
        g = {(idx[x][0], idx[x][1]) for x in cl}
        if len(g) == 1: dom[g.pop()] = sorted(idx[x][2] for x in cl)
print('groups with a domain (parcel,kind):', len(dom), '| domain size range', min(map(len, dom.values())), max(map(len, dom.values())))
step = {b-a for d in dom.values() for a, b in zip(d, d[1:])}; print('value steps:', step)

import itertools, json
# forbidden pairs per ordered group pair (both orders)
F = collections.defaultdict(set)
for n, cl in enumerate(cls, 1):
    if len(cl) == 2 and all(x < 0 for x in cl):
        a, b = idx[-cl[0]], idx[-cl[1]]
        if (a[0], a[1]) == (b[0], b[1]): continue
        F[((a[0], a[1]), (b[0], b[1]))].add((a[2], b[2]))
# fit allowed = { lo <= x-y <= hi } for each UNORDERED group pair (keep one orientation)
rel = {}; bad = 0
for (X, Y), forb in F.items():
    dx, dy = dom[X], dom[Y]; allowed = {(a, b) for a in dx for b in dy} - forb
    diffs = sorted({a-b for a, b in allowed})
    lo, hi = (diffs[0], diffs[-1]) if diffs else (None, None)
    fit = {(a, b) for a in dx for b in dy if lo is not None and lo <= a-b <= hi} == allowed
    rel[(X, Y)] = (lo, hi, fit); bad += (not fit)
print('group pairs with constraints (one orientation):', len(rel), '| NOT expressible as lo<=x-y<=hi:', bad)

# --- final lifted model ---
# (1) adjacency: for each edge, difference constraints  x_X - x_Y >= lo  /  <= hi  (bounds not implied by domains)
adj_c = []; seen = set()
for (X, Y), (lo, hi, fit) in sorted(rel.items()):
    if X[0] == Y[0] or (Y, X) in seen: continue
    seen.add((X, Y)); dx, dy = dom[X], dom[Y]; mn, mx = min(dx)-max(dy), max(dx)-min(dy)
    assert fit
    adj_c.append((X, Y, None if lo <= mn else lo, None if hi >= mx else hi))
# (2) same-parcel: width >= 10, except the "dry" pair (m=p=ground) when the data allows it
parc = sorted({k[0] for k in dom}); width = {}; dry = {}
for p in parc:
    X, Y = (p, 'm'), (p, 'p'); allowed = {(a, b) for a in dom[X] for b in dom[Y]} - F[(X, Y)]
    band = {(a, b) for a in dom[X] for b in dom[Y] if b-a >= 10}; extra = allowed - band
    assert allowed >= band and len(extra) <= 1
    dry[p] = next(iter(extra))[0] if extra else None
print('parcels:', len(parc), '| with dry exception:', sum(v is not None for v in dry.values()), '| adjacency difference constraints:', len(adj_c))
# --- regenerate ALL binary nogoods and compare with the original clause set ---
orig = set()
for cl in cls:
    if len(cl) == 2 and all(x < 0 for x in cl):
        a, b = idx[-cl[0]], idx[-cl[1]]
        orig.add(frozenset(((a[0], a[1], a[2]), (b[0], b[1], b[2]))))
reg = set()
for g, vs in dom.items():                                   # at-most-one per (parcel, kind) group
    for a, b in itertools.combinations(vs, 2): reg.add(frozenset(((g[0], g[1], a), (g[0], g[1], b))))
for p in parc:
    for a in dom[(p, 'm')]:
        for b in dom[(p, 'p')]:
            if not (b-a >= 10 or (dry[p] is not None and a == b == dry[p])): reg.add(frozenset(((p, 'm', a), (p, 'p', b))))
for X, Y, lo, hi in adj_c:
    for a in dom[X]:
        for b in dom[Y]:
            d = a-b
            if (lo is not None and d < lo) or (hi is not None and d > hi): reg.add(frozenset(((X[0], X[1], a), (Y[0], Y[1], b))))
print(f'binary nogoods: original {len(orig)} | regenerated from lifted model {len(reg)} | IDENTICAL: {orig == reg}')
print('positive (domain) clauses:', len(dom), '| unit estimates:', 70, '| lifted facts: 46 parcels, 92 domains,', len(adj_c), 'adjacency constraints,', 70, 'estimates')
est = [idx[cls[n-1][0]] for n in sorted(k for k, v in lab.items() if v == 'E')]
json.dump(dict(parcels=parc, dom={f'{k[0]}|{k[1]}': v for k, v in dom.items()}, dry={p: v for p, v in dry.items()},
               adj=[dict(X=list(X), Y=list(Y), lo=lo, hi=hi) for X, Y, lo, hi in adj_c], est=[list(e) for e in est],
               edges=[[a, b, l] for (a, b), l in edges.items()]), open(P('lifted_model.json'), 'w'))

