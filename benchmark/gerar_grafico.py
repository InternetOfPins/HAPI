import os, time, subprocess
import matplotlib.pyplot as plt

BENCH_DIR        = os.path.dirname(os.path.abspath(__file__))
source           = os.path.join(BENCH_DIR, "main.cpp")
include_dir      = os.path.join(BENCH_DIR, "..", "include")
parse_source     = os.path.join(BENCH_DIR, "..", "..", "OneParse", "benchmark", "bench_parse.cpp")
op_include_dir   = os.path.join(BENCH_DIR, "..", "..", "OneParse", "include")

sizes_map   = [10, 100, 200, 300]
sizes_find  = [10, 100, 200, 300]
sizes_tree  = [2, 5, 10, 14]        # B values, total = B² (max 196)
sizes_val   = [10, 20, 30, 50]
sizes_parse = [5, 10, 20, 40, 80]   # N distinct field types

def base_cmd(n, flag, tree=False):
    size_flag = f"-DTREE_B={n}" if tree else f"-DTEST_SIZE={n}"
    return (
        f"g++ -std=c++20 -fsyntax-only -ftemplate-depth=2000 "
        f"-I{include_dir} {size_flag} -D{flag} {source}"
    )

def parse_cmd(n, flag):
    return (
        f"g++ -std=c++20 -fsyntax-only -ftemplate-depth=2000 "
        f"-I{include_dir} -I{op_include_dir} "
        f"-DTEST_SIZE={n} -D{flag} {parse_source}"
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
    ("TEST_HAPI_MAPPED",   "HAPI Map<Inc> (value)",      "green",  "-",  "s", sizes_val, False),
    ("TEST_TRANS",         "HAPI Trans<Inc> pipeline",   "purple", "-",  "P", sizes_val, False),
    ("TEST_HANA_VAL_FIND", "Hana find_if (value)",       "blue",   "-.", "D", sizes_val, False),
    ("TEST_STD_VAL_MAP",   "std::apply+tuple (value)",   "red",    "-",  "o", sizes_val, False),
    ("TEST_HAPI_FOR_EACH", "HAPI forEach (type-level)",  "green",  "-.", "D", sizes_val, False),
    ("TEST_NODE_ONLY",     "Node construction only",     "gray",   ":",  "x", sizes_val, False),
]

parser_tests = [
    ("TEST_PARSE_BASELINE",      "Baseline",   "black", "--", "x", sizes_parse),
    ("TEST_PARSE_ONEPARSE_JSON", "oneParse",   "green", "-",  "s", sizes_parse),
    ("TEST_PARSE_SPIRIT_JSON",   "Spirit.X3",  "red",   "-",  "o", sizes_parse),
    ("TEST_PARSE_HANA_JSON",     "Hana (type)","blue",  "--", "^", sizes_parse),
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

def measure_parse(sizes, flag):
    times = []
    for n in sizes:
        t0 = time.time()
        result = subprocess.run(
            parse_cmd(n, flag), shell=True,
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        elapsed = time.time() - t0
        if result.returncode != 0:
            print(f"  ERROR {flag} N={n}: {result.stderr.decode()[:100]}")
            times.append(None)
        else:
            times.append(elapsed)
            print(f"  {flag} N={n}: {elapsed:.3f}s")
    return times

parse_results = {}
for flag, label, color, ls, marker, sizes in parser_tests:
    print(f"\nA medir (parser): {label}")
    parse_results[flag] = measure_parse(sizes, flag)

def get(flag, sizes, tree):
    return results.get((flag, sizes[0], tree), [None]*len(sizes))

def get_parse(flag, sizes):
    return parse_results.get(flag, [None]*len(sizes))

# ---- plot --------------------------------------------------------------
fig, axes = plt.subplots(3, 2, figsize=(16, 18))
fig.suptitle(
    'HAPI vs Boost.Hana vs Spirit.X3 — Compile-time type-level performance\n'
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

def plot_parse_panel(ax, tests, title):
    for flag, label, color, ls, marker, sizes in tests:
        vals = get_parse(flag, sizes)
        xs = [s for s, v in zip(sizes, vals) if v is not None]
        ys = [v for v in vals if v is not None]
        ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
    ax.set_title(title)
    ax.set_xlabel('N distinct field types')
    ax.set_ylabel('Tempo (Segundos)')
    ax.legend(fontsize=8)
    ax.grid(True)

plot_panel(axes[0][0], map_tests,  'Map: int -> int* (type-level)')
plot_panel(axes[0][1], find_tests, 'Find: flat chain — first / middle / last')
plot_panel(axes[1][0], tree_tests, 'Tree topology (B×B) — HAPI native vs flatten',
           xlabel='Total Elements (N = B²)', x_fn=lambda b: b*b)
plot_panel(axes[1][1], val_tests,
           "Iteration over all N: value ops (Hana/std) vs type-level forEach (HAPI)")
plot_parse_panel(axes[2][0], parser_tests,
                 "Parser grammar instantiation — N distinct field types\n"
                 "OneParse (HAPI chain) vs Spirit.X3 vs Hana tuple")
axes[2][1].set_visible(False)

plt.tight_layout()
plt.savefig('grafico_performance.png', dpi=150)
print("\nGráfico gerado: grafico_performance.png")
