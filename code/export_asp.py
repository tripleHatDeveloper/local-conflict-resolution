# Writes the ASP programs and the facts of the real block as plain .lp files in encodings/asp/.
import os
from paths import ROOT
import lifted_enc as L

out = os.path.join(ROOT, 'encodings', 'asp'); os.makedirs(out, exist_ok=True)
M = L.load_real()
names = {'ASP-values': 'values.lp', 'ASP-thresholds': 'thresholds.lp', 'ASP-DL': 'difference_logic.lp'}
for key, fname in names.items():
    prog = L.PRE + L.PROGS[key].replace('%S%', str(M['step'])).replace('%W%', str(M['width']))
    header = f'% {key}: rules only. Facts (dom, est, w, dry, dmin, dmax) are in real_block_facts.lp.\n'
    open(os.path.join(out, fname), 'w').write(header + prog)
open(os.path.join(out, 'real_block_facts.lp'), 'w').write('% Facts of the real flood block, unit weights.\n' + L.facts(M, None))
print('written to', out)
