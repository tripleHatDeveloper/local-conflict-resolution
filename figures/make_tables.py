import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'code'))
from paths import P, OUT
import json, glob, statistics as st
E = []
for f in sorted(glob.glob(P('enc_n*.json'))): E += json.load(open(f))
ENCS = [('CNF-direct', None), ('CNF-order + RC2', 'CNF-order+RC2'), ('ASP, one atom per value', 'ASP-values'), ('ASP, threshold atoms', 'ASP-thresholds'), ('ASP + difference logic', 'ASP-DL'), ('SMT (Z3)', 'SMT-Z3'), ('CP-SAT', 'CP-SAT'), ('Conflict enumeration + MaxSAT', 'conflict-graph+RC2')]
ns = sorted({r['n'] for r in E})
def sz(x):
    if x is None: return '--'
    return f'{x/1e6:.1f}M' if x >= 1e6 else (f'{x/1e3:.0f}k' if x >= 1e4 else f'{x:.0f}')
def tm(x): return '--' if x is None else (f'{x:.3f}' if x < 0.01 else f'{x:.2f}' if x < 10 else f'{x:.1f}')
def row(label, key, weighted, what):
    cells = []
    for n in ns:
        if key is None:
            cells.append(sz(st.median(r['direct'] for r in E if r['n'] == n)) if what == 'size' else '--'); continue
        g = [r for r in E if r['n'] == n and r['enc'] == key and r['weighted'] == weighted]
        fin = [r for r in g if r.get('opt') is not None and r.get('complete') not in ('TIMEOUT', 'unknown') and not str(r.get('complete', '')).startswith('SKIP')]
        if what == 'size': cells.append(sz(st.median(r['size'] for r in g if r.get('size') is not None)) if any(r.get('size') is not None for r in g) else '--')
        else:
            if fin: cells.append(tm(st.median((r['ground'] or 0)+(r['solve'] or 0) for r in fin)) + ('' if len(fin) == len(g) else f'$^{{{len(fin)}/{len(g)}}}$'))
            else: cells.append('n/s' if g and str(g[0].get('complete', '')).startswith('SKIP') else 't/o')
    return f'{label} & ' + ' & '.join(cells) + r' \\'
hdr = ' & '.join(f'$n={n}$' for n in ns)
out = [r'\begin{tabular}{l' + 'r'*len(ns) + '}', r'\toprule', f'Encoding & {hdr} \\\\', r'\midrule', r'\multicolumn{%d}{l}{\emph{Encoding size (clauses or ground rules, median)}}\\' % (len(ns)+1)]
out += [row(l, k, False, 'size') for l, k in ENCS]
out += [r'\midrule', r'\multicolumn{%d}{l}{\emph{Time to a proven optimum, min-cardinality repair (s, build + solve, median)}}\\' % (len(ns)+1)]
out += [row(l, k, False, 'time') for l, k in ENCS[1:]]
out += [r'\midrule', r'\multicolumn{%d}{l}{\emph{Time to a proven optimum, weighted repair with contextual reliabilities (s)}}\\' % (len(ns)+1)]
out += [row(l, k, True, 'time') for l, k in ENCS[1:]]
out += [r'\bottomrule', r'\end{tabular}']
open(OUT('tab_scaling.tex'), 'w').write('\n'.join(out))
nest = {n: st.median(r['nest'] for r in E if r['n'] == n) for n in ns}; print(nest); print('\n'.join(out))
