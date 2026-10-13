import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'code'))
from paths import P, OUT
import matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyArrowPatch
plt.rcParams.update({'pdf.fonttype': 42, 'font.family': 'serif', 'font.size': 8})
true = [9, 8, 7, 6, 5, 4, 3, 2]; est = {1: 9, 2: 8, 5: 9, 8: 2}          # parcel -> estimated level; P5's estimate is too high
BLUE, RED, GREY = '#1f4e79', '#b03a2e', '#9aa5b1'
fig, axs = plt.subplots(3, 1, figsize=(7.0, 6.4), sharex=True)
titles = ['(a) A chain of parcels; water flows from left to right, so levels must not increase downstream',
          '(b) A cover of thickness 1 around parcel 3 sees parcels 2 to 4: the conflict is not visible',
          '(c) A cover of thickness 2 around parcel 3 sees parcels 1 to 5: the conflict is found']
for ax, t in zip(axs, titles):
    ax.set_xlim(0.2, 8.8); ax.set_ylim(-1.6, 14.8); ax.axis('off'); ax.set_title(t, loc='left', fontsize=8, pad=4)
    for i, h in enumerate(true, start=1):
        ax.add_patch(Rectangle((i-0.34, 0), 0.68, h, fc='#e8eef4', ec=GREY, lw=.8)); ax.text(i, -1.3, f'P{i}', ha='center', fontsize=7.5)
    for i in range(1, 8): ax.add_patch(FancyArrowPatch((i+0.36, 13.7), (i+0.64, 13.7), arrowstyle='-|>', mutation_scale=7, lw=.8, color=GREY))
    for i, v in est.items(): ax.plot([i-0.3, i+0.3], [v, v], color=RED if i in (2, 5) else BLUE, lw=2.2); ax.text(i, v+0.35, f'{v}', ha='center', fontsize=7, color=RED if i in (2, 5) else BLUE)
ax = axs[0]; ax.annotate('', xy=(5, 10.1), xytext=(2, 9.1), arrowprops=dict(arrowstyle='<->', color=RED, lw=1.1, ls='--', connectionstyle='arc3,rad=-0.18'))
ax.text(3.5, 12.35, 'estimates at P2 (8) and P5 (9) cannot both hold: conflict spanning 3 steps', ha='center', color=RED, fontsize=7.2)
for ax, (lo, hi), col, lab in ((axs[1], (2, 4), BLUE, 'cover, $k=1$'), (axs[2], (1, 5), '#1e8449', 'cover, $k=2$')):
    ax.add_patch(Rectangle((lo-0.45, -0.4), hi-lo+0.9, 11.2, fc='none', ec=col, lw=1.6, ls='-')); ax.text((lo+hi)/2, 11.05, lab, ha='center', fontsize=7.5, color=col)
axs[1].text(6.9, 6.5, 'P5 lies outside the cover:\nthe chain P2--P5 is cut', fontsize=7.2, color=RED, ha='center')
axs[2].text(6.9, 6.5, 'parcels 6 to 8 are not needed\nto see this conflict', fontsize=7.2, color='#444', ha='center')
plt.tight_layout(h_pad=0.4); plt.savefig(OUT('fig_locality.pdf'), bbox_inches='tight'); plt.close()
