# HAPI dev notes

## Benchmark

`benchmark/bench_hapi.cpp` — 8 compile-time probes + 1 runtime probe, no external deps.
Probes are independently selectable via `-DTEST_XXX`.

`benchmark/run_bench.sh` — runs all probes at N=10/25/50/100/200, builds runtime binaries
at N=50 and N=200, stores a timestamped log in `benchmark/results/`, and auto-diffs against
the previous run.

```sh
./benchmark/run_bench.sh
```

### First-run results (g++ 13.3, -O2, host: neurux)

| | N=50 | N=200 |
|---|---|---|
| baseline | 88 ms | 83 ms |
| find\_first | 89 ms | 93 ms |
| **node\_only** | **301 ms** | **13 319 ms** |
| forEach | 306 ms | 13 445 ms |
| binary size | 107 KB | 1 111 KB |
| forEach runtime | 2.2 ns/comp | 2.3 ns/comp |
| runEach runtime | 5.5 ns/comp | 7.5 ns/comp |

**The Chain collapse is the story — not the traversal.**
`Map` and `FindFirst` stay near baseline even at N=200 because they never trigger the
inheritance collapse. `node_only` at N=200 costs 13 s vs 83 ms baseline — the deep
`O::Part<Chain<OO...>::Part<API>>` recursion dominates. forEach/runEach add only ~100 ms
on top of that; traversal template overhead is negligible relative to chain construction.
