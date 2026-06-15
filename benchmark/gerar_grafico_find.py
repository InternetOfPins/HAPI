import os, time, subprocess
import matplotlib.pyplot as plt

sizes = [10, 25, 50, 100, 200, 500]

source      = "../main_find.cpp"
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

tests = [
    # (flag,              label,                  color,   linestyle, marker)
    ("TEST_BASELINE",    "Baseline",              "black", "--",      "x"),
    # HAPI
    ("TEST_HAPI_FIRST",  "HAPI find (first)",     "green", "-",       "s"),
    ("TEST_HAPI_MIDDLE", "HAPI find (middle)",    "green", "-.",      "D"),
    ("TEST_HAPI_LAST",   "HAPI find (last)",      "green", ":",       "^"),
    # Hana
    ("TEST_HANA_FIRST",  "Hana find_if (first)",  "blue",  "-",       "s"),
    ("TEST_HANA_MIDDLE", "Hana find_if (middle)", "blue",  "-.",      "D"),
    ("TEST_HANA_LAST",   "Hana find_if (last)",   "blue",  ":",       "^"),
]

results = {}
for flag, label, color, ls, marker in tests:
    print(f"\nA medir: {label}")
    results[flag] = measure(sizes, flag)

# ---- Plot ---------------------------------------------------------------
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
fig.suptitle('Compile-time Find: hapi::FindFirst vs hana::find_if\n'
             '(type-level only, no value instantiated)', fontsize=13)

hapi_tests = [t for t in tests if "HAPI" in t[0] or "BASELINE" in t[0]]
hana_tests = [t for t in tests if "HANA" in t[0] or "BASELINE" in t[0]]

def plot_panel(ax, panel_tests, title):
    for flag, label, color, ls, marker in panel_tests:
        vals = results[flag]
        # filter out None (compile errors)
        xs = [s for s, v in zip(sizes, vals) if v is not None]
        ys = [v for v in vals if v is not None]
        ax.plot(xs, ys, label=label, color=color, linestyle=ls, marker=marker)
    ax.set_title(title)
    ax.set_xlabel('Número de Elementos (N)')
    ax.set_ylabel('Tempo (Segundos)')
    ax.legend()
    ax.grid(True)

plot_panel(ax1, hapi_tests, 'hapi::FindFirst — lazy, position-dependent')
plot_panel(ax2, hana_tests, 'hana::find_if — position-dependent')

plt.tight_layout()
plt.savefig('grafico_find.png', dpi=150)
print("\nGráfico gerado: grafico_find.png")
