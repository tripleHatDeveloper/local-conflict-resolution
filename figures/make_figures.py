import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'code'))
from paths import P, OUT
import os, sys, json, re, glob, collections, statistics as st
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'code'))
import matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt, networkx as nx
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
plt.rcParams.update({'pdf.fonttype': 42, 'font.family': 'serif', 'font.size': 9, 'axes.spines.top': False, 'axes.spines.right': False, 'figure.dpi': 150})
C = dict(blue='#1f4e79', red='#b03a2e', orange='#d68910', green='#1e8449', grey='#7f8c8d', purple='#6c3483', teal='#117a8b')
# ---------------------------------------------------------------- Fig 2: real block graph coloured by verdict
M = json.load(open(P('lifted_model.json'))); V = json.load(open(P('verdicts.json')))
rank = {'safe (in no conflict)': 0, 'suspect (only in non-minimum minimal repairs)': 1, 'ambiguous (in some minimum-loss repairs)': 2, 'certainly erroneous (in every minimum-loss repair)': 3}
pv = collections.defaultdict(int); pe = collections.defaultdict(int)
for j, (p, k, v) in enumerate(M['est']): pv[p] = max(pv[p], rank[V[str(j)]]); pe[p] += 1
G = nx.Graph(); 
for a, b, l in M['edges']: G.add_edge(a, b)
d = json.load(open(P('marco.json'))); conf = [tuple(sorted({M['est'][c-54776][0] for c in m})) for m in d['muses']]
pos = nx.kamada_kawai_layout(G)
fig, ax = plt.subplots(figsize=(6.6, 4.4)); col = {0: '#d5dbdb', 1: C['orange'], 2: '#e59866', 3: C['red']}
nx.draw_networkx_edges(G, pos, ax=ax, edge_color='#b2babb', width=0.7)
for (a, b) in {c for c in conf if len(c) == 2}: nx.draw_networkx_edges(G, pos, edgelist=[(a, b)] if G.has_edge(a, b) else [], ax=ax, edge_color=C['red'], width=2.2)
long = [c for c in conf if len(c) == 2 and not G.has_edge(*c)]
for a, b in long: ax.plot([pos[a][0], pos[b][0]], [pos[a][1], pos[b][1]], ls=(0, (4, 3)), color=C['red'], lw=1.6, alpha=.9)
nx.draw_networkx_nodes(G, pos, node_color=[col[pv.get(n, 0)] for n in G.nodes], node_size=[260 if pv.get(n, 0) else 70 for n in G.nodes], edgecolors='#566573', linewidths=.6, ax=ax)
nx.draw_networkx_labels(G, pos, labels={n: n[1:] for n in G.nodes if pv.get(n, 0)}, font_size=6.5, ax=ax)
ax.axis('off')
from matplotlib.lines import Line2D
ax.legend(handles=[Line2D([0], [0], marker='o', ls='', mfc=col[0], mec='#566573', label='all estimates safe'), Line2D([0], [0], marker='o', ls='', mfc=col[1], mec='#566573', label='suspect'), Line2D([0], [0], marker='o', ls='', mfc=col[2], mec='#566573', label='ambiguous'), Line2D([0], [0], marker='o', ls='', mfc=col[3], mec='#566573', label='necessarily removed'),
                   Line2D([0], [0], color=C['red'], lw=2.2, label='conflict between adjacent parcels'), Line2D([0], [0], color=C['red'], ls='--', lw=1.6, label='conflict across 2--3 steps')], loc='upper center', bbox_to_anchor=(0.5, 0.0), ncol=3, fontsize=7, frameon=False)
plt.savefig(OUT('fig2_block.pdf'), bbox_inches='tight'); plt.close()
# ---------------------------------------------------------------- Fig 3: recall vs k and validity vs k
from synth import gen, oracle_pairs, ball
import random
def curve(sigma, seeds=14):
    acc = [[] for _ in range(13)]
    for s in range(seeds):
        I = gen(16, 0.2, sigma, 3000+s); full = oracle_pairs(I)
        if not full: continue
        found = set()
        for R in range(13):
            for c in range(I.N): found |= oracle_pairs(I, ball(I, c, R).keys())
            acc[R].append(len(found & full)/len(full))
    return [st.mean(a) for a in acc]
cur = {s: curve(s) for s in (1, 2, 3, 4)}
json.dump(cur, open(OUT('recall_curves.json'), 'w'))
val = {1: [0, .22, .62, .89, .99, 1, 1, 1, 1, 1, 1], 2: [0, .04, .17, .50, .81, .96, 1, 1, 1, 1, 1], 3: [0, .04, .04, .19, .57, .81, .99, 1, 1, 1, 1], 4: [0, 0, .01, .06, .45, .63, .90, .97, .99, 1, 1]}
fig, axs = plt.subplots(1, 2, figsize=(7.2, 2.7)); cs = [C['blue'], C['green'], C['orange'], C['red']]
for (s, y), c in zip(cur.items(), cs): axs[0].plot(range(13), y, '-o', ms=3, color=c, label=f'$\\sigma={s}$')
axs[0].set_xlabel('cover thickness $k$'); axs[0].set_ylabel('share of true conflicts found'); axs[0].set_ylim(0, 1.02); axs[0].legend(frameon=False, fontsize=7); axs[0].set_title('(a) conflict recall of local parts', fontsize=8)
for (s, y), c in zip(val.items(), cs): axs[1].plot(range(len(y)), y, '-s', ms=3, color=c, label=f'$\\sigma={s}$')
axs[1].set_xlabel('cover thickness $k$'); axs[1].set_ylabel('share of valid removal sets'); axs[1].set_ylim(0, 1.02); axs[1].set_title('(b) validity of removal sets built locally', fontsize=8)
plt.tight_layout(); plt.savefig(OUT('fig3_recall_validity.pdf'), bbox_inches='tight'); plt.close()
# ---------------------------------------------------------------- Fig 4: structures comparison
rows = json.load(open(P('exp4_g1.json'))); names = [k for k in rows[0] if k not in ('seed', 'mus')]
fig, ax = plt.subplots(figsize=(5.6, 3.5))
style = {'Global': ('s', C['grey']), 'LS': ('D', C['purple']), 'B': ('^', C['orange']), 'C': ('o', C['blue'])}
short = {'Global (one part)': 'Global', 'LS: Parikh finest splitting': 'Global = LS (one pass)', 'B: tiles 6, margin 0 (partition)': 'B 6/0', 'B: tiles 4, margin 2': 'B 4/2', 'B: tiles 6, margin 3': 'B 6/3',
         'C: r=0 k=3': 'C 0/3', 'C: r=0 k=5': 'C 0/5', 'C: r=0 k=8': 'C 0/8', 'C: r=2 k=5': 'C 2/5'}
for nm in names:
    g = [r[nm] for r in rows]; x = st.mean(a['nparts']*a['meanpart'] for a in g); y = st.mean(a['exact'] for a in g)*100
    key = 'Global' if nm.startswith('Global') else nm.split(':')[0].split(' ')[0]
    m, c = style[key]; ax.scatter(x, y, marker=m, s=46, color=c, zorder=3)
    if nm.startswith('Global'): continue
    ax.annotate(short[nm], (x, y), textcoords='offset points', xytext=(5, 4 if 'C 2/5' not in short[nm] else -10), fontsize=7)
ax.set_xscale('log'); ax.set_xlabel('total work (sum of part sizes, parcels)'); ax.set_ylabel('instances solved exactly (%)'); ax.set_ylim(-5, 108)
ax.scatter([], [], marker='D', color=C['purple'], label='LS (Parikh)'); ax.scatter([], [], marker='^', color=C['orange'], label='B-structure (tiles/margin)'); ax.scatter([], [], marker='o', color=C['blue'], label='C-structure ($r$/$k$)'); ax.scatter([], [], marker='s', color=C['grey'], label='global')
ax.legend(frameon=False, fontsize=7, loc='lower right')
plt.savefig(OUT('fig4_structures.pdf'), bbox_inches='tight'); plt.close()
# ---------------------------------------------------------------- Fig 5: encodings vs n
E = []
for f in sorted(glob.glob(P('enc_n*.json'))): E += json.load(open(f))
ENCS = ['CNF-order+RC2', 'ASP-values', 'ASP-thresholds', 'ASP-DL', 'SMT-Z3', 'CP-SAT', 'conflict-graph+RC2']
col = dict(zip(ENCS, [C['blue'], C['red'], C['orange'], C['green'], C['purple'], C['teal'], C['grey']])); mk = dict(zip(ENCS, 'osd^vP*'))
fig, axs = plt.subplots(1, 2, figsize=(7.2, 2.9)); ns = sorted({r['n'] for r in E})
dirsz = [st.median(r['direct'] for r in E if r['n'] == n) for n in ns]; axs[0].plot(ns, dirsz, 'k--', lw=1, label='CNF-direct')
for e in ENCS:
    xs, ys, xt, yt = [], [], [], []
    for n in ns:
        g = [r for r in E if r['n'] == n and r['enc'] == e and not r['weighted'] and r.get('size') is not None]
        if g: xs.append(n); ys.append(st.median(r['size'] for r in g))
        f = [r for r in E if r['n'] == n and r['enc'] == e and not r['weighted'] and r.get('opt') is not None and r.get('complete') not in ('TIMEOUT', 'unknown')]
        if f: xt.append(n); yt.append(max(1e-3, st.median((r['ground'] or 0)+(r['solve'] or 0) for r in f)))
    axs[0].plot(xs, ys, '-'+mk[e], ms=3.5, color=col[e], label={'conflict-graph+RC2': 'conflict enumeration + MaxSAT', 'CNF-order+RC2': 'CNF-order + RC2'}.get(e, e)); axs[1].plot(xt, yt, '-'+mk[e], ms=3.5, color=col[e])
axs[0].set_yscale('log'); axs[0].set_xscale('log'); axs[1].set_yscale('log'); axs[1].set_xscale('log')
axs[0].set_xlabel('grid side $n$ ($n^2$ parcels)'); axs[0].set_ylabel('encoding size (clauses / ground rules)'); axs[0].set_title('(a) size', fontsize=8)
axs[1].set_xlabel('grid side $n$'); axs[1].set_ylabel('build + solve time (s), min-cardinality'); axs[1].set_title('(b) time to proven optimum', fontsize=8)
from matplotlib.ticker import NullFormatter
for a in axs: a.set_xticks(ns); a.set_xticklabels(ns); a.xaxis.set_minor_formatter(NullFormatter()); a.minorticks_off(); a.set_xticks(ns)
h, l = axs[0].get_legend_handles_labels(); fig.legend(h, l, loc='lower center', ncol=4, fontsize=6.5, frameon=False, bbox_to_anchor=(0.5, -0.09)); plt.tight_layout(); plt.savefig(OUT('fig5_encodings.pdf'), bbox_inches='tight'); plt.close()
print('figures done')
