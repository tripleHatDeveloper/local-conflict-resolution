from paths import P
import sys, json, os, time, math

from lifted_enc import *
import graded, synth
n = int(sys.argv[1]); seeds = int(sys.argv[2]); TL = float(sys.argv[3]); out = f'enc_n{n}.json'
rows = json.load(open(P(out))) if os.path.exists(P(out)) else []
done = {(r['seed'], r['enc'], r['weighted']) for r in rows}
def conflict_graph_opt(I, w):                     # enumerate conflicts (MUS), then optimal hitting set (MaxSAT on conflicts)
    t = time.time(); full = synth.oracle_pairs(I); tc = time.time()-t
    t = time.time(); R = graded.optimal_repair(list(full), {j: w[j] for j in range(len(I.est))}, range(len(I.est))); ts = time.time()-t
    return dict(opt=sum(w[j] for j in R), ground=tc, solve=ts, size=len(full), atoms=len(I.est), complete='OK')
ENCS = ['CNF-order+RC2', 'ASP-values', 'ASP-thresholds', 'ASP-DL', 'SMT-Z3', 'CP-SAT', 'conflict-graph+RC2']
for seed in range(seeds):
    I = graded.gen_two_sources(n, 50000+seed); M = from_synth(I); wc = [graded.weight(graded.p_err(I.sig[j])) for j in range(len(I.est))]
    direct = cnf_direct_size(M)
    for weighted in (False, True):
        w = wc if weighted else None; ww = wc if weighted else [1]*len(I.est)
        for enc in ENCS:
            if (seed, enc, weighted) in done: continue
            if enc == 'ASP-values' and direct > 1_200_000: r = dict(opt=None, size=direct, ground=None, solve=None, atoms=None, complete='SKIPPED (grounding too large)')
            else:
                t = time.time()
                try:
                    if enc == 'CNF-order+RC2': r = run_rc2_order(M, w)
                    elif enc == 'SMT-Z3': r = run_z3(M, w, tl=TL)
                    elif enc == 'CP-SAT': r = run_cpsat(M, w, tl=TL)
                    elif enc == 'conflict-graph+RC2': r = conflict_graph_opt(I, ww)
                    else: r = run_asp(enc, M, w, mode='opt', tl=TL)
                except Exception as e: r = dict(opt=None, size=None, ground=None, solve=None, atoms=None, complete='ERR '+str(e)[:40])
            r = {k: v for k, v in r.items() if k != 'repairs'}; r.update(n=n, seed=seed, enc=enc, weighted=weighted, nest=len(I.est), direct=direct)
            rows.append(r); json.dump(rows, open(P(out), 'w'))
print('n', n, 'rows', len(rows))
