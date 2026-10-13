from paths import P
# Work / critical-path simulation behind the "7.6 to 7.8 times on 8 workers" figure (real flood block, atomic parts).
# Per-part conflict-enumeration times are measured sequentially; schedules are simulated (greedy longest-task-first), no communication cost.
import time, heapq
src = open('real_k.py').read(); exec(src[:src.index("d = json.load(open(P('marco.json')))")])
def makespan(times, p):
    w = [0.0]*p; heapq.heapify(w)
    for t in sorted(times, reverse=True): heapq.heappush(w, heapq.heappop(w)+t)
    return max(w)
for k in (1, 2, 3):
    parts = sorted({frozenset(ball(p, k)) for p in parcels}, key=lambda s: sorted(s)); times = []
    for pt in parts:
        t = time.perf_counter(); part_muses(set(pt)); times.append(time.perf_counter()-t)
    W = sum(times)
    print(f'k={k}: {len(parts)} parts, total {W*1000:.0f} ms, largest part {100*max(times)/W:.0f}% of work, simulated speedup:',
          {p: round(W/makespan(times, p), 2) for p in (2, 4, 8, 16, 47)})
