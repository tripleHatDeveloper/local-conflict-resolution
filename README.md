# local-conflict-resolution

Data, encodings and code for resolving conflicts in multi-source data by searching for conflicts in small neighbourhoods and checking the result with one global consistency check.

The real case is a flood block from the Hérault valley (47 parcels, 70 water-level estimates, 54,845 clauses). The other experiments use a generator of synthetic grids with flow constraints.

## Layout

```
data/flood_block/     clause files and adjacency list of the real block
model/                lifted model (difference constraints) built from the clause files
encodings/asp/        ASP programs (values, thresholds, difference logic) and the facts of the real block
code/                 Python modules and scripts
c-implementation/     C program for local resolution with Reiter's hitting-set algorithm
results/              outputs of the experiments (json)
figures/              scripts for the figures and tables, and the generated files in figures/output/
```

## Data

`data/flood_block/` holds the four input files of the real block.

| file | content |
|---|---|
| `clauses-bloc1_zeta` | propositional clauses (first line: `cnf <variables> <clauses>`), one clause per line, preceded by its length |
| `clauses-bloc1_zeta.set` | membership of the clauses; clauses 54776 to 54845 are the 70 estimates, the others are constraints |
| `clauses-bloc1_zeta.corresp` | variable names, e.g. `C223p_640` for "the maximum level of parcel 223 is 640", with their numbers |
| `voisinage1` | adjacency list: `(parcel, parcel, label);` |

`model/lifted_model.json` is derived from these files by `code/lift_model.py`. It holds the domains of the 92 (parcel, attribute) pairs, the 158 difference constraints between adjacent parcels, the 33 parcels with the "dry" exception and the 70 estimates, and it regenerates the 54,683 binary clauses of the clause file exactly.

## Setup

Python 3.10 or later.

```
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Tested with python-sat 1.9, clingo 5.8, clingo-dl 1.5, ortools 9.15, z3-solver 5.1, networkx 3.6 and matplotlib 3.10.

All scripts are run from the repository root and find their files through `code/paths.py`.

## Real block

```
python code/lift_model.py             # model/lifted_model.json, with a check against the clause file
python code/marco.py                  # results/marco.json: all minimal conflicts and minimal removal sets
python code/verdicts.py               # results/verdicts.json: classification of the 70 estimates
python code/local_discovery_real.py   # conflicts found in local parts for increasing thickness
python code/verification_real.py      # local discovery followed by the global check, on random starting points
```

`marco.py` finds 12 minimal conflicts and 144 minimal removal sets (72 of size 8, 72 of size 10).

## Encodings

`code/lifted_enc.py` builds the encodings of the lifted model: order encoding in SAT, three ASP programs, SMT (Z3) and CP-SAT. The ASP programs are also available as plain files in `encodings/asp/` (rules in `values.lp`, `thresholds.lp` and `difference_logic.lp`, facts in `real_block_facts.lp`); `code/export_asp.py` writes them. The difference-logic program needs clingo-dl.

```
python code/encodings_real.py                          # size, time and solutions of every encoding on the real block
python code/encodings_scaling.py N SEEDS TIME_LIMIT    # synthetic grids of side N, writes results/enc_nN.json
python code/encodings_summary.py                       # summary of results/enc_n*.json
```

## Synthetic grids

`code/synth.py` generates grids of parcels with a smooth terrain, noisy estimates and a closed-form conflict generator. `code/structures.py` builds language splittings, B-structures and C-structures on them, and `code/certify.py` contains local discovery followed by the global check.

```
python code/confinement.py                    # smallest thickness that finds every conflict (16x16 grids)
python code/confinement_large.py N SIGMA      # same on larger grids
python code/validity.py [SIGMAS]              # share of valid removal sets built from locally found conflicts
python code/compare_structures.py GAPS        # language splitting, B- and C-structures with GAPS passages between basins
python code/incremental.py                    # resolution around one new estimate
python code/decomposition.py                  # independent conflict components
python code/verification_calls.py             # global SAT calls with and without local discovery
python code/marco_vs_verification.py          # global MARCO compared with the local loop
python code/reliability.py                    # fixed priorities against context-dependent weights
python code/work_span.py                      # per-part work and simulated speed-up on the real block
```

SAT solver comparison:

```
python code/solvers_local.py B cadical195,glucose4,...     # local enumeration on the parts of a C-structure
python code/solvers_global.py cadical195,glucose4,...      # global MARCO with a 25 s cap
python code/solvers_maxsat.py                              # weighted resolution with RC2, Z3 and CP-SAT
python code/solvers_maxsat_timeouts.py                     # the same with a hard time limit
```

Solver names are those of PySAT (`cadical195`, `cadical153`, `glucose3`, `glucose4`, `glucose42`, `lingeling`, `maplechrono`, `maplecm`, `maplesat`, `mergesat3`, `minicard`, `minisat22`, `minisat-gh`).

Most of these runs take minutes and some take much longer; the outputs computed here are in `results/`, and `summarize_confinement.py` and `summarize_structures.py` print summaries of them.

## C program

`c-implementation/` holds the C sources of the program that computes minimal removal sets with Reiter's hitting-set algorithm on local parts of the space. It reads the clause files of `data/flood_block/` and the two input files `graphe.don` (adjacency) and `parcelles.don` (parcel identifiers) kept next to the sources.

```
cd c-implementation
make
echo "0 1" | ./espace -t ../data/flood_block/clauses-bloc1_zeta
```

The core radius `r` and the thickness `k` of the covers are read from standard input. The program prints the number of parts, the minimal removal sets found and the computing time. Small settings such as `0 0` or `0 1` finish within about a minute; larger ones can run for hours.

## Figures and tables

```
python figures/fig_pipeline.py     # overview diagram
python figures/fig_locality.py     # small example of a conflict seen through covers of two thicknesses
python figures/make_figures.py     # block graph, recall and validity curves, structures, encodings
python figures/make_tables.py      # scaling table of the encodings
```

The generated files are written to `figures/output/`.

## License

Code: MIT (see `LICENSE`).
Data, model and results: CC BY 4.0 (see `LICENSE-DATA`).
