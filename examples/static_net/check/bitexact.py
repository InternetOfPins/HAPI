#!/usr/bin/env python3
"""Is a device bit-exact with the host? Compares a device's Sonar benchmark report with the host's, ROW BY ROW.

    bitexact.py --log device.txt          a saved report (anything the device printed; extra text is ignored)
    bitexact.py --port /dev/ttyUSB0       read a live device (opening the port resets most boards; 115200 baud)
    (simavr output works as a --log too: sonar_rows_avr.cpp, an ATmega328p, simulated)

The device runs include/bench.h and prints, for each cell, its output on every held-out row of fold 0 as a bit string
("narrow rows 0101...", "wide   rows 0101..."), next to the counts. The host side is host_ref.cpp: the SAME bench_correctness() built
with g++. This script builds and runs it, then compares every report it finds in the device text (a bare-metal build prints one per
instruction-cache setting, so two per pass): the two bit strings, and the counts. Counts alone can agree while rows differ; this cannot.
Exit status: 0 all reports identical to the host's, 1 a mismatch, 2 no complete report found."""
import argparse, os, re, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROWS = re.compile(r'^(narrow|wide)\s+rows\s+([01]+)\.*\s*$', re.M)          # \.*: simavr echoes the CR LF of a UART as dots
COUNT = re.compile(r'^(narrow|wide)\s+\(Prod=int\d+\)\s+correct\s+(\d+)/(\d+)\s+agree\s+(\d+)\.*\s*$', re.M)


def host_report():
    hapi = os.environ.get('HAPI', os.path.join(HERE, '../../../include'))
    with tempfile.TemporaryDirectory() as d:
        exe = os.path.join(d, 'host_ref')
        subprocess.run(['g++', '-std=c++17', '-O2', '-I' + HERE, '-I' + os.path.join(HERE, '../include'), '-I' + os.path.join(HERE, '../models/sonar'), '-I' + hapi,
                        os.path.join(HERE, 'host_ref.cpp'), '-o', exe], check=True)
        return subprocess.run([exe], check=True, capture_output=True, text=True).stdout


def device_text(a):
    if a.log:
        return open(a.log, errors='replace').read()
    import serial
    s = serial.Serial(a.port, a.baud, timeout=0.2)
    t0, buf = time.time(), b''
    while time.time() - t0 < a.wait:
        buf += s.read(512)
    s.close()
    return buf.decode(errors='replace')


def summarize(text):
    rows = [(k, v) for k, v in ROWS.findall(text.replace('\r', ''))]
    counts = [(k, int(c), int(n), int(g)) for k, c, n, g in COUNT.findall(text.replace('\r', ''))]
    return rows, counts


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--log')
    g.add_argument('--port')
    ap.add_argument('--baud', type=int, default=115200)
    ap.add_argument('--wait', type=float, default=10.0, help='seconds to read from --port')
    a = ap.parse_args()

    hrows, hcounts = summarize(host_report())
    host = dict(hrows)
    hcount = {k: (c, n, g) for k, c, n, g in hcounts}
    print('host   :', {k: '%s (%d rows)' % (v[:12] + '...', len(v)) for k, v in host.items()}, 'counts', hcount)

    drows, dcounts = summarize(device_text(a))
    reports = [(drows[i], drows[i + 1]) for i in range(len(drows) - 1) if drows[i][0] == 'narrow' and drows[i + 1][0] == 'wide']
    if not reports:
        print('no complete report (a "narrow rows" line followed by a "wide   rows" line) in the device text'); return 2
    bad = 0
    for n, ((_, nb), (_, wb)) in enumerate(reports, 1):
        for name, bits in (('narrow', nb), ('wide', wb)):
            ref = host[name]
            diff = [i for i in range(max(len(ref), len(bits))) if i >= len(ref) or i >= len(bits) or ref[i] != bits[i]]
            if diff:
                bad += 1
                print('report %d %-6s MISMATCH in %d of %d rows: %s' % (n, name, len(diff), len(ref), ', '.join(
                    'row %d device %s host %s' % (i, bits[i] if i < len(bits) else '-', ref[i] if i < len(ref) else '-') for i in diff[:8])))
    print('%d report(s) x 2 cells x %d rows compared, %d cell report(s) differ' % (len(reports), len(host['narrow']), bad))
    dc = {}
    for k, c, nn, g in dcounts:
        dc.setdefault(k, set()).add((c, nn, g))
    for k in ('narrow', 'wide'):
        ok = dc.get(k) == {hcount[k]}
        print('counts %-6s device %s host %s: %s' % (k, sorted(dc.get(k, [])), hcount[k], 'same' if ok else 'DIFFERENT'))
        bad += not ok
    print('BIT-EXACT: %d report(s), every row of both cells identical to the host' % len(reports) if not bad else 'NOT bit-exact')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
