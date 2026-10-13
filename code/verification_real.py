from paths import P
import json, random
from mus import *      # S, sel (selectors for the 70 estimates), sat(), name()
d = json.load(open(P('marco.json'))); muses = [tuple(m) for m in d['muses']]
long_ = {(54777,54799),(54809,54840),(54810,54841)}
found = {frozenset(m) for m in muses if m not in long_}         # = the nine conflicts between neighbouring parcels
rs = {v: n for n, v in sel.items()}; calls = 0
def solve(sub):
    global calls; calls += 1; return S.solve(assumptions=[sel[n] for n in sub])
def shrink(seed):
    cur = list(seed); solve(cur); core = {rs[l] for l in S.get_core()}; cur = [x for x in cur if x in core]; i = 0
    while i < len(cur):
        t = cur[:i]+cur[i+1:]
        if solve(t): i += 1
        else: core = {rs[l] for l in S.get_core()}; cur = [x for x in t if x in core]; i = min(i, len(cur))
    return frozenset(cur)
rng = random.Random(3); rnd = 0
while True:
    rnd += 1
    cover = set(x for m in found for x in m); order = sorted(cover); rng.shuffle(order)
    for v in order:
        if all(m & (cover-{v}) for m in found): cover -= {v}
    kept = [n for n in E if n not in cover]
    if solve(kept):
        print(f'round {rnd}: global check SAT -> repair certified ({len(cover)} estimates retracted), {calls} SAT calls in total'); break
    m = shrink(kept); found.add(m)
    print(f'round {rnd}: global check UNSAT -> missed conflict {[name(x) for x in sorted(m)]}')
true = set(frozenset(m) for m in muses)
print('final conflict set == all 12 true MUSes:', found == true, '| certified repair is one of the 144 true minimal repairs:', frozenset(cover) in set(frozenset(x) for x in d['mcses']))
