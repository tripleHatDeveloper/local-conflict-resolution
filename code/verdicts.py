from paths import P
import json, re, collections, clingo, clingodl
from clingo.ast import ProgramBuilder, parse_string
import lifted_enc as L
M = L.load_real(); d = json.load(open(P('marco.json')))
mcs = [frozenset(c-54776 for c in m) for m in d['mcses']]; mus = [frozenset(c-54776 for c in m) for m in d['muses']]
best = [m for m in mcs if len(m) == min(map(len, mcs))]
name = lambda j: f"{M['est'][j][0]}.{M['est'][j][1]}={M['est'][j][2]}"
inv = set().union(*mus)
cat = {}
for j in range(70):
    if j not in inv: cat[j] = 'safe (in no conflict)'
    elif all(j in m for m in best): cat[j] = 'certainly erroneous (in every minimum-loss repair)'
    elif any(j in m for m in best): cat[j] = 'ambiguous (in some minimum-loss repairs)'
    elif any(j in m for m in mcs): cat[j] = 'suspect (only in non-minimum minimal repairs)'
    else: cat[j] = 'involved but never retracted'
c = collections.Counter(cat.values()); print('verdicts over the 70 expert estimates:'); [print(f'   {v:3}  {k}') for k, v in c.most_common()]
for k in ('certainly erroneous (in every minimum-loss repair)', 'ambiguous (in some minimum-loss repairs)', 'suspect (only in non-minimum minimal repairs)'):
    print(k + ':', [name(j) for j in range(70) if cat[j] == k])
# ASP cross-check: brave / cautious consequences over minimum-loss repairs (bound = optimum 8)
prog = L.PRE + L.PROGS['ASP-DL'].replace('%S%', '10').replace('%W%', '10').replace('#minimize { W,E : retract(E), w(E,W) }.', ':- #sum { 1,E : retract(E) } > 8.') + '\n' + L.facts(M)
res = {}
for mode in ('brave', 'cautious'):
    ctl = clingo.Control(['0', f'--enum-mode={mode}']); thy = clingodl.ClingoDLTheory(); thy.register(ctl)
    with ProgramBuilder(ctl) as b: parse_string(prog, lambda s: thy.rewrite_ast(s, b.add))
    ctl.ground([('base', [])]); thy.prepare(ctl); last = [None]
    def om(m): thy.on_model(m); last[0] = frozenset(int(s.arguments[0].number) for s in m.symbols(shown=True) if s.name == 'retract')
    ctl.solve(on_model=om); res[mode] = last[0]
brave = frozenset().union(*best); caut = frozenset.intersection(*best)
print(f'\nASP brave consequences == union of the 72 minimum-loss repairs: {res["brave"] == brave}  ({len(brave)} estimates)')
print(f'ASP cautious consequences == intersection of the 72 repairs:   {res["cautious"] == caut}  ({len(caut)} estimates)')
json.dump({str(j): cat[j] for j in range(70)}, open(P('verdicts.json'), 'w'))
