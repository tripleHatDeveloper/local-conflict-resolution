from paths import P
from lifted_enc import *
import json
M = load_real(); d = json.load(open(P('marco.json'))); mcs = {frozenset(c-54776 for c in m) for m in d['mcses']}; best = {m for m in mcs if len(m) == 8}
print('REAL Herault block -> one lifted model, many encodings (min-cardinality repair; reference optimum 8 with 72 optimal repairs)')
print(f'{"encoding":28} | size (clauses/rules) | build | solve | optimum | all optimal repairs equal reference')
print(f'{"CNF-direct":28} | {cnf_direct_size(M):20} |   -   |   -   | 8       | (reference, SAT MARCO)')
r = run_rc2_order(M); cl, sel, nv = cnf_order(M)
# enumerate all optimal repairs of the order CNF with MARCO-style blocking: use synth.marco
from pysat.solvers import Cadical153
S = Cadical153(bootstrap_with=cl); import sys; import synth
mus, mcs2, ok = synth.marco(S, sel, want_mcs=True, tmax=120); mcs_o = {frozenset(m) for m in mcs2}
print(f'{"CNF-order + RC2 / MARCO":28} | {len(cl):20} | {r["ground"]:.2f}s | {r["solve"]:.2f}s | {r["opt"]}       | {len({m for m in mcs_o if len(m)==8})} optimal; {len(mcs_o)} subset-minimal; equals reference: {mcs_o == mcs}')
for nm in ('ASP-values', 'ASP-thresholds', 'ASP-DL'):
    a = run_asp(nm, M, mode='all'); print(f'{nm:28} | {a["size"]:20} | {a["ground"]:.2f}s | {a["solve"]:.2f}s | {a["opt"]}       | {len(a["repairs"])} optimal; equals reference: {a["repairs"] == best}')
z = run_z3(M); print(f'{"SMT (Z3, integer diff.)":28} | {z["size"]:20} | {z["ground"]:.2f}s | {z["solve"]:.2f}s | {z["opt"]}       | (optimum only)')
c = run_cpsat(M); print(f'{"CP-SAT (integer)":28} | {c["size"]:20} | {c["ground"]:.2f}s | {c["solve"]:.2f}s | {c["opt"]}       | (optimum only)')
