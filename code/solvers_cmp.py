from paths import P
from synth import *
from pysat.solvers import Solver
import time, statistics as st, collections, json

ALL = ['cadical153','cadical195','glucose3','glucose4','glucose42','lingeling','maplechrono','maplecm','maplesat','mergesat3','minicard','minisat22','minisat-gh']

def sat_clauses(I, parcels):
    """same order encoding as build_sat, returned as plain data so every solver loads IDENTICAL clauses."""
    D = I.D; parcels = sorted(parcels); pos = {p: k for k, p in enumerate(parcels)}
    base = lambda p, kind, v: 1 + (pos[p]*2 + (0 if kind == 'P' else 1))*(D-1) + (v-1)
    cl = []
    for p in parcels:
        for kind in 'PM':
            for v in range(1, D-1): cl.append([-base(p, kind, v+1), base(p, kind, v)])
        for v in range(1, D): cl.append([-base(p, 'M', v), base(p, 'P', v)])
        for q in I.succ[p]:
            if q in pos:
                for v in range(1, D): cl.append([-base(q, 'P', v), base(p, 'P', v)]); cl.append([-base(q, 'M', v), base(p, 'M', v)])
    nv = len(parcels)*2*(D-1); sel = {}
    for j, (k, i, val) in enumerate(I.est):
        if i not in pos: continue
        s = nv+1+len(sel); sel[j] = s
        if val >= 1: cl.append([-s, base(i, k, val)])
        if val+1 <= D-1: cl.append([-s, -base(i, k, val+1)])
    return cl, sel

class Eng:
    """a solver + selector map with call counting and MUS/MCS tools (assumption-based, core-guided shrinking)."""
    def __init__(self, name, cl, sel):
        t = time.time(); self.S = Solver(name=name, bootstrap_with=cl); self.load = time.time()-t
        self.sel = sel; self.rs = {s: j for j, s in sel.items()}; self.calls = 0; self.name = name
    def sat(self, sub): self.calls += 1; return self.S.solve(assumptions=[self.sel[j] for j in sub])
    def core(self): return {self.rs[l] for l in self.S.get_core()}
    def shrink(self, seed):
        cur = list(seed); i = 0
        while i < len(cur):
            t = cur[:i]+cur[i+1:]
            if self.sat(t): i += 1
            else: c = self.core(); cur = [x for x in t if x in c]; i = min(i, len(cur))
        return frozenset(cur)
    def delete(self): self.S.delete()

def marco_mus(E, tmax=60, map_name='glucose4'):
    ids = sorted(E.sel); M = Solver(name=map_name); X = {j: k+1 for k, j in enumerate(ids)}
    for j in ids: M.add_clause([X[j], -X[j]])
    out = []; t0 = time.time(); complete = True
    while M.solve():
        if time.time()-t0 > tmax: complete = False; break
        m = M.get_model(); seed = [j for j in ids if m[X[j]-1] > 0]
        if E.sat(seed):
            cur = list(seed)
            for j in ids:
                if j not in cur and E.sat(cur+[j]): cur.append(j)
            mcs = [j for j in ids if j not in cur]
            if not mcs: break
            M.add_clause([X[j] for j in mcs])
        else:
            if E.sat(seed) is False: pass
            c = E.core(); seed = [j for j in seed if j in c] if c else seed
            mus = E.shrink(seed); out.append(mus); M.add_clause([-X[j] for j in mus])
    M.delete(); return out, complete
