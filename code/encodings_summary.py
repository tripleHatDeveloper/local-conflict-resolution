from paths import P
import json, glob, collections, statistics as st
rows = []
for f in sorted(glob.glob(P('enc_n*.json'))): rows += json.load(open(f))
ENCS = ['CNF-order+RC2', 'ASP-values', 'ASP-thresholds', 'ASP-DL', 'SMT-Z3', 'CP-SAT', 'conflict-graph+RC2']
def fmt(x, p=2): return '   -  ' if x is None else (f'{x:.{p}f}' if isinstance(x, float) else str(x))
for weighted in (False, True):
    print('\n=== ' + ('WEIGHTED repair (contextual reliabilities)' if weighted else 'MIN-CARDINALITY repair') + '  [median over instances: size | build s | solve s | proven/finished]')
    ns = sorted({r['n'] for r in rows})
    print(f'{"encoding":20}' + ''.join(f' | n={n:<2} ({st.median(r["nest"] for r in rows if r["n"]==n):.0f} est)               ' for n in ns))
    for e in ENCS:
        cells = []
        for n in ns:
            g = [r for r in rows if r['n'] == n and r['enc'] == e and r['weighted'] == weighted]
            if not g: cells.append(' ' * 36); continue
            fin = [r for r in g if r.get('opt') is not None and r.get('complete') not in ('TIMEOUT',) and not str(r.get('complete', '')).startswith(('SKIP', 'ERR', 'UNKNOWN'))]
            sz = st.median(r['size'] for r in g if r.get('size') is not None) if any(r.get('size') is not None for r in g) else None
            if fin: cells.append(f' {fmt(sz,0):>8} | {st.median(r["ground"] or 0 for r in fin):5.2f} | {st.median(r["solve"] or 0 for r in fin):6.2f} | {len(fin)}/{len(g)}  ')
            else: cells.append(f' {fmt(sz,0):>8} |   -   |   -    | 0/{len(g)} ({g[0].get("complete")})'[:36].ljust(36))
        print(f'{e:20} |' + ' |'.join(cells))
# agreement of optimum
g = collections.defaultdict(dict)
for r in rows:
    if r.get('opt') is not None and r.get('complete') not in ('TIMEOUT',) and not str(r.get('complete', '')).startswith(('SKIP', 'ERR', 'UNKNOWN')): g[(r['n'], r['seed'], r['weighted'])][r['enc']] = r['opt']
ok = sum(1 for v in g.values() if len(set(v.values())) == 1)
print(f'\noptimum agreement: {ok}/{len(g)} (instance, objective) cases: all encodings that finished (proven optimum) agree')
for k, v in g.items():
    if len(set(v.values())) != 1: print('  DISAGREE', k, v)
print('direct CNF size (clauses):', {n: f"{st.median(r['direct'] for r in rows if r['n']==n):,.0f}" for n in sorted({r['n'] for r in rows})})
