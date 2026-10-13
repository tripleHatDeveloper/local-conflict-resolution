from paths import P
from synth import *
import random, collections

# ---------- instances with controllable connectivity (walls = barriers that cut adjacency) ----------
def gen_walled(n, rho, sigma, seed, nwalls=2, gaps=0):
    """like gen(), then cut ALL adjacencies across `nwalls` vertical lines except at `gaps` random rows (passes)."""
    I = gen(n, rho, sigma, seed)
    if nwalls == 0: return I
    rng = random.Random(seed*31+7)
    xs = [round(n*(w+1)/(nwalls+1))-1 for w in range(nwalls)]
    cut = set()
    for xw in xs:
        open_rows = set(rng.sample(range(n), gaps)) if gaps else set()
        for y in range(n):
            if y in open_rows: continue
            u, v = xw*n+y, (xw+1)*n+y; cut.add((u, v)); cut.add((v, u))
    I.nbr = [[v for v in I.nbr[u] if (u, v) not in cut] for u in range(I.N)]
    I.succ = [[v for v in I.succ[u] if (u, v) not in cut] for u in range(I.N)]
    I.pred = [[v for v in I.pred[u] if (u, v) not in cut] for u in range(I.N)]
    return I

# ---------- structure builders: each returns a list of parts (sets of parcels = sub-languages) ----------
def multi_ball(I, centers, k):
    dist = {c: 0 for c in centers}; fr = list(centers)
    for d in range(1, k+1):
        nf = []
        for u in fr:
            for v in I.nbr[u]:
                if v not in dist: dist[v] = d; nf.append(v)
        fr = nf
    return frozenset(dist)

def components(I):
    seen = set(); comps = []
    for s in range(I.N):
        if s in seen: continue
        st = [s]; c = set()
        while st:
            u = st.pop()
            if u in c: continue
            c.add(u); st += [v for v in I.nbr[u] if v not in c]
        seen |= c; comps.append(frozenset(c))
    return comps

def parts_global(I): return [frozenset(range(I.N))]
def parts_LS(I):     return components(I)               # Parikh: finest T-splitting = connected components of the relevance graph
def parts_C(I, r, k, seed=0):                           # C-structure: cores partition V (balls of radius r), covers of thickness k
    rng = random.Random(seed); order = list(range(I.N)); rng.shuffle(order); unc = set(range(I.N)); cores = []
    for c in order:
        if c in unc:
            core = {v for v in multi_ball(I, [c], r) if v in unc}; cores.append(core); unc -= core
    return list({multi_ball(I, core, k) for core in cores})
def parts_C_cores(I, cores, k): return list({multi_ball(I, core, k) for core in cores})
def parts_B(I, s, m):                                   # B-structure: overlapping s x s tiles widened by m (index space), no closure required
    n = I.n; parts = []
    for tx in range(0, n, s):
        for ty in range(0, n, s):
            parts.append(frozenset(x*n+y for x in range(max(0, tx-m), min(n, tx+s+m)) for y in range(max(0, ty-m), min(n, ty+s+m))))
    return parts
def kpartition(parts, N):
    mult = collections.Counter(p for part in parts for p in part); return max(mult.values()), len(mult) == N

def found_in(I, parts):
    f = set()
    for p in parts: f |= oracle_pairs(I, p)
    return f

def random_minimal_cover(conflicts, rng):
    cover = set(x for m in conflicts for x in m); order = sorted(cover); rng.shuffle(order)
    for v in order:
        if all(m & (cover-{v}) for m in conflicts): cover -= {v}
    return cover
def valid_fraction(found, full, rng, T=20):
    if not found: return 0.0
    ed = list(found); ok = 0
    for _ in range(T): C = random_minimal_cover(ed, rng); ok += all(m & C for m in full)
    return ok/T
