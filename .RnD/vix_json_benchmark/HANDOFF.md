# HAPI × vix.cpp — Opportunity #2: OneParse vs. vix's real JSON backend

LOCAL, not pushed. Real benchmark work itself lives in OneParse's own
repo (`benchmark/`, commit `48bcb6b`, also local/not pushed) — this file
is the cross-reference for the vix integration line of work.

## What changed from the original framing

The handoff doc described this as a "lowest-risk asset swap... benchmarked
drop-in comparison." Reading `vix::json`'s own README
(`HAPIPartners/vix/modules/json/benchmarks/README.md`) before running
anything showed it's not a from-scratch parser to swap out — `vix::json`
wraps `nlohmann::json` directly, and its own documented conclusions
already say "Use `Json` / `nlohmann::json` directly when the main task is
parsing." So the real, honest comparison is OneParse vs. the actual
concrete library vix ships (`nlohmann::json`), not vix's ergonomic
wrapper layer around it.

## What was done

Added `nlohmann::json` as a new comparison arm to OneParse's existing
runtime benchmark harness (`OneParse/benchmark/bench_runtime.cpp`/
`bench.py`/`chart_only.py`) — same shape as the existing RapidJSON/
simdjson branches (full DOM parse, flat single-level key/value walk),
not a new harness. Vendored `nlohmann/json` v3.11.3 (real MIT release
single-header download) into `benchmark/nlohmann_src/`, matching the
existing vendoring convention for `rapidjson_src`/`simdjson_src`/etc.
(gitignored, not tracked, same as the others).

## Real result

`-O2`, g++ 13.3.0, 20000 iterations × 10 runs per fixture, real numbers
(`OneParse/benchmark/results/history.csv`, 2026-08-26 rows):

| parser (MB/s) | small | medium | large  | longstr |
|---------------|------:|-------:|-------:|--------:|
| oneParse      | 325.9 |  444.0 |  628.8 |  1218.4 |
| rapidjson     | 128.9 |  158.0 |  174.0 |   203.9 |
| nlohmann      |  26.8 |   35.1 |   41.1 |    58.4 |

nlohmann is the **slowest parser in the entire comparison set** —
grammar-combinator frameworks included — across every fixture size, by a
wide margin: 10-21x slower than OneParse, and still 3-5x slower than its
nearest general-purpose peer, RapidJSON. This isn't a surprising or
suspicious result — nlohmann's own documented tradeoff (ergonomic
variant-based API, exception-heavy conversion paths, DOM-allocation-heavy
construction) is widely known in the C++ community to trail
allocation-lean parsers; it's consistent with, not contradicting, vix's
own README already steering users toward "use nlohmann::json directly"
for the hot path and only reaching for the ergonomic wrapper layer when
readability matters more than throughput.

Chart: `OneParse/benchmark/bench_comparison.png` (regenerated, includes
the new teal-hatched nlohmann bar, labeled "vix.cpp's backend" directly
in the legend). **Read the "Honest scope" section below before quoting
these numbers anywhere** — the comparison isn't fully apples-to-apples.

## Honest scope

This is a raw parse-throughput comparison, not a vix integration — no
vix code was touched, no `vix::json` API was exercised (only the
`nlohmann::json` library it wraps, directly). It answers "is there a
real performance gap worth a pitch," not "does OneParse slot into
`vix::json`'s actual call sites" — that would need `vix::json`'s public
API surface (`json::o()`, `jget()`/`jset()`, the Simple model) either
reimplemented on OneParse or left as-is with OneParse offered as a
faster opt-in path for the hot parse/serialize case specifically,
matching vix's own README's already-stated guidance. Not attempted this
round — flagging as the natural next step if this opportunity moves
toward a real PR.

**A second, more important scope caveat, found on review and confirmed
by reading the actual benchmarked code path**: the `"oneParse"` row above
is `PARSER_ONEPARSE_INDEX` (`bench_runtime.cpp`), which emits
`Arr<Pair<std::string_view,std::string_view>,8>` — key **and value** are
raw spans into the original buffer, confirmed by the extractor's own
comment ("emits a std::string_view span for every key and every value").
No numeric conversion happens anywhere in that path (no `strtod`/
`from_chars`/`atof` anywhere in OneParse's `include/`, confirmed by grep)
— a JSON number stays exactly as its source text. `nlohmann::json::parse`
does materially more work per field: it converts numbers to typed
`double`/`int64_t`, unescapes and owns strings, and builds a real DOM
tree. So this table is **not** an apples-to-apples "who parses JSON
faster" result — it's closer to "OneParse's structural span-extraction"
vs. "nlohmann's structural extraction *plus* full value materialization."
The numbers themselves are real and correctly measured; the claim they
support is narrower than the table's framing implies. Don't quote this
as "OneParse is 10-21x faster than vix's JSON backend" without this
caveat attached — that overstates it. The honest fix is either scoping
the write-up to the narrower claim explicitly (done here, now) or adding
real numeric conversion to OneParse's benchmarked path before re-running
— deliberately deferred as a separate piece of work, not done this round.

## Next

Per the outreach doc's own sequencing: opportunity #3 (middleware) next,
now that #1 and #2 both have real, verified evidence.
