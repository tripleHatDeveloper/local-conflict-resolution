from paths import P
import sys, time, multiprocessing as mp
from solvers_maxsat import *
def worker(q, which, conf, w):
    q.put(run_cpsat(conf, w) if which == 'cpsat' else run_z3(conf, w))
insts = [build(30000+s) for s in range(3)]
for which in ('cpsat', 'z3'):
    out = []
    for I, conf, w in insts:
        q = mp.Queue(); p = mp.Process(target=worker, args=(q, which, conf, w)); p.start(); p.join(40)
        if p.is_alive(): p.terminate(); out.append(('TIMEOUT 40s', None))
        else: out.append(q.get())
    print(which, out, flush=True)
