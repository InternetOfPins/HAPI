"""
HAPIFold + Find benchmark chart — 2×2 layout
  [0,0] Compile: Transform  (bench_fold.cpp)
  [0,1] Compile: Find       (main.cpp, HAPI + Hana)
  [1,0] Runtime: Transform  (bench_fold.cpp FOLD_RUNTIME)
  [1,1] Runtime: Ref cliff  (bench_fold.cpp FOLD_RUNTIME, fine-grained N)
"""

import os, time, subprocess, tempfile
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

BENCH_DIR    = os.path.dirname(os.path.abspath(__file__))
SRC_FOLD     = os.path.join(BENCH_DIR, "bench_fold.cpp")
SRC_FIND     = os.path.join(BENCH_DIR, "main.cpp")
INCLUDE_DIR  = os.path.join(BENCH_DIR, "..", "include")

CXX          = os.environ.get("CXX", "g++")

FLAGS_FOLD   = [CXX, "-std=c++20", "-ftemplate-depth=2000",
                f"-I{INCLUDE_DIR}", "-I/usr/include"]
FLAGS_FIND   = [CXX, "-std=c++20", "-ftemplate-depth=2000",
                f"-I{INCLUDE_DIR}", "-I/usr/include"]

COMPILE_SIZES_TRANSFORM = [10, 25, 50, 100, 200]
COMPILE_SIZES_FIND      = [10, 50, 100, 200, 300]
RUNTIME_SIZES_TRANSFORM = [10, 25, 50]
RUNTIME_SIZES_REF_CLIFF = list(range(5, 55, 5))   # 5,10,15,...,50

REPS = 1_000_000


# ── helpers ──────────────────────────────────────────────────────────────────

def compile_ms(flags, src, n, define, size_flag="-DTEST_SIZE"):
    cmd = flags + ["-fsyntax-only", "-O0",
                   f"{size_flag}={n}", f"-D{define}", src]
    t0 = time.time()
    r  = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    ms = (time.time() - t0) * 1000
    if r.returncode != 0:
        print(f"  ERR {define} N={n}: {r.stderr.decode()[:80]}")
        return None
    print(f"  {define:25s} N={n:4d}: {ms:7.0f} ms")
    return ms


def runtime_ns(n, tmpdir, extra_sizes=False):
    """Run FOLD_RUNTIME binary for N, return {label: ns_per_call}."""
    exe = os.path.join(tmpdir, f"fold_rt_{n}")
    cmd = FLAGS_FOLD + ["-O2", f"-DTEST_SIZE={n}", f"-DREPS={REPS}",
                        "-DTEST_FOLD_RUNTIME", "-o", exe, SRC_FOLD]
    r = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    if r.returncode != 0:
        print(f"  ERR runtime N={n}: {r.stderr.decode()[:80]}")
        return {}
    out = subprocess.run([exe], capture_output=True, text=True).stdout
    res = {}
    for line in out.splitlines():
        for key in ("manual", "mutate", "fn_ptr", "ref", "ctref", "trans", "hana", "hana_fn"):
            parts = line.strip().split()
            if parts and parts[0] == key:
                try:
                    res[key] = float(parts[1])
                except (IndexError, ValueError):
                    pass
    print(f"  runtime N={n:4d}: {res}")
    return res


# ── measure ──────────────────────────────────────────────────────────────────

print("=== [0,0] Compile: Transform ===")
transform_keys = ["baseline", "mutate", "fn_ptr", "trans", "hana_fold", "ref", "ctref"]
transform_defs = {
    "baseline":  "TEST_BASELINE",
    "mutate":    "TEST_MUTATE",
    "fn_ptr":    "TEST_FNPTR",
    "trans":     "TEST_TRANS",
    "hana_fold": "TEST_HANA_FOLD",
    "ref":       "TEST_REF",
    "ctref":     "TEST_CTREF",
}
transform_ct = {k: [] for k in transform_keys}
for n in COMPILE_SIZES_TRANSFORM:
    for k in transform_keys:
        transform_ct[k].append(compile_ms(FLAGS_FOLD, SRC_FOLD, n, transform_defs[k]))

print("\n=== [0,1] Compile: Find ===")
find_keys = ["hapi_first", "hapi_mid", "hapi_last",
             "hana_first", "hana_mid",  "hana_last"]
find_defs = {
    "hapi_first": "TEST_HAPI_FIRST",
    "hapi_mid":   "TEST_HAPI_MIDDLE",
    "hapi_last":  "TEST_HAPI_LAST",
    "hana_first": "TEST_HANA_FIRST",
    "hana_mid":   "TEST_HANA_MIDDLE",
    "hana_last":  "TEST_HANA_LAST",
}
find_ct = {k: [] for k in find_keys}
for n in COMPILE_SIZES_FIND:
    for k in find_keys:
        find_ct[k].append(compile_ms(FLAGS_FIND, SRC_FIND, n, find_defs[k]))

print("\n=== [1,0] Runtime: Transform ===")
rt_transform = {}
print("\n=== [1,1] Runtime: Ref cliff ===")
rt_cliff = {}
with tempfile.TemporaryDirectory() as tmp:
    all_ns = sorted(set(RUNTIME_SIZES_TRANSFORM) | set(RUNTIME_SIZES_REF_CLIFF))
    for n in all_ns:
        rt_transform[n] = runtime_ns(n, tmp)
        rt_cliff[n]     = rt_transform[n]   # same binary, both panels read from it


# ── plot ─────────────────────────────────────────────────────────────────────

fig, axes = plt.subplots(2, 2, figsize=(16, 11))
fig.suptitle(
    "HAPI HAPIFold vs Boost.Hana — Compile-time & Runtime\n"
    f"g++ 13.3 · host: neurux",
    fontsize=13,
)

# ── [0,0] Compile: Transform ─────────────────────────────────────────────────
ax = axes[0][0]
ct_series = [
    ("baseline",  "Baseline (include only)", "black",   "--", "x"),
    ("mutate",    "Mutate<functor{}>",       "green",   "-",  "s"),
    ("fn_ptr",    "Mutate<free_fn>",         "orange",  "-",  "s"),
    ("trans",     "Trans<fn>",               "purple",  "-.", "^"),
    ("ref",       "Ref<T,F>",                "blue",    "-",  "o"),
    ("ctref",     "CtRef<I,T,fn,Arr>",       "cyan",    "-.", "D"),
    ("hana_fold", "Hana for_each",           "blue",    ":",  "D"),
]
for key, label, color, ls, marker in ct_series:
    ys = transform_ct[key]
    xs = [n for n, y in zip(COMPILE_SIZES_TRANSFORM, ys) if y is not None]
    ys = [y for y in ys if y is not None]
    ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
ax.set_title("Compile time — Transform  (-fsyntax-only -O0)")
ax.set_xlabel("N  (chain length)")
ax.set_ylabel("ms")
ax.legend(fontsize=9)
ax.grid(True, alpha=0.4)

# ── [0,1] Compile: Find ──────────────────────────────────────────────────────
ax = axes[0][1]
find_series = [
    ("hapi_first", "HAPI find — first",    "green", "-",  "s"),
    ("hapi_mid",   "HAPI find — middle",   "green", "-.", "D"),
    ("hapi_last",  "HAPI find — last",     "green", ":",  "^"),
    ("hana_first", "Hana find_if — first", "blue",  "-",  "s"),
    ("hana_mid",   "Hana find_if — middle","blue",  "-.", "D"),
    ("hana_last",  "Hana find_if — last",  "blue",  ":",  "^"),
]
for key, label, color, ls, marker in find_series:
    ys = find_ct[key]
    xs = [n for n, y in zip(COMPILE_SIZES_FIND, ys) if y is not None]
    ys = [y for y in ys if y is not None]
    ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
ax.set_title("Compile time — Find  (-fsyntax-only -O0, -std=c++20)")
ax.set_xlabel("N  (chain length)")
ax.set_ylabel("ms")
ax.legend(fontsize=9)
ax.grid(True, alpha=0.4)

# ── [1,0] Runtime: Transform ─────────────────────────────────────────────────
ax = axes[1][0]
rt_series = [
    ("manual",  "manual",              "black",  "--", "x"),
    ("mutate",  "Mutate<functor{}>",   "green",  "-",  "s"),
    ("fn_ptr",  "Mutate<free_fn>",     "orange", "-",  "s"),
    ("trans",   "Trans<fn>",           "purple", "-.", "^"),
    ("ctref",   "CtRef<I,T,fn,Arr>",   "cyan",   "-",  "D"),
    ("hana",    "Hana for_each",       "blue",   ":",  "D"),
    ("hana_fn", "Hana for_each (ptr)", "red",    ":",  "x"),
]
for key, label, color, ls, marker in rt_series:
    xs = [n for n in RUNTIME_SIZES_TRANSFORM if key in rt_transform.get(n, {})]
    ys = [rt_transform[n][key] for n in xs]
    if xs:
        ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
ax.set_title("Runtime — Transform pipeline  (-O2, 1M reps)")
ax.set_xlabel("N  (chain length)")
ax.set_ylabel("ns / call")
ax.legend(fontsize=9)
ax.grid(True, alpha=0.4)

# ── [1,1] Runtime: Ref cliff ─────────────────────────────────────────────────
ax = axes[1][1]
cliff_series = [
    ("manual", "manual",           "black",  "--", "x"),
    ("mutate", "Mutate<F>",        "green",  "-",  "s"),
    ("ctref",  "CtRef<I,T,F,Arr>", "cyan",   "-",  "D"),
    ("ref",    "Ref<T,F>",         "blue",   "-",  "o"),
]
for key, label, color, ls, marker in cliff_series:
    xs = [n for n in RUNTIME_SIZES_REF_CLIFF if key in rt_cliff.get(n, {})]
    ys = [rt_cliff[n][key] for n in xs]
    if xs:
        ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
ax.set_title("Runtime — Ref<T,F> inlining cliff  (-O2, 1M reps)")
ax.set_xlabel("N  (chain length)")
ax.set_ylabel("ns / call")
ax.legend(fontsize=9)
ax.grid(True, alpha=0.4)

plt.tight_layout()
out = os.path.join(BENCH_DIR, "grafico_fold.png")
plt.savefig(out, dpi=150)
print(f"\nSaved: {out}")
