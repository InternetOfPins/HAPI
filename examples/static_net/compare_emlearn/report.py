#!/usr/bin/env python3
"""Joins run.sh's measurements (out/measure.txt: name flash ram n ok min max sum) with results.json into one table.
'net' columns subtract the null program (no model: the harness, the row fetch and the call), so they are the model's own cost."""
import json, os
here = os.path.dirname(os.path.abspath(__file__))
res = json.load(open(os.path.join(here, 'results.json')))['models']
rows = [l.split() for l in open(os.path.join(here, 'out/measure.txt')) if l.strip()]
F = ('flash', 'ram', 'n', 'ok', 'min', 'max', 'sum')
m = {r[0]: dict(zip(F, (int(v) for v in r[1:]))) for r in rows}
null = m.get('null')
print('| model | flash net (B) | RAM net (B) | on-device ok | cycles min / mean / max (net of null) | accuracy fold 1 / 5-fold mean (%) |')
print('|---|---|---|---|---|---|')
for name, d in m.items():
    base = null if (null and name != 'null') else dict.fromkeys(F, 0)
    n = d['n']
    cyc = (d['min'] - base['min'], d['sum'] / n - (base['sum'] / base['n'] if base['n'] else 0), d['max'] - base['max'])
    r = res.get(name)
    acc = '%.2f / %.2f' % (r['test_acc_fold1'], r['test_acc_mean']) if r else '-'
    total = ' (total)' if name == 'null' else ''
    print('| %s | %d%s | %d%s | %d/%d | %d / %.1f / %d | %s |' % (name, d['flash'] - base['flash'], total, d['ram'] - base['ram'], total, d['ok'], n, cyc[0], cyc[1], cyc[2], acc))
