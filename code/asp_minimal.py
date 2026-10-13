from paths import P
import json, time, clingo, clingodl
from clingo.ast import ProgramBuilder, parse_string
from enc_asp import *
d = json.load(open(P('marco.json'))); mcs = {frozenset(E_CLAUSES.index(c) for c in m) for m in d['mcses']}
def subset_minimal(name, dom_mod):
    prog = ENC[name].replace('#minimize { W,E : retract(E), w(E,W) }.', '').replace('#project retract/1.', '') + '\n' + facts() + '\n#heuristic retract(E) : est(E,_,_,_). [1,false]\n'
    ctl = clingo.Control(['0', '--enum-mode=domRec', '--heuristic=Domain', f'--dom-mod={dom_mod}']); thy = None
    if name == 'ASP-DL': thy = clingodl.ClingoDLTheory(); thy.register(ctl)
    with ProgramBuilder(ctl) as b: parse_string(prog, (lambda s: thy.rewrite_ast(s, b.add)) if thy else b.add)
    ctl.ground([('base', [])]); 
    if thy: thy.prepare(ctl)
    out = []; 
    def om(m):
        if thy: thy.on_model(m)
        out.append(frozenset(int(s.arguments[0].number) for s in m.symbols(shown=True) if s.name == 'retract'))
    t = time.time(); ctl.solve(on_model=om); return out, time.time()-t
for name in ('ASP-DL', 'ASP-thresholds'):
    for dm in ('5,16', '3,16'):
        try:
            out, dt = subset_minimal(name, dm); S = set(out)
            # keep only the non-dominated ones (domRec reports models as it goes: earlier models may be dominated by later ones)
            mins = {s for s in S if not any(t < s for t in S)}
            print(f'{name:15} dom-mod={dm:5} models reported {len(out):5} distinct {len(S):5} subset-minimal among them {len(mins):4} == reference 144: {mins == mcs} ({dt:.1f}s)', flush=True)
        except Exception as e: print(name, dm, 'ERR', str(e)[:100])
