from paths import P
import json, statistics as st
names = None
for gaps, label in ((0, '3 separate basins (no pass: LS can split)'), (1, 'basins joined by ONE pass per wall'), (3, 'basins joined by 3 passes per wall')):
    rows = json.load(open(P(f'exp4_g{gaps}.json'))); names = [k for k in rows[0] if k not in ('seed', 'mus')]
    print(f'\n=== {label}  ({len(rows)} instances, median #MUS {st.median(r["mus"] for r in rows):.0f})')
    print(f'{"structure":34} | max part | #parts | k-partition | recall | exact | valid-repair share')
    for nm in names:
        g = [r[nm] for r in rows]
        print(f'{nm:34} | {st.mean(x["maxpart"] for x in g):8.0f} | {st.mean(x["nparts"] for x in g):6.0f} | {st.mean(x["mult"] for x in g):11.1f} | {st.mean(x["recall"] for x in g):.3f}  | {100*sum(x["exact"] for x in g)/len(g):4.0f}% | {st.mean(x["valid"] for x in g):.2f}')
