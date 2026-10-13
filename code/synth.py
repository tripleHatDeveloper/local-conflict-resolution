from paths import P
import random, math, heapq, collections, time
from pysat.solvers import Cadical153, Glucose4

# ---------------------------------------------------------------- generator
class Inst: pass

def gen(n, rho, sigma, seed, pP=0.8, pM=0.7, tilt=None):
    """n x n parcels, 4-adjacency. Flow arcs go from higher to lower water surface h.
    Estimates: noisy observations of upper (P) / lower (M) level, density rho, noise sigma (levels)."""
    rng = random.Random(seed)
    th = rng.uniform(0, 2*math.pi) if tilt is None else tilt
    bumps = [(rng.uniform(0,n), rng.uniform(0,n), rng.uniform(2,5), rng.uniform(n/6,n/3)) for _ in range(4)]
    def h(x, y):
        v = x*math.cos(th) + y*math.sin(th)                    # ~1 level per hop slope
        for bx, by, a, s in bumps: v += a*math.exp(-((x-bx)**2+(y-by)**2)/(2*s*s))
        return v
    N = n*n; idx = lambda x, y: x*n+y
    H = [h(x, y) for x in range(n) for y in range(n)]
    lo = min(H); L = [int(round(v-lo))+3 for v in H]            # true upper level
    D = max(L)+6
    Mtrue = [max(0, l-1) for l in L]                            # true lower level (monotone too)
    nbr = [[] for _ in range(N)]; succ = [[] for _ in range(N)]; pred = [[] for _ in range(N)]
    for x in range(n):
        for y in range(n):
            u = idx(x, y)
            for dx, dy in ((1,0),(0,1)):
                if x+dx < n and y+dy < n:
                    v = idx(x+dx, y+dy); nbr[u].append(v); nbr[v].append(u)
                    a, b = (u, v) if (H[u] > H[v] or (H[u] == H[v] and u < v)) else (v, u)
                    succ[a].append(b); pred[b].append(a)
    est = []                                                    # (kind, parcel, value)
    for i in range(N):
        if rng.random() < rho:
            if rng.random() < pP: est.append(('P', i, min(D-1, max(0, L[i]+int(round(rng.gauss(0, sigma)))))))
            if rng.random() < pM: est.append(('M', i, min(D-1, max(0, Mtrue[i]+int(round(rng.gauss(0, sigma)))))))
    I = Inst(); I.n, I.N, I.D, I.nbr, I.succ, I.pred, I.est = n, N, D, nbr, succ, pred, est
    I.Ltrue, I.Mtrue = L, Mtrue
    return I

# ------------------------------------------------- semantic oracle (pairwise)
# Constraints: p_a>=p_b, m_a>=m_b on every arc a->b, m_i<=p_i.  Every MUS over the estimates is a pair
# (u gives an upper bound, l a lower bound) with a directed path a~>b  (a may equal b) and val_u < val_l:
#   (P_a,P_b) a!=b ; (P_a,M_b) ; (M_a,M_b) a!=b.
def compatible(eu, el):
    ku, a, v = eu; kl, b, w = el
    if ku == kl and a == b: return v != w          # two estimates of the SAME variable that disagree
    if not v < w: return False
    if ku == 'P' and kl == 'P': return a != b
    if ku == 'P' and kl == 'M': return True
    if ku == 'M' and kl == 'M': return a != b
    return False

def reach_within(I, src, allowed):
    """set of parcels reachable from src by directed paths staying in `allowed` (incl. src)."""
    seen = {src}; st = [src]
    while st:
        u = st.pop()
        for v in I.succ[u]:
            if v in allowed and v not in seen: seen.add(v); st.append(v)
    return seen

def oracle_pairs(I, parcels=None):
    """all MUS pairs (as frozenset of estimate indices) inside the sub-space `parcels` (default: all)."""
    allowed = set(range(I.N)) if parcels is None else set(parcels)
    byp = collections.defaultdict(list)
    for j, (k, i, v) in enumerate(I.est):
        if i in allowed: byp[i].append(j)
    out = set()
    for a, js in byp.items():
        R = reach_within(I, a, allowed)
        for b in R:
            for jb in byp.get(b, ()):
                for ja in js:
                    if ja != jb and compatible(I.est[ja], I.est[jb]): out.add(frozenset((ja, jb)))
    return out

def ball(I, c, R):
    dist = {c: 0}; fr = [c]
    for d in range(1, R+1):
        nf = []
        for u in fr:
            for v in I.nbr[u]:
                if v not in dist: dist[v] = d; nf.append(v)
        fr = nf
    return dist

def bottleneck_ext(I, a):
    """for each parcel b: min over directed paths a~>b of max adjacency-distance from a along the path
    = smallest k such that the k-ball around a contains a directed path a~>b."""
    dist = ball(I, a, 10**6) if I.N < 10**6 else None
    best = {a: 0}; pq = [(0, a)]
    while pq:
        c, u = heapq.heappop(pq)
        if c > best[u]: continue
        for v in I.succ[u]:
            nc = max(c, dist[v])
            if nc < best.get(v, 10**9): best[v] = nc; heapq.heappush(pq, (nc, v))
    return best

# --------------------------------------------------------------- SAT engine
def build_sat(I, parcels, solver=Cadical153):
    """order encoding: P[i][v] <=> p_i>=v (v=1..D-1), same for M. Estimates = soft groups with a selector."""
    D = I.D; parcels = sorted(parcels); pos = {p: k for k, p in enumerate(parcels)}
    base = lambda p, kind, v: 1 + (pos[p]*2 + (0 if kind == 'P' else 1))*(D-1) + (v-1)
    S = solver()
    for p in parcels:
        for kind in 'PM':
            for v in range(1, D-1): S.add_clause([-base(p, kind, v+1), base(p, kind, v)])
        for v in range(1, D): S.add_clause([-base(p, 'M', v), base(p, 'P', v)])
        for q in I.succ[p]:
            if q in pos:
                for v in range(1, D):
                    S.add_clause([-base(q, 'P', v), base(p, 'P', v)])
                    S.add_clause([-base(q, 'M', v), base(p, 'M', v)])
    nv = len(parcels)*2*(D-1); sel = {}
    for j, (k, i, val) in enumerate(I.est):
        if i not in pos: continue
        s = nv+1+len(sel); sel[j] = s
        if val >= 1: S.add_clause([-s, base(i, k, val)])
        if val+1 <= D-1: S.add_clause([-s, -base(i, k, val+1)])
    return S, sel

def marco(S, sel, want_mcs=False, tmax=60):
    ids = sorted(sel); rs = {s: j for j, s in sel.items()}
    def sat(sub): return S.solve(assumptions=[sel[j] for j in sub])
    def shrink(seed):
        cur = list(seed); i = 0
        while i < len(cur):
            trial = cur[:i]+cur[i+1:]
            if sat(trial): i += 1
            else:
                core = {rs[l] for l in S.get_core()}; cur = [x for x in trial if x in core]; i = min(i, len(cur))
        return sorted(cur)
    def grow(seed):
        cur = list(seed)
        for j in ids:
            if j not in cur and sat(cur+[j]): cur.append(j)
        return cur
    M = Glucose4(); X = {j: k+1 for k, j in enumerate(ids)}
    for j in ids: M.add_clause([X[j], -X[j]])
    try: M.set_phases([X[j] for j in ids])
    except Exception: pass
    muses, mcses = [], []; t0 = time.time(); complete = True
    while M.solve():
        if time.time()-t0 > tmax: complete = False; break
        m = M.get_model(); seed = [j for j in ids if m[X[j]-1] > 0]
        if sat(seed):
            mss = grow(seed); mcs = [j for j in ids if j not in mss]
            if want_mcs: mcses.append(mcs)
            M.add_clause([X[j] for j in mcs]) if mcs else None
            if not mcs: break
        else:
            mus = shrink(seed); muses.append(frozenset(mus)); M.add_clause([-X[j] for j in mus])
    M.delete()
    return muses, mcses, complete
