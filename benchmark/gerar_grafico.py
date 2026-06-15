import os, time, subprocess
import matplotlib.pyplot as plt

source      = "../main.cpp"
include_dir = "../../include"

sizes_map   = [10, 100, 200, 300]
sizes_find  = [10, 100, 200, 300]
sizes_tree  = [2, 5, 10, 14]        # B values, total = B² (max 196)
sizes_val   = [10, 100, 200, 300]

def base_cmd(n, flag, tree=False):
    size_flag = f"-DTREE_B={n}" if tree else f"-DTEST_SIZE={n}"
    return (
        f"g++ -std=c++17 -fsyntax-only -ftemplate-depth=2000 "
        f"-I{include_dir} {size_flag} -D{flag} {source}"
    )

def measure(sizes, flag, tree=False):
    times = []
    for n in sizes:
        t0 = time.time()
        result = subprocess.run(
            base_cmd(n, flag, tree), shell=True,
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE
        )
        elapsed = time.time() - t0
        if result.returncode != 0:
            print(f"  ERROR {flag} N={n}: {result.stderr.decode()[:100]}")
            times.append(None)
        else:
            times.append(elapsed)
            print(f"  {flag} N={n if not tree else n*n}: {elapsed:.3f}s")
    return times

# ---- test definitions --------------------------------------------------
map_tests = [
    ("TEST_BASELINE",   "Baseline",           "black", "--", "x", sizes_map,  False),
    ("TEST_TUPLE_TYPE", "std::tuple (type)",  "red",   "-",  "o", sizes_map,  False),
    ("TEST_HANA_TYPE",  "Hana transform (type)","blue","-",  "^", sizes_map,  False),
    ("TEST_HAPI_TYPE",  "hapi::Map (type)",   "green", "-",  "s", sizes_map,  False),
]

find_tests = [
    ("TEST_BASELINE",    "Baseline",              "black", "--",  "x", sizes_find, False),
    ("TEST_HAPI_FIRST",  "HAPI find — first",     "green", "-",   "s", sizes_find, False),
    ("TEST_HAPI_MIDDLE", "HAPI find — middle",    "green", "-.",  "D", sizes_find, False),
    ("TEST_HAPI_LAST",   "HAPI find — last",      "green", ":",   "^", sizes_find, False),
    ("TEST_HANA_FIRST",  "Hana find_if — first",  "blue",  "-",   "s", sizes_find, False),
    ("TEST_HANA_MIDDLE", "Hana find_if — middle", "blue",  "-.",  "D", sizes_find, False),
    ("TEST_HANA_LAST",   "Hana find_if — last",   "blue",  ":",   "^", sizes_find, False),
]

tree_tests = [
    ("TEST_BASELINE",       "Baseline",                  "black", "--", "x", sizes_tree, False),
    ("TEST_HAPI_TREE_MAP",  "HAPI Map — tree (native)",  "green", "-",  "s", sizes_tree, True),
    ("TEST_HAPI_TREE_FIRST","HAPI find — tree first",    "green", "-.", "D", sizes_tree, True),
    ("TEST_HAPI_TREE_LAST", "HAPI find — tree last",     "green", ":",  "^", sizes_tree, True),
    ("TEST_HANA_TREE_MAP",  "Hana flatten+transform",    "blue",  "-",  "s", sizes_tree, True),
    ("TEST_HANA_TREE_FIND", "Hana flatten+find_if",      "blue",  "-.", "D", sizes_tree, True),
]

val_tests = [
    ("TEST_BASELINE",      "Baseline",                   "black",  "--", "x", sizes_val, False),
    ("TEST_HANA_VAL_MAP",  "Hana transform (value)",     "blue",   "-",  "^", sizes_val, False),
    ("TEST_HANA_VAL_FIND", "Hana find_if (value)",       "blue",   "-.", "D", sizes_val, False),
    ("TEST_STD_VAL_MAP",   "std::apply+tuple (value)",   "red",    "-",  "o", sizes_val, False),
    ("TEST_HAPI_FOR_EACH", "HAPI forEach (type-level)",  "green",  "-",  "s", sizes_val, False),
    ("TEST_RUN_EACH",      "HAPI runEach (hybrid)",      "orange", "-",  "D", sizes_val, False),
]

all_tests = map_tests + find_tests + tree_tests + val_tests

# deduplicate by (flag, sizes, tree)
seen = set()
unique_tests = []
for t in all_tests:
    key = (t[0], t[5][0], t[6])
    if key not in seen:
        seen.add(key)
        unique_tests.append(t)

# ---- measure -----------------------------------------------------------
results = {}
for flag, label, color, ls, marker, sizes, tree in unique_tests:
    print(f"\nA medir: {label}")
    results[(flag, sizes[0], tree)] = measure(sizes, flag, tree)

def get(flag, sizes, tree):
    return results.get((flag, sizes[0], tree), [None]*len(sizes))

# ---- plot --------------------------------------------------------------
fig, axes = plt.subplots(2, 2, figsize=(16, 12))
fig.suptitle(
    'HAPI vs Boost.Hana — Compile-time type-level performance\n'
    'All measurements: template instantiation cost only  '
    '(g++ -fsyntax-only, no runtime values)',
    fontsize=12
)

def plot_panel(ax, tests, title, xlabel='Número de Elementos (N)', x_fn=None):
    for flag, label, color, ls, marker, sizes, tree in tests:
        vals = get(flag, sizes, tree)
        xs = [x_fn(s) if x_fn else s for s, v in zip(sizes, vals) if v is not None]
        ys = [v for v in vals if v is not None]
        ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
    ax.set_title(title)
    ax.set_xlabel(xlabel)
    ax.set_ylabel('Tempo (Segundos)')
    ax.legend(fontsize=8)
    ax.grid(True)

plot_panel(axes[0][0], map_tests,  'Map: int -> int* (type-level)')
plot_panel(axes[0][1], find_tests, 'Find: flat chain — first / middle / last')
plot_panel(axes[1][0], tree_tests, 'Tree topology (B×B) — HAPI native vs flatten',
           xlabel='Total Elements (N = B²)', x_fn=lambda b: b*b)
plot_panel(axes[1][1], val_tests,
           "Iteration over all N: value ops (Hana/std) vs type-level forEach (HAPI)")

plt.tight_layout()
plt.savefig('grafico_performance.png', dpi=150)
print("\nGráfico gerado: grafico_performance.png")
