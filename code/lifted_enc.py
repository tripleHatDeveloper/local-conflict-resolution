from paths import P
"""One lifted model, many encodings.  model = dict(step, width, dom{'P|k':[values]}, dry{P:val|None}, adj[{X,Y,lo,hi}], est[[P,k,V]])"""
import json, time, itertools, clingo, clingodl
from clingo.ast import ProgramBuilder, parse_string
from pysat.solvers import Solver
from pysat.examples.rc2 import RC2
from pysat.formula import WCNF

def load_real():
    M = json.load(open(P('lifted_model.json'))); M['step'] = 10; M['width'] = 10; return M
def from_synth(I, weights=None):
    """convert a synth.Inst (monotone flow model) into the lifted model"""
    dom = {}; adj = []
    for p in range(I.N):
        for k in 'pm': dom[f'c{p}|{k}'] = list(range(I.D))
        for q in I.succ[p]:                        # arc p->q : level(q) <= level(p)   (X - Y <= 0 with X=q, Y=p)
            for k in 'pm': adj.append(dict(X=[f'c{q}', k], Y=[f'c{p}', k], lo=None, hi=0))
    est = [[f'c{i}', k.lower(), v] for (k, i, v) in I.est]
    return dict(step=1, width=0, dom=dom, dry={f'c{p}': None for p in range(I.N)}, adj=adj, est=est)
sym = lambda p: p.lower()
# --------------------------------------------------------------------------------------------- ASP
def facts(M, weights=None):
    f = []
    for key, vs in M['dom'].items():
        p, k = key.split('|')
        for v in vs: f.append(f'dom({sym(p)},{k},{v}).')
    for p, g in M['dry'].items():
        if g is not None: f.append(f'dry({sym(p)},{g}).')
    for a in M['adj']:
        (xp, xk), (yp, yk) = a['X'], a['Y']
        if a['lo'] is not None: f.append(f'dmin({sym(xp)},{xk},{sym(yp)},{yk},{a["lo"]}).')
        if a['hi'] is not None: f.append(f'dmax({sym(xp)},{xk},{sym(yp)},{yk},{a["hi"]}).')
    for j, (p, k, v) in enumerate(M['est']): f.append(f'est({j},{sym(p)},{k},{v}). w({j},{1 if weights is None else weights[j]}).')
    return '\n'.join(f)
PRE = '#defined dmin/5.\n#defined dmax/5.\n#defined dry/2.\n'
VALUES = '''group(P,K) :- dom(P,K,_).
1 { lev(P,K,V) : dom(P,K,V) } 1 :- group(P,K).
{ retract(E) } :- est(E,_,_,_).
:- est(E,P,K,V), not retract(E), not lev(P,K,V).
dryok(P,G,G) :- dry(P,G).
:- lev(P,m,M), lev(P,p,Q), Q-M < %W%, not dryok(P,M,Q).
:- lev(X,KX,A), lev(Y,KY,B), dmin(X,KX,Y,KY,L), A-B < L.
:- lev(X,KX,A), lev(Y,KY,B), dmax(X,KX,Y,KY,H), A-B > H.
#minimize { W,E : retract(E), w(E,W) }.
#project retract/1.
#show retract/1.
'''
THRESH = '''group(P,K) :- dom(P,K,_).
lomin(P,K,L) :- group(P,K), L = #min { V : dom(P,K,V) }.
himax(P,K,H) :- group(P,K), H = #max { V : dom(P,K,V) }.
{ ge(P,K,V) : dom(P,K,V), not lomin(P,K,V) }.
ge(P,K,V) :- lomin(P,K,V).
:- ge(P,K,V), dom(P,K,V-%S%), not ge(P,K,V-%S%).
{ retract(E) } :- est(E,_,_,_).
:- est(E,P,K,V), not retract(E), not ge(P,K,V).
:- est(E,P,K,V), not retract(E), dom(P,K,V+%S%), ge(P,K,V+%S%).
{ dryon(P) } :- dry(P,_).
:- dryon(P), dry(P,G), ge(P,m,V), V > G.
:- dryon(P), dry(P,G), ge(P,p,V), V > G.
:- group(P,m), not dryon(P), ge(P,m,V), dom(P,p,V+%W%), not ge(P,p,V+%W%).
:- group(P,m), not dryon(P), ge(P,m,V), himax(P,p,H), V+%W% > H.
:- dmin(X,KX,Y,KY,L), ge(Y,KY,V), himax(X,KX,H), V+L > H.
:- dmin(X,KX,Y,KY,L), ge(Y,KY,V), dom(X,KX,V+L), lomin(X,KX,Xm), V+L > Xm, not ge(X,KX,V+L).
:- dmax(X,KX,Y,KY,H), ge(X,KX,V), himax(Y,KY,Ym), V-H > Ym.
:- dmax(X,KX,Y,KY,H), ge(X,KX,V), dom(Y,KY,V-H), lomin(Y,KY,Ym), V-H > Ym, not ge(Y,KY,V-H).
#minimize { W,E : retract(E), w(E,W) }.
#project retract/1.
#show retract/1.
'''
DLP = '''group(P,K) :- dom(P,K,_).
lomin(P,K,L) :- group(P,K), L = #min { V : dom(P,K,V) }.
himax(P,K,H) :- group(P,K), H = #max { V : dom(P,K,V) }.
&diff { lvl(P,K) - 0 } >= L :- lomin(P,K,L).
&diff { lvl(P,K) - 0 } <= H :- himax(P,K,H).
{ retract(E) } :- est(E,_,_,_).
&diff { lvl(P,K) - 0 } = V :- est(E,P,K,V), not retract(E).
{ dryon(P) } :- dry(P,_).
&diff { lvl(P,m) - lvl(P,p) } <= -%W% :- group(P,m), not dryon(P).
&diff { lvl(P,m) - 0 } = G :- dryon(P), dry(P,G).
&diff { lvl(P,p) - 0 } = G :- dryon(P), dry(P,G).
&diff { lvl(Y,KY) - lvl(X,KX) } <= -L :- dmin(X,KX,Y,KY,L).
&diff { lvl(X,KX) - lvl(Y,KY) } <= H :- dmax(X,KX,Y,KY,H).
#minimize { W,E : retract(E), w(E,W) }.
#show retract/1.
'''
PROGS = {'ASP-values': VALUES, 'ASP-thresholds': THRESH, 'ASP-DL': DLP}
def run_asp(name, M, weights=None, mode='opt', tl=120):
    prog = PRE + PROGS[name].replace('%S%', str(M['step'])).replace('%W%', str(M['width'])) + '\n' + facts(M, weights)
    args = ['0', f'--opt-mode={"opt" if mode == "opt" else "optN"}'] + (['--opt-strategy=usc'] if mode == 'opt' else []) + (['--project'] if name != 'ASP-DL' and mode != 'opt' else [])
    ctl = clingo.Control(args); thy = None
    if name == 'ASP-DL': thy = clingodl.ClingoDLTheory(); thy.register(ctl)
    t0 = time.time()
    with ProgramBuilder(ctl) as b: parse_string(prog, (lambda s: thy.rewrite_ast(s, b.add)) if thy else b.add)
    ctl.ground([('base', [])]); tg = time.time()-t0
    if thy: thy.prepare(ctl)
    sols = {}
    def om(m):
        if thy: thy.on_model(m)
        sols.setdefault(tuple(m.cost), set()).add(frozenset(int(s.arguments[0].number) for s in m.symbols(shown=True) if s.name == 'retract'))
    t1 = time.time(); h = ctl.solve(on_model=om, async_=True)
    if not h.wait(tl): h.cancel(); r = 'TIMEOUT'; h.wait()
    else: r = h.get()
    ts = time.time()-t1
    lp = ctl.statistics['problem']['lp']; opt = min(sols) if sols else None
    return dict(opt=opt[0] if opt else None, repairs=sols.get(opt, set()) if opt else set(), ground=tg, solve=ts, size=int(lp['rules']), atoms=int(lp['atoms']), complete=str(r))
# --------------------------------------------------------------------------------------------- CNF encodings
def cnf_order(M):
    """order encoding: x[g][v] <=> level(g) >= v.  Returns clauses, selector map est->var, #vars"""
    S = M['step']; W = M['width']; var = {}; nv = 0; cl = []
    def X(g, v):
        nonlocal nv
        if (g, v) not in var: nv += 1; var[(g, v)] = nv
        return var[(g, v)]
    dom = {tuple(k.split('|')): v for k, v in M['dom'].items()}; lo = {g: min(v) for g, v in dom.items()}; hi = {g: max(v) for g, v in dom.items()}
    for g, vs in dom.items():
        for v in vs:
            if v > lo[g] and v-S in dom[g]: 
                if v-S > lo[g]: cl.append([-X(g, v), X(g, v-S)])
    def geq(g, v):                                   # literal for level(g) >= v ; True/False/var
        if v <= lo[g]: return True
        if v > hi[g]: return False
        return X(g, v)
    def implies(prem, concl, extra=()):             # prem -> concl  (prem/concl in {True,False,var}) ; extra literals appended to clause
        if prem is False or concl is True: return
        c = list(extra)
        if prem is not True: c.append(-prem)
        if concl is not False: c.append(concl)
        cl.append(c)
    dry = {p: g for p, g in M['dry'].items() if g is not None}; dvar = {}
    for p, g in dry.items(): nv += 1; dvar[p] = nv
    for p in {k.split('|')[0] for k in M['dom']}:
        gm, gp = (p, 'm'), (p, 'p')
        for v in dom[gm]:                            # not dry: m>=v -> p>=v+W
            implies(geq(gm, v), geq(gp, v+W), extra=([dvar[p]] if p in dvar else []))
        if p in dvar:
            for v in dom[gm]:
                if v > dry[p]: cl.append([-dvar[p], -X(gm, v)])
            for v in dom[gp]:
                if v > dry[p]: cl.append([-dvar[p], -X(gp, v)])
    for a in M['adj']:
        gx, gy = tuple(a['X']), tuple(a['Y'])
        if a['hi'] is not None:                      # X - Y <= hi :  X>=v -> Y >= v-hi
            for v in dom[gx]: implies(geq(gx, v), geq(gy, v-a['hi']))
        if a['lo'] is not None:                      # X - Y >= lo :  Y>=v -> X >= v+lo
            for v in dom[gy]: implies(geq(gy, v), geq(gx, v+a['lo']))
    sel = {}
    for j, (p, k, v) in enumerate(M['est']):
        g = (p, k); nv += 1; sel[j] = nv
        for lit, ok in ((geq(g, v), True), (geq(g, v+S), False)):
            if lit is True and ok: continue
            if lit is False and not ok: continue
            if lit is True and not ok: cl.append([-sel[j]]); continue
            if lit is False and ok: cl.append([-sel[j]]); continue
            cl.append([-sel[j], lit if ok else -lit])
    return cl, sel, nv
def cnf_direct_size(M):
    """number of clauses of the direct encoding (ALO + AMO + one nogood per forbidden value pair + estimates)"""
    dom = {tuple(k.split('|')): v for k, v in M['dom'].items()}; n = len(dom) + len(M['est'])
    for g, vs in dom.items(): n += len(vs)*(len(vs)-1)//2
    W = M['width']
    for p in {k.split('|')[0] for k in M['dom']}:
        n += sum(1 for a in dom[(p, 'm')] for b in dom[(p, 'p')] if not (b-a >= W or (M['dry'].get(p) is not None and a == b == M['dry'][p])))
    for a in M['adj']:
        gx, gy = tuple(a['X']), tuple(a['Y'])
        n += sum(1 for u in dom[gx] for v in dom[gy] if (a['lo'] is not None and u-v < a['lo']) or (a['hi'] is not None and u-v > a['hi']))
    return n
def run_rc2_order(M, weights=None, solver='cadical153'):
    t0 = time.time(); cl, sel, nv = cnf_order(M); W = WCNF()
    for c in cl: W.append(c)
    for j, s in sel.items(): W.append([s], weight=1 if weights is None else weights[j])
    tb = time.time()-t0; t1 = time.time()
    with RC2(W, solver=solver) as r:
        m = r.compute(); cost = r.cost
    return dict(opt=cost, repairs={frozenset(j for j, s in sel.items() if s not in set(x for x in m if x > 0))}, ground=tb, solve=time.time()-t1, size=len(cl), atoms=nv)
# --------------------------------------------------------------------------------------------- integer encodings (SMT / CP)
def run_z3(M, weights=None, tl=120):
    import z3
    t0 = time.time(); o = z3.Optimize(); o.set('timeout', int(tl*1000)); lv = {}
    for k, vs in M['dom'].items():
        v = z3.Int(k.replace('|', '_')); lv[tuple(k.split('|'))] = v; o.add(v >= min(vs), v <= max(vs))
    keep = [z3.Bool(f'k{j}') for j in range(len(M['est']))]
    for j, (p, k, v) in enumerate(M['est']): o.add(z3.Implies(keep[j], lv[(p, k)] == v))
    for p in {k.split('|')[0] for k in M['dom']}:
        m_, p_ = lv[(p, 'm')], lv[(p, 'p')]
        g = M['dry'].get(p)
        o.add(p_ - m_ >= M['width'] if g is None else z3.Or(p_ - m_ >= M['width'], z3.And(m_ == g, p_ == g)))
    for a in M['adj']:
        x, y = lv[tuple(a['X'])], lv[tuple(a['Y'])]
        if a['lo'] is not None: o.add(x - y >= a['lo'])
        if a['hi'] is not None: o.add(x - y <= a['hi'])
    w = weights or [1]*len(keep); o.minimize(z3.Sum([z3.If(keep[j], 0, w[j]) for j in range(len(keep))])); tb = time.time()-t0
    t1 = time.time(); r = o.check(); ts = time.time()-t1; m = o.model() if str(r) == 'sat' else None
    cost = sum(w[j] for j in range(len(keep)) if m is not None and not z3.is_true(m.eval(keep[j], model_completion=True)))
    return dict(opt=cost if m is not None else None, repairs=set(), ground=tb, solve=ts, size=len(o.assertions()), atoms=len(lv), complete=str(r))
def run_cpsat(M, weights=None, tl=120):
    from ortools.sat.python import cp_model
    t0 = time.time(); m = cp_model.CpModel(); lv = {}
    for k, vs in M['dom'].items(): lv[tuple(k.split('|'))] = m.NewIntVar(min(vs), max(vs), k)
    keep = [m.NewBoolVar(f'k{j}') for j in range(len(M['est']))]
    for j, (p, k, v) in enumerate(M['est']): m.Add(lv[(p, k)] == v).OnlyEnforceIf(keep[j])
    for p in {k.split('|')[0] for k in M['dom']}:
        m_, p_ = lv[(p, 'm')], lv[(p, 'p')]; g = M['dry'].get(p)
        if g is None: m.Add(p_ - m_ >= M['width'])
        else:
            d = m.NewBoolVar(f'd{p}'); m.Add(p_ - m_ >= M['width']).OnlyEnforceIf(d.Not()); m.Add(m_ == g).OnlyEnforceIf(d); m.Add(p_ == g).OnlyEnforceIf(d)
    for a in M['adj']:
        x, y = lv[tuple(a['X'])], lv[tuple(a['Y'])]
        if a['lo'] is not None: m.Add(x - y >= a['lo'])
        if a['hi'] is not None: m.Add(x - y <= a['hi'])
    w = weights or [1]*len(keep); m.Minimize(sum(w[j]*(1-keep[j]) for j in range(len(keep)))); tb = time.time()-t0
    s = cp_model.CpSolver(); s.parameters.num_workers = 1; s.parameters.max_time_in_seconds = tl; t1 = time.time(); r = s.Solve(m); ts = time.time()-t1
    ok = r in (cp_model.OPTIMAL,)
    return dict(opt=int(round(s.ObjectiveValue())) if r in (cp_model.OPTIMAL, cp_model.FEASIBLE) else None, repairs=set(), ground=tb, solve=ts, size=len(m.Proto().constraints), atoms=len(lv), complete='OPT' if ok else s.StatusName(r))
