from paths import P
import sys, time, math
from graded import *
from pysat.examples.rc2 import RC2
from pysat.formula import WCNF
import networkx as nx
def build(seed, n=32, re_=0.25, rs=0.7):
    I = gen_two_sources(n, seed, rho_e=re_, rho_s=rs); full = oracle_pairs(I)
    w = {j: weight(p_err(I.sig[j])) for j in range(len(I.est))}
    return I, [tuple(sorted(m)) for m in full], w
def wcnf(conf, w):
    inv = sorted({j for m in conf for j in m}); pos = {j: k+1 for k, j in enumerate(inv)}; W = WCNF()
    for m in conf: W.append([-pos[a] for a in m])
    for j in inv: W.append([pos[j]], weight=w[j])
    return W, inv, pos
def cost_of(model_kept_ids, inv, w): return sum(w[j] for j in inv if j not in model_kept_ids)
def run_rc2(conf, w, name):
    W, inv, pos = wcnf(conf, w); t = time.time()
    with RC2(W, solver=name) as r:
        m = r.compute(); dt = time.time()-t; ok = set(x for x in m if x > 0)
    return sum(w[j] for j in inv if pos[j] not in ok), dt
def run_cpsat(conf, w):
    from ortools.sat.python import cp_model
    inv = sorted({j for m in conf for j in m}); M = cp_model.CpModel(); x = {j: M.NewBoolVar(f'x{j}') for j in inv}
    for a, b in conf: M.Add(x[a]+x[b] <= 1)
    M.Maximize(sum(w[j]*x[j] for j in inv)); S = cp_model.CpSolver(); S.parameters.num_workers = 1; t = time.time(); S.Solve(M); dt = time.time()-t
    return sum(w[j] for j in inv)-int(round(S.ObjectiveValue())), dt
def run_z3(conf, w):
    import z3
    inv = sorted({j for m in conf for j in m}); o = z3.Optimize(); x = {j: z3.Bool(f'x{j}') for j in inv}
    for a, b in conf: o.add(z3.Not(z3.And(x[a], x[b])))
    obj = o.maximize(z3.Sum([z3.If(x[j], w[j], 0) for j in inv])); t = time.time(); o.check(); dt = time.time()-t
    return sum(w[j] for j in inv)-int(str(o.model().eval(z3.Sum([z3.If(x[j], w[j], 0) for j in inv])))), dt
if __name__ == '__main__':
    insts = [build(30000+s) for s in range(3)]
    for I, conf, w in insts:
        G = nx.Graph(); G.add_edges_from(conf); comps = sorted((len(c) for c in nx.connected_components(G)), reverse=True)
        print(f'instance: {len(I.est)} estimates, {len(conf)} conflicts, largest conflict components: {comps[:4]}')
    names = sys.argv[1].split(',')
    print(f'\n{"engine":14} | mean time over 3 instances | optimum cost (identical across engines?)')
    ref = None
    for nm in names:
        try:
            if nm == 'cpsat': rs_ = [run_cpsat(c, w) for I, c, w in insts]
            elif nm == 'z3': rs_ = [run_z3(c, w) for I, c, w in insts]
            else: rs_ = [run_rc2(c, w, nm) for I, c, w in insts]
            costs = [r[0] for r in rs_]; ref = ref or costs
            print(f'{nm:14} | {st.mean(r[1] for r in rs_):10.3f} s | {costs} {"OK" if costs == ref else "MISMATCH"}', flush=True)
        except Exception as e: print(f'{nm:14} | FAILED {type(e).__name__}: {str(e)[:70]}')
