from paths import P
from synth import *
from pysat.examples.rc2 import RC2
from pysat.formula import WCNF
import math, random, statistics as st, collections, json, time

def Phi(x): return 0.5*(1+math.erf(x/math.sqrt(2)))
def p_err(sig, tau=1.5): return min(0.49, max(0.01, 2*(1-Phi(tau/max(sig, 1e-6)))))   # P(|round(N(0,sig))|>=2)
def weight(p): return max(1, int(round(100*math.log((1-p)/p))))

def gen_two_sources(n, seed, rho_e=0.08, rho_s=0.25, sig_e=1.2, sig_s_good=0.7, sig_s_bad=3.0):
    """expert (sparse, rough: sigma_e everywhere) + satellite (dense; reliable on the left half, poor on the right half
    e.g. vegetation / urban double-bounce).  Every estimate remembers its true error and the sigma that produced it."""
    I = gen(n, 0.0, 0.0, seed); rng = random.Random(seed*7+1); est = []; src = []; sg = []; err = []
    for i in range(I.N):
        x = i//n
        for source, rho in (('E', rho_e), ('S', rho_s)):
            if rng.random() < rho:
                sig = sig_e if source == 'E' else (sig_s_good if x < n//2 else sig_s_bad)
                for kind, pk, truth in (('P', 0.8, I.Ltrue[i]), ('M', 0.7, I.Mtrue[i])):
                    if rng.random() < pk:
                        e = int(round(rng.gauss(0, sig))); v = min(I.D-1, max(0, truth+e))
                        est.append((kind, i, v)); src.append(source); sg.append(sig); err.append(abs(e) >= 2)
    I.est, I.src, I.sig, I.err = est, src, sg, err
    return I

def optimal_repair(conflicts, wts, ids):
    """exact weighted MaxSAT on the conflict hypergraph (variable j = keep estimate j)."""
    if not conflicts: return set()
    inv = sorted({j for m in conflicts for j in m}); pos = {j: k+1 for k, j in enumerate(inv)}
    W = WCNF()
    for m in conflicts: W.append([-pos[j] for j in m])                       # cannot keep a whole conflict
    for j in inv: W.append([pos[j]], weight=wts[j])                          # keeping j is worth its weight
    with RC2(W) as r:
        model = r.compute(); kept = {j for j in inv if pos[j] in set(x for x in model if x > 0)}
    return set(inv)-kept

def evaluate(I, strategies):
    full = oracle_pairs(I)
    if not full: return None
    detect = {j for m in full for j in m}                                    # estimates involved in some conflict
    wrong_detectable = {j for j in detect if I.err[j]}
    out = {}
    for name, wts in strategies.items():
        w = {j: wts(I, j) for j in range(len(I.est))}
        R = optimal_repair(list(full), w, range(len(I.est)))
        tp = len(R & wrong_detectable)
        fpL = sum(1 for j in R if not I.err[j] and I.est[j][1]//I.n < I.n//2); fpR = sum(1 for j in R if not I.err[j] and I.est[j][1]//I.n >= I.n//2)
        out[name] = (len(R), tp, len(wrong_detectable), fpL, fpR)
    return out

def strategies(sig_e=1.2, sg=0.7, sb=3.0, n=16):
    ctx = lambda I, j: weight(p_err(I.sig[j]))                                # calibrated, context-dependent
    def glob(I, j):                                                           # one average reliability per source
        return weight(p_err(sig_e)) if I.src[j] == 'E' else weight(p_err(math.sqrt((sg**2+sb**2)/2)))
    return {'min-cardinality (no reliabilities)': lambda I, j: 1,
            'fixed priority: trust expert': lambda I, j: 100 if I.src[j] == 'E' else 1,
            'fixed priority: trust satellite': lambda I, j: 100 if I.src[j] == 'S' else 1,
            'graded, per-source weights': glob,
            'graded, contextual weights': ctx}
if __name__ == '__main__':
    strat = strategies(); tot = collections.defaultdict(lambda: [0, 0, 0, 0, 0]); t0 = time.time(); used = 0
    for seed in range(150):
        I = gen_two_sources(16, 9000+seed); ev = evaluate(I, strat)
        if not ev: continue
        used += 1
        for k, v in ev.items():
            for q in range(5): tot[k][q] += v[q]
    print(used, 'instances', f'{time.time()-t0:.0f}s')
    print('strategy | retracted | precision | recall | F1 | good estimates wrongly retracted: left half (good satellite) / right half (poor satellite)')
    for k, (r, tp, wd, fl, fr) in tot.items():
        pr = tp/r; rc = tp/wd; print(f'{k:36} | {r:4} | {pr:.2f} | {rc:.2f} | {2*pr*rc/(pr+rc):.2f} | {fl:4} / {fr:4}')
