import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'code'))
from paths import P, OUT
import matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
plt.rcParams.update({'pdf.fonttype': 42, 'font.family': 'serif', 'font.size': 8})
C = dict(blue='#1f4e79', red='#b03a2e', orange='#d68910', green='#1e8449', grey='#7f8c8d', purple='#6c3483', teal='#117a8b')
fig, ax = plt.subplots(figsize=(7.4, 3.5)); ax.axis('off'); ax.set_xlim(0, 173); ax.set_ylim(0, 48)
def box(x, y, w, h, t, fc, ec, fs=7.4):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle='round,pad=0.4,rounding_size=1.2', fc=fc, ec=ec, lw=1.1, zorder=3))
    ax.text(x + w/2, y + h/2, t, ha='center', va='center', fontsize=fs, zorder=4)
def arr(x1, y1, x2, y2, c='#444', lw=1.2):
    ax.add_patch(FancyArrowPatch((x1, y1), (x2, y2), arrowstyle='-|>', mutation_scale=10, lw=lw, color=c, zorder=2))
for t, y in (('Source 1\nestimates', 33), ('Source 2\nestimates', 22), ('Source $h$\nestimates', 11)):
    box(1, y, 18, 8, t, '#eaf2f8', C['blue'], 7.2); arr(19.8, y + 4, 23.4, 25)
box(24, 14, 23, 22, 'Encoding\nconstraints $\\Sigma$,\nestimates $E$,\nweights $w$', '#fef9e7', C['orange'], 7.0)
arr(47.8, 25, 52.6, 27)
ax.add_patch(FancyBboxPatch((50, 3), 91, 38, boxstyle='round,pad=0.3,rounding_size=1.5', fc='#fbfbfb', ec='#aab7b8', lw=1.0, ls='--', zorder=1))
ax.text(95.5, 38.2, 'Local resolution with global verification', ha='center', fontsize=7.4, style='italic', color='#566573')
box(53, 20, 22, 14, 'Local conflict\ndiscovery in\ncovers of\nthickness $k$', '#e8f6f3', C['teal'])
box(84, 20, 22, 14, 'Minimal-loss\nresolution:\nhitting set or\nMaxSAT', '#f5eef8', C['purple'])
box(115, 20, 22, 14, 'Global\nverification:\n$\\Sigma\\cup(E\\setminus R)$\nconsistent?', '#fdedec', C['red'])
arr(75.8, 27, 83.2, 27); ax.text(79.5, 29.3, '$\\mathcal{F}$', ha='center', fontsize=8.5)
arr(106.8, 27, 114.2, 27); ax.text(110.5, 29.3, '$R$', ha='center', fontsize=8.5)
ax.plot([126, 126], [19.4, 11], color=C['red'], lw=1.6, zorder=2); ax.plot([126, 95], [11, 11], color=C['red'], lw=1.6, zorder=2)
arr(95, 11, 95, 19.4, c=C['red'], lw=1.6)
ax.text(110.5, 7.0, 'no: a conflict was missed;\nadd it to $\\mathcal{F}$ and repeat', ha='center', va='center', fontsize=7.4, color=C['red'], style='italic')
arr(137.8, 27, 146.6, 27, c=C['green'], lw=1.6); ax.text(142.2, 29.3, 'yes', ha='center', fontsize=7.8, color=C['green'], weight='bold')
box(147.5, 20, 24, 14, 'Resolved base\nand verdicts', '#eafaf1', C['green'], 7.2)
ax.text(85, 45.5, 'uncertain, heterogeneous data  $\\Rightarrow$  one consistent base with provenance', fontsize=8.2, ha='center', weight='bold')
plt.savefig(OUT('fig1_pipeline.pdf'), bbox_inches='tight'); plt.close()
