from paths import P
import sys
from solvers_cmp import *
names = sys.argv[1].split(','); I = gen(12, 0.25, 2.0, 777); full = oracle_pairs(I); cl, sel = sat_clauses(I, range(I.N))
print(f'global MARCO (MUS+MCS blocking), n=12: {len(cl)} clauses, {len(sel)} estimates, {len(full)} true MUS, 25 s cap per solver')
print(f'{"solver":12} | MUS found | complete | SAT calls | calls/s  | time')
for nm in names:
    E = Eng(nm, cl, sel); t = time.time(); mus, ok = marco_mus(E, tmax=25); dt = time.time()-t
    print(f'{nm:12} | {len(mus):4}/{len(full):<4} | {str(ok):8} | {E.calls:9} | {E.calls/dt:8.0f} | {dt:5.1f}s | correct subset: {set(mus) <= full}', flush=True); E.delete()
