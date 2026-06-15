import os, time, subprocess
import matplotlib.pyplot as plt

sizes_map  = [10, 25, 50, 100, 200, 500]
sizes_find = [10, 25, 50, 100, 200, 500]

source      = "../main.cpp"
include_dir = "../../include"

base_cmd = lambda n, flag: (
    f"g++ -std=c++17 -fsyntax-only -I{include_dir} "
    f"-DTEST_SIZE={n} -D{flag} {source}"
)

def measure(sizes, flag):
    times = []
    for n in sizes:
        t0 = time.time()
        result = subprocess.run(base_cmd(n, flag), shell=True,
                                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        elapsed = time.time() - t0
        if result.returncode != 0:
            print(f"  ERROR {flag} N={n}: {result.stderr.decode()[:120]}")
            times.append(None)
        else:
            times.append(elapsed)
            print(f"  {flag} N={n}: {elapsed:.3f}s")
    return times

map_tests = [
    ("TEST_BASELINE",   "Baseline",          "black", "--", "x"),
    ("TEST_TUPLE_TYPE", "std::tuple (type)", "red",   "-",  "o"),
    ("TEST_HANA_TYPE",  "Hana (type)",       "blue",  "-",  "^"),
    ("TEST_HAPI_TYPE",  "hapi::Map (type)",  "green", "-",  "s"),
]

find_tests = [
    ("TEST_BASELINE",    "Baseline",              "black", "--",  "x"),
    ("TEST_HAPI_FIRST",  "HAPI find (first)",     "green", "-",   "s"),
    ("TEST_HAPI_MIDDLE", "HAPI find (middle)",    "green", "-.",  "D"),
    ("TEST_HAPI_LAST",   "HAPI find (last)",      "green", ":",   "^"),
    ("TEST_HANA_FIRST",  "Hana find_if (first)",  "blue",  "-",   "s"),
    ("TEST_HANA_MIDDLE", "Hana find_if (middle)", "blue",  "-.",  "D"),
    ("TEST_HANA_LAST",   "Hana find_if (last)",   "blue",  ":",   "^"),
]

# Deduplicate — baseline measured once, reused in both charts
all_flags = {t[0] for t in map_tests + find_tests}
results = {}
for flag in all_flags:
    sizes = sizes_map if flag in {t[0] for t in map_tests} else sizes_find
    print(f"\nA medir: {flag}")
    results[flag] = measure(sizes, flag)

# ---- Plot ---------------------------------------------------------------
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
fig.suptitle('HAPI vs Boost.Hana — Type-level Map and Find\n'
             '(compile-time only, no value instantiated)', fontsize=13)

def plot_panel(ax, tests, sizes, title):
    for flag, label, color, ls, marker in tests:
        vals = results[flag]
        xs = [s for s, v in zip(sizes, vals) if v is not None]
        ys = [v for v in vals if v is not None]
        ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
    ax.set_title(title)
    ax.set_xlabel('Número de Elementos (N)')
    ax.set_ylabel('Tempo (Segundos)')
    ax.legend()
    ax.grid(True)

plot_panel(ax1, map_tests,  sizes_map,  'Map: int -> int* (type-level)')
plot_panel(ax2, find_tests, sizes_find, 'Find: first / middle / last position')

plt.tight_layout()
plt.savefig('grafico_performance.png', dpi=150)
print("\nGráfico gerado: grafico_performance.png")
