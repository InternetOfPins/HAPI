#!/usr/bin/env python3
"""
bench_chart.py — 4-panel compile-time chart from run_bench.sh log
Usage: python3 bench_chart.py results/YYYY-MM-DD_HH-MM-SS.log
Output: bench_compile.png (alongside this script)

Panels:
  top-left:     Map type-level (HAPI vs Hana vs node-only)
  top-right:    FindFirst first/middle/last (HAPI vs Hana)
  bottom-left:  Tree topology B×B (HAPI native vs Hana flatten)
  bottom-right: Value-level iteration vs type-level forEach
"""

import sys, re, os
import matplotlib.pyplot as plt

LOG = sys.argv[1] if len(sys.argv) > 1 else None
if not LOG:
    results_dir = os.path.join(os.path.dirname(__file__), "results")
    logs = sorted(f for f in os.listdir(results_dir)
                  if f.endswith(".log") and not f.startswith("fold_"))
    if not logs:
        print("No log files found"); sys.exit(1)
    LOG = os.path.join(results_dir, logs[-1])

print(f"Reading: {LOG}")
lines = open(LOG).readlines()

# ── parse header ──────────────────────────────────────────────────────────────

date_str, hapi_ver = "", ""
for l in lines:
    if l.startswith("Date:"):  date_str = l.split(":", 1)[1].strip()
    if l.startswith("HAPI:"):  hapi_ver = l.split(":", 1)[1].strip()

# ── parse sections (### SECTION: name ###) ────────────────────────────────────

sections = {}   # name → {"keys": [int...], "data": {label: [int|None...]}}
current = None

for l in lines:
    l = l.rstrip()
    m = re.match(r'###\s+SECTION:\s+(\w+)\s+###', l)
    if m:
        current = m.group(1)
        sections[current] = {"keys": [], "data": {}}
        continue
    if current is None:
        continue
    if "------" in l or l.startswith("===") or l.startswith("Saved"):
        continue
    kv = re.findall(r'[NB]=(\d+)', l)
    if kv:
        sections[current]["keys"] = [int(x) for x in kv]
        continue
    parts = l.split()
    if len(parts) < 2:
        continue
    label = parts[0]
    if ":" in label or label.startswith("==="):
        continue
    vals = []
    for p in parts[1:]:
        try:    vals.append(int(p))
        except: vals.append(None)
    if vals:
        sections[current]["data"][label] = vals

flat = sections.get("flat", {"keys": [], "data": {}})
tree = sections.get("tree", {"keys": [], "data": {}})

flat_ns     = flat["keys"]
flat_data   = flat["data"]
tree_bs     = tree["keys"]
tree_totals = [b * b for b in tree_bs]
tree_data   = tree["data"]

flat_baseline = flat_data.get("baseline", [0] * len(flat_ns))
tree_baseline = tree_data.get("baseline", [0] * len(tree_bs))

# ── helpers ───────────────────────────────────────────────────────────────────

def net_seconds(raw, baseline):
    """Subtract baseline per-index, convert ms → s. Returns (xs, ys) lists."""
    xs, ys = [], []
    for i, v in enumerate(raw):
        b = baseline[i] if i < len(baseline) else None
        if v is not None and b is not None:
            xs.append(i)
            ys.append((v - b) / 1000.0)
    return xs, ys

def plot_panel(ax, x_vals, data, baseline, series, xlabel, title):
    """Plot net-of-baseline lines. series = [(label, style_dict), ...]"""
    bl = list(baseline)
    for label, style in series:
        raw = data.get(label)
        if not raw:
            continue
        xs_i, ys = net_seconds(raw, bl)
        xs = [x_vals[i] for i in xs_i if i < len(x_vals)]
        ys = ys[:len(xs)]
        if xs:
            ax.plot(xs, ys, **style)
    ax.set_xlabel(xlabel, fontsize=9)
    ax.set_ylabel("Compile time above baseline (seconds)", fontsize=9)
    ax.set_title(title, fontsize=10)
    ax.legend(fontsize=8)
    ax.grid(True, alpha=0.35)

# ── figure 2×2 ────────────────────────────────────────────────────────────────

fig, axes_flat = plt.subplots(1, 3, figsize=(22, 7), layout="constrained")
fig.suptitle(
    f"HAPI v{hapi_ver} vs Boost.Hana vs Spirit.X3 — Compile-time type-level performance\n"
    "All measurements: template instantiation cost only "
    "(g++ −fsyntax−only, no runtime values)",
    fontsize=11
)
ax_map, ax_find, ax_tree = axes_flat

HAPI_G = ["#1a7a1a", "#2ca02c", "#4dbb4d", "#80d480", "#006600"]
HANA_B = ["#1f77b4", "#4a9fd4", "#7ec0e8", "#aee8ff"]
HANA_R = ["#cc2200", "#e84c2e", "#ff7755", "#ffaa88"]  # Hana in red for tree (flatten cost)
STD_K  = "#7f7f7f"
NODE_K = "#aaaaaa"

# ── Map + Iteration combined ──────────────────────────────────────────────────
plot_panel(ax_map, flat_ns, flat_data, flat_baseline,
    [
        ("hapi_type",    dict(label="HAPI Map type",               color=HAPI_G[0],marker="o")),
        ("hana_type",    dict(label="Hana transform type",         color=HANA_B[0],marker="s", linestyle="--")),
        ("hana_val_map", dict(label="Hana transform (value)",      color=HANA_B[1],marker="s")),
        ("hapi_first",   dict(label="HAPI find type-level",        color=HAPI_G[1],marker="o", linestyle=":")),
        ("hana_val_find",dict(label="Hana find (value)",           color=HANA_B[2],marker="s", linestyle=":")),
    ],
    "Number of elements (N)",
    "Type-level map & find performance")

# ── FindFirst ─────────────────────────────────────────────────────────────────
plot_panel(ax_find, flat_ns, flat_data, flat_baseline,
    [
        ("hapi_first", dict(label="HAPI find - first",  color=HAPI_G[0], marker="o")),
        ("hapi_mid",   dict(label="HAPI find - middle", color=HAPI_G[1], marker="o", linestyle="--")),
        ("hapi_last",  dict(label="HAPI find - last",   color=HAPI_G[2], marker="o", linestyle=":")),
        ("hana_first", dict(label="Hana find - first",  color=HANA_B[0], marker="s")),
        ("hana_mid",   dict(label="Hana find - middle", color=HANA_B[1], marker="s", linestyle="--")),
        ("hana_last",  dict(label="Hana find - last",   color=HANA_B[2], marker="s", linestyle=":")),
    ],
    "Number of elements (N)",
    "Find: flat chain — first / middle / last")

# ── Tree topology ─────────────────────────────────────────────────────────────
plot_panel(ax_tree, tree_totals, tree_data, tree_baseline,
    [
        ("hapi_tree_map",   dict(label="HAPI Map (tree native)",   color=HAPI_G[0], marker="o")),
        ("hapi_tree_first", dict(label="HAPI find - first",        color=HAPI_G[1], marker="o", linestyle="--")),
        ("hapi_tree_last",  dict(label="HAPI find - last",         color=HAPI_G[2], marker="o", linestyle=":")),
        ("hana_tree_map",   dict(label="Hana flatten+transform",   color=HANA_R[0], marker="s", linestyle="--")),
        ("hana_tree_find",  dict(label="Hana flatten+find",        color=HANA_R[1], marker="s", linestyle=":")),
    ],
    "Total elements (N = B×B)",
    "Tree topology (B×B) — HAPI native vs Hana flatten")

OUT = os.path.join(os.path.dirname(__file__), "bench_compile.png")
plt.savefig(OUT, dpi=150)
print(f"Chart → {OUT}")
