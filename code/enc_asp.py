from paths import P
import json, time, itertools, clingo, clingodl
from clingo.ast import ProgramBuilder, parse_string
M = json.load(open(P('lifted_model.json')))
E_CLAUSES = list(range(54776, 54776+70))          # clause numbers of the 70 estimates (1-based numbering used in marco.json)
def sym(p): return p.lower()
# ------------------------------------------------------------------ facts shared by the ASP encodings
def facts(model=M, weights=None):
    f = []
    for key, vs in model['dom'].items():
        p, k = key.split('|'); 
        for v in vs: f.append(f'dom({sym(p)},{k},{v}).')
    for p, g in model['dry'].items():
        if g is not None: f.append(f'dry({sym(p)},{g}).')
    for a in model['adj']:
        (xp, xk), (yp, yk) = a['X'], a['Y']
        if a['lo'] is not None: f.append(f'dmin({sym(xp)},{xk},{sym(yp)},{yk},{a["lo"]}).')
        if a['hi'] is not None: f.append(f'dmax({sym(xp)},{xk},{sym(yp)},{yk},{a["hi"]}).')
    for j, (p, k, v) in enumerate(model['est']):
        f.append(f'est({j},{sym(p)},{k},{v}).'); f.append(f'w({j},{1 if weights is None else weights[j]}).')
    return '\n'.join(f)
# ------------------------------------------------------------------ (A) ASP, one atom per value (same granularity as the direct CNF)
ASP_DIRECT = '''
group(P,K) :- dom(P,K,_).
1 { lev(P,K,V) : dom(P,K,V) } 1 :- group(P,K).
{ retract(E) } :- est(E,_,_,_).
:- est(E,P,K,V), not retract(E), not lev(P,K,V).
parcel(P) :- group(P,_).
dryok(P,G,G) :- dry(P,G).
:- lev(P,m,M), lev(P,p,Q), Q-M < 10, not dryok(P,M,Q).
:- lev(X,KX,A), lev(Y,KY,B), dmin(X,KX,Y,KY,L), A-B < L.
:- lev(X,KX,A), lev(Y,KY,B), dmax(X,KX,Y,KY,H), A-B > H.
#minimize { W,E : retract(E), w(E,W) }.
#project retract/1.
#show retract/1.
'''
# ------------------------------------------------------------------ (B) ASP, threshold ("order") atoms: ge(P,K,V) <=> level >= V
ASP_ORDER = '''
group(P,K) :- dom(P,K,_).
lomin(P,K,L) :- group(P,K), L = #min { V : dom(P,K,V) }.
himax(P,K,H) :- group(P,K), H = #max { V : dom(P,K,V) }.
{ ge(P,K,V) : dom(P,K,V), not lomin(P,K,V) }.
ge(P,K,V) :- lomin(P,K,V).
:- ge(P,K,V), dom(P,K,V-10), not ge(P,K,V-10).
{ retract(E) } :- est(E,_,_,_).
:- est(E,P,K,V), not retract(E), not ge(P,K,V).
:- est(E,P,K,V), not retract(E), dom(P,K,V+10), ge(P,K,V+10).
% width rule: p >= m + 10, unless the parcel is "dry" (m = p = floor)
{ dryon(P) } :- dry(P,_).
:- dryon(P), dry(P,G), ge(P,m,V), V > G.
:- dryon(P), dry(P,G), ge(P,p,V), V > G.
:- group(P,m), not dryon(P), ge(P,m,V), dom(P,p,V+10), not ge(P,p,V+10).
:- group(P,m), not dryon(P), ge(P,m,V), not dom(P,p,V+10).
% difference constraints  X - Y >= L :  Y >= V  ->  X >= V+L
:- dmin(X,KX,Y,KY,L), ge(Y,KY,V), dom(X,KX,V+L), not ge(X,KX,V+L), V+L > Xmin, lomin(X,KX,Xmin).
:- dmin(X,KX,Y,KY,L), ge(Y,KY,V), himax(X,KX,H), V+L > H.
% X - Y <= H :  X >= V -> Y >= V-H
:- dmax(X,KX,Y,KY,H), ge(X,KX,V), dom(Y,KY,V-H), not ge(Y,KY,V-H), V-H > Ymin, lomin(Y,KY,Ymin).
:- dmax(X,KX,Y,KY,H), ge(X,KX,V), himax(Y,KY,Ym), V-H > Ym.
#minimize { W,E : retract(E), w(E,W) }.
#project retract/1.
#show retract/1.
'''
# ------------------------------------------------------------------ (C) ASP + difference logic (clingo-dl): the lifted model almost verbatim
ASP_DL = '''
group(P,K) :- dom(P,K,_).
lomin(P,K,L) :- group(P,K), L = #min { V : dom(P,K,V) }.
himax(P,K,H) :- group(P,K), H = #max { V : dom(P,K,V) }.
&diff { lvl(P,K) - 0 } >= L :- lomin(P,K,L).
&diff { lvl(P,K) - 0 } <= H :- himax(P,K,H).
{ retract(E) } :- est(E,_,_,_).
&diff { lvl(P,K) - 0 } = V :- est(E,P,K,V), not retract(E).
{ dryon(P) } :- dry(P,_).
&diff { lvl(P,m) - lvl(P,p) } <= -10 :- group(P,m), not dryon(P).
&diff { lvl(P,m) - 0 } = G :- dryon(P), dry(P,G).
&diff { lvl(P,p) - 0 } = G :- dryon(P), dry(P,G).
&diff { lvl(Y,KY) - lvl(X,KX) } <= -L :- dmin(X,KX,Y,KY,L).
&diff { lvl(X,KX) - lvl(Y,KY) } <= H :- dmax(X,KX,Y,KY,H).
#minimize { W,E : retract(E), w(E,W) }.
#show retract/1.
'''
for _n in ('ASP_DIRECT','ASP_ORDER','ASP_DL'): globals()[_n] = '#defined dmin/5.\n#defined dmax/5.\n#defined dry/2.\n' + globals()[_n]
ENC = {'ASP-values': ASP_DIRECT, 'ASP-thresholds': ASP_ORDER, 'ASP-DL': ASP_DL}
def run(name, weights=None, extra='', model=M, mode='optN', tlimit=120):
    prog = ENC[name] + '\n' + facts(model, weights) + '\n' + extra
    args = ['0', f'--opt-mode={mode}', '--project' if name != 'ASP-DL' else '--project=no']
    args = [a for a in args if a != '--project=no']
    ctl = clingo.Control(args); thy = None
    if name == 'ASP-DL': thy = clingodl.ClingoDLTheory(); thy.register(ctl)
    t0 = time.time()
    with ProgramBuilder(ctl) as b:
        parse_string(prog, (lambda s: thy.rewrite_ast(s, b.add)) if thy else b.add)
    ctl.ground([('base', [])])
    if thy: thy.prepare(ctl)
    tg = time.time()-t0; best = None; sols = {}
    def om(m):
        nonlocal best
        if thy: thy.on_model(m)
        c = tuple(m.cost); S = frozenset(int(s.arguments[0].number) for s in m.symbols(shown=True) if s.name == 'retract')
        if m.optimality_proven or True: sols.setdefault(c, set()).add(S)
    t1 = time.time(); r = ctl.solve(on_model=om, async_=False); ts = time.time()-t1
    lp = ctl.statistics['problem']['lp']
    opt = min(sols) if sols else None
    return dict(name=name, ground_time=tg, solve_time=ts, atoms=int(lp['atoms']), rules=int(lp['rules']), constraints=int(lp['rules']),
                opt=opt, repairs=sols.get(opt, set()) if opt else set(), sat=str(r))
if __name__ == '__main__':
    d = json.load(open(P('marco.json'))); mcs = {frozenset(E_CLAUSES.index(c) for c in m) for m in d['mcses']}
    best8 = {m for m in mcs if len(m) == min(map(len, mcs))}
    print(f'reference (SAT, CNF-direct, MARCO): {len(mcs)} subset-minimal repairs, {len(best8)} of minimum cardinality {min(map(len, mcs))}')
    for nm in ('ASP-values', 'ASP-thresholds', 'ASP-DL'):
        r = run(nm); print(f"{nm:15} ground {r['ground_time']:6.2f}s solve {r['solve_time']:6.2f}s | ground atoms {r['atoms']:7} rules {r['rules']:7} | optimum {r['opt']} | optimal repairs found {len(r['repairs'])} | equal to reference: {r['repairs'] == best8}", flush=True)
