from paths import P
from synth import *
import random, statistics as st, collections, time, json

def local_found(I, R):
    f = set()
    for c in range(I.N): f |= oracle_pairs(I, ball(I, c, R).keys())
    return f

class Counted:
    """wrap a pysat solver and count solve() calls"""
    def __init__(self, S): self.S = S; self.calls = 0
    def solve(self, assumptions=()): self.calls += 1; return self.S.solve(assumptions=assumptions)
    def get_core(self): return self.S.get_core()

def mus_from_core(C, sel, retained):
    """shrink the unsat set `retained` (list of estimate ids) to a MUS using solver C"""
    rs = {s: j for j, s in sel.items()}
    cur = list(retained); C.solve([sel[j] for j in cur]); core = {rs[l] for l in C.get_core()}
    cur = [j for j in cur if j in core]; i = 0
    while i < len(cur):
        trial = cur[:i]+cur[i+1:]
        if C.solve([sel[j] for j in trial]): i += 1
        else:
            core = {rs[l] for l in C.get_core()}; cur = [j for j in trial if j in core]; i = min(i, len(cur))
    return frozenset(cur)

def minimal_cover(conflicts, rng):
    cover = set(x for m in conflicts for x in m); order = sorted(cover); rng.shuffle(order)
    for v in order:
        t = cover - {v}
        if all(m & t for m in conflicts): cover = t
    return cover

def certified_revision(I, k0, grow_k, seed=0, S_sel=None, kmax=40):
    """discover conflicts locally at radius k0, derive a repair, certify it with ONE global SAT check;
    on failure the global core is shrunk to a missed conflict, added, (optionally k grows) and we repeat."""
    rng = random.Random(seed)
    S, sel = S_sel if S_sel else build_sat(I, range(I.N)); C = Counted(S)
    k = k0 if k0 is not None else 0; found = set(local_found(I, k)) if k0 is not None else set(); rounds = 0; missed = 0
    while True:
        rounds += 1
        rep = minimal_cover(list(found), rng) if found else set()
        kept = [j for j in sel if j not in rep]
        if C.solve([sel[j] for j in kept]):
            return dict(ok=True, rounds=rounds, k=k, sat_calls=C.calls, missed=missed, conflicts=len(found), repair=rep)
        m = mus_from_core(C, sel, kept); missed += 1
        found.add(m)
        if grow_k and k < kmax: k += 1; found |= local_found(I, k)

if __name__ == '__main__':
    t0 = time.time(); rows = []
    for sigma in (1, 2, 3):
        for rho in (0.1, 0.2):
            for seed in range(8):
                I = gen(16, rho, sigma, 7000+seed); full = oracle_pairs(I)
                if not full: continue
                ss = build_sat(I, range(I.N))
                kstar = next(R for R in range(40) if local_found(I, R) >= full)
                row = dict(sigma=sigma, rho=rho, seed=seed, mus=len(full), kstar=kstar)
                for k0 in (0, 1, 2):
                    for g in (False, True):
                        r = certified_revision(I, k0, g, seed, ss)
                        # certified repair must hit every TRUE conflict and be minimal w.r.t. them
                        valid = all(m & r['repair'] for m in full)
                        minimal = all(any(m & (r['repair']-{v}) == frozenset() for m in full) for v in r['repair'])
                        row[f'k{k0}{"g" if g else "f"}'] = dict(rounds=r['rounds'], calls=r['sat_calls'], missed=r['missed'], k=r['k'], valid=valid, minimal=minimal)
                rows.append(row)
    json.dump(rows, open(P('cert.json'), 'w'))
    print(len(rows), 'instances', f'{time.time()-t0:.0f}s')
    print('all certified repairs valid AND globally minimal:', all(r[v]['valid'] and r[v]['minimal'] for r in rows for v in r if v.startswith('k') and v[1:2].isdigit()))
    print('\nvariant        | mean rounds | mean global SAT calls | mean conflicts found by global feedback | mean final k')
    for k0 in (0, 1, 2):
        for g in (False, True):
            key = f'k{k0}{"g" if g else "f"}'
            print(f'k0={k0} {"k grows " if g else "k fixed "} | {st.mean(r[key]["rounds"] for r in rows):11.2f} | {st.mean(r[key]["calls"] for r in rows):21.1f} | {st.mean(r[key]["missed"] for r in rows):38.2f} | {st.mean(r[key]["k"] for r in rows):.1f}')
    print('mean true k* :', f'{st.mean(r["kstar"] for r in rows):.1f}', ' mean #MUS:', f'{st.mean(r["mus"] for r in rows):.0f}')
