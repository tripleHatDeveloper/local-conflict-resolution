from paths import P
# Task A: certificate loop (global SAT check + core-guided shrink) ; Task B: local MUS enumeration on C-structure parts
import sys, random, json
from solvers_cmp import *
from structures import parts_C
from certify import local_found
import certify
task = sys.argv[1]; names = sys.argv[2].split(',') if len(sys.argv) > 2 else ALL
res = collections.defaultdict(list)
if task == 'A':
    insts = [gen(20, 0.2, 2.0, 11000+s) for s in range(4)]
    for I in insts:
        full = oracle_pairs(I); cl, sel = sat_clauses(I, range(I.N)); found0 = local_found(I, 2)
        for nm in names:
            E = Eng(nm, cl, sel); t0 = time.time(); rng = random.Random(0); found = set(found0); rounds = 0; csz = []
            while True:
                rounds += 1; rep = certify.minimal_cover(list(found), rng); kept = [j for j in sel if j not in rep]
                if E.sat(kept): break
                c = E.core(); csz.append(len(c)); found.add(E.shrink([j for j in kept if j in c]))
            dt = time.time()-t0
            valid = all(m & rep for m in full)
            res[nm].append(dict(load=E.load, time=dt, calls=E.calls, rounds=rounds, core=st.mean(csz) if csz else 0, valid=valid)); E.delete()
    print(f'instances: n=20, {len(cl)} clauses each; k0=2 local discovery (oracle), certificate loop on the solver')
    print(f'{"solver":12} | clause load | loop time | SAT calls | rounds | mean raw core size | certified repair valid')
    for nm in names:
        r = res[nm]
        print(f'{nm:12} | {st.mean(x["load"] for x in r):8.2f} s | {st.mean(x["time"] for x in r):7.3f} s | {st.mean(x["calls"] for x in r):9.1f} | {st.mean(x["rounds"] for x in r):6.1f} | {st.mean(x["core"] for x in r):18.1f} | {all(x["valid"] for x in r)}')
if task == 'B':
    I = gen(14, 0.2, 2.0, 12345); full = oracle_pairs(I); parts = [p for p in parts_C(I, 2, 5, 0)]
    plist = [sorted(p) for p in parts]; data = []
    for p in plist:
        cl, sel = sat_clauses(I, p); data.append((p, cl, sel))
    ref = set()
    for p in parts: ref |= oracle_pairs(I, p)
    print(f'n=14, C-structure r=2 k=5: {len(parts)} parts, max part {max(len(p) for p in parts)} parcels, {len(full)} true MUS (local oracle finds {len(ref)})')
    print(f'{"solver":12} | total time | of which clause loading | SAT calls | union of local MUS == local oracle')
    for nm in names:
        t0 = time.time(); found = set(); calls = 0; load = 0
        for p, cl, sel in data:
            if not sel: continue
            E = Eng(nm, cl, sel); load += E.load; mus, ok = marco_mus(E, tmax=20); found |= set(mus); calls += E.calls; E.delete()
        print(f'{nm:12} | {time.time()-t0:8.2f} s | {load:8.2f} s | {calls:9} | {found == ref}', flush=True)
