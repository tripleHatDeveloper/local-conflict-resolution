from paths import P
import re, json, collections, time
from pysat.solvers import Cadical153
idx = {}; par = {}
for l in open(P('clauses-bloc1_zeta.corresp')):
    n, i = l.split(); idx[int(i)] = n; par[int(i)] = re.match(r'(C\d+)', n)[1]
cls = [list(map(int, l.split()[1:])) for l in open(P('clauses-bloc1_zeta')).read().split('\n')[1:] if l.strip()]
lab = {}
for l in open(P('clauses-bloc1_zeta.set')).read().split('\n')[1:]:
    t = l.split()
    if len(t) == 2: lab[int(t[0])] = t[1]
E = sorted(n for n, c in lab.items() if c == 'E'); hard = [n for n, c in lab.items() if c != 'E']
cpar = {n: {par[abs(x)] for x in cls[n-1]} for n in range(1, len(cls)+1)}
adj = collections.defaultdict(set)
for l in open(P('voisinage1')):
    m = re.match(r'\((C\d+),(C\d+),', l)
    if m: adj[m[1]].add(m[2]); adj[m[2]].add(m[1])
parcels = sorted(set(par.values()) | set(adj)); nv = max(abs(x) for c in cls for x in c)
def ball(c, k):
    d = {c: 0}; fr = [c]
    for s in range(k):
        fr = [y for x in fr for y in adj[x] if y not in d and not d.setdefault(y, s+1) is None]
    return set(d)
def part_muses(part):
    S = Cadical153(); sel = {}
    for n in hard:
        if cpar[n] <= part: S.add_clause(cls[n-1])
    for n in E:
        if cpar[n] <= part: s = nv+1+len(sel); sel[n] = s; S.add_clause(cls[n-1]+[-s])
    ids = sorted(sel); rs = {s: n for n, s in sel.items()}
    def sat(sub): return S.solve(assumptions=[sel[n] for n in sub])
    from pysat.solvers import Glucose4
    M = Glucose4(); X = {n: i+1 for i, n in enumerate(ids)}
    for n in ids: M.add_clause([X[n], -X[n]])
    out = []
    while M.solve():
        m = M.get_model(); seed = [n for n in ids if m[X[n]-1] > 0]
        if sat(seed):
            cur = list(seed)
            for n in ids:
                if n not in cur and sat(cur+[n]): cur.append(n)
            mcs = [n for n in ids if n not in cur]
            if not mcs: break
            M.add_clause([X[n] for n in mcs])
        else:
            cur = list(seed); i = 0
            while i < len(cur):
                t = cur[:i]+cur[i+1:]
                if sat(t): i += 1
                else:
                    core = {rs[l] for l in S.get_core()}; cur = [x for x in t if x in core]; i = min(i, len(cur))
            out.append(frozenset(cur)); M.add_clause([-X[n] for n in cur])
    S.delete(); M.delete(); return out
d = json.load(open(P('marco.json'))); true = {frozenset(m) for m in d['muses']}
print('parcels in graph:', len(parcels), '| connected component sizes:', end=' ')
seen = set(); cs = []
for p in parcels:
    if p in seen: continue
    c = ball(p, 99); seen |= c; cs.append(len(c))
print(cs)
print('\nr=0 atomic C-structure on the REAL Herault block, local engine = modern SAT')
print('k | parts | max part | MUS found / 12 true | complete? | time')
for k in range(0, 6):
    t = time.time(); parts = {frozenset(ball(p, k)) for p in parcels}; found = set()
    for pt in parts: found |= set(part_muses(set(pt)))
    print(f'{k} | {len(parts):5} | {max(len(p) for p in parts):8} | {len(found & true):2} / {len(true)}          | {"YES" if found == true else "no ":3}      | {time.time()-t:.1f}s', flush=True)
