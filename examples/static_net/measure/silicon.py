#!/usr/bin/env python3
"""Do the simavr numbers hold on a real chip? Flash each measure/ ELF to an ATmega328p (an Arduino Nano), read what it prints over UART, and
put it next to simavr's output for THE SAME ELF.

    silicon.py [--port /dev/ttyUSB0] [--programmer arduino] [--baud 57600] [targets...]      targets: ELF files (default: the ones run.sh and ../compare_emlearn/run.sh build)
    silicon.py --sim-only [targets...]                                                        only simavr (no board): what the table would compare against

Build the ELFs first (run.sh, ../compare_emlearn/run.sh), so both sides run the identical binary. The harness prints its numbers once, right after start,
at UBRR0=8 (111.1 kbaud at 16 MHz, 3.5% under 115200: fine for the CH340/CP210x/FTDI of a Nano). Opening the serial port resets a Nano through DTR, so
the port is opened right after avrdude finishes and read for a few seconds. avrdude: -c arduino at 57600 is the old Nano bootloader, 115200 the new one.
Both kinds of output are understood: "label:cycles" (measure/) and "name: n= ok= min= max= sum=" (compare_emlearn/).
Run on an Arduino Nano (ATmega328p, 16 MHz, CH340, -c arduino -b 115200) on 2026-09-24: all 100 numbers of run.sh's and ../compare_emlearn/run.sh's programs
were identical to simavr's (silicon_results.md)."""
import argparse, glob, os, re, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
OLD = re.compile(r'([a-z0-9]+):(\d+)')
BNC = re.compile(r'(\w+): n=(\d+) ok=(\d+) min=(\d+) max=(\d+) sum=(\d+)')


def parse(text):
    """{label: value} from either output format (simavr colours its output; the escape codes are removed first)"""
    text = re.sub(r'\x1b\[[0-9;]*m', '', text)
    out = {}
    m = BNC.search(text)
    if m:
        name, n, ok, mn, mx, sm = m.group(1), *(int(v) for v in m.groups()[1:])
        out.update({name + '.ok': ok, name + '.min': mn, name + '.max': mx, name + '.sum': sm})
    else:
        for k, v in OLD.findall(text):
            out[k] = int(v)
    return out


def simavr(elf):
    r = subprocess.run(['simavr', '-m', 'atmega328p', '-f', '16000000', elf], capture_output=True, text=True, timeout=60)
    return parse(r.stdout + r.stderr)


def silicon(elf, a):
    import serial
    subprocess.run(['avrdude', '-p', 'm328p', '-c', a.programmer, '-P', a.port, '-b', str(a.baud), '-U', 'flash:w:%s:e' % elf],
                   check=True, capture_output=True)
    s = serial.Serial(a.port, 115200, timeout=0.2)         # opening resets the board; the program prints within milliseconds of starting
    t0, buf = time.time(), b''
    while time.time() - t0 < a.wait and b'\n' not in buf:      # the harness ends its report with a newline
        buf += s.read(256)
    s.close()
    return parse(buf.decode(errors='replace'))


def compare(name, sim, dev):
    rows = []
    for k in sim:
        d = dev.get(k)
        rows.append((name, k, sim[k], d, '' if d is None else '%+d' % (d - sim[k])))
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('targets', nargs='*')
    ap.add_argument('--port', default='/dev/ttyUSB0')
    ap.add_argument('--programmer', default='arduino')
    ap.add_argument('--baud', type=int, default=57600)
    ap.add_argument('--wait', type=float, default=4.0)
    ap.add_argument('--sim-only', action='store_true')
    a = ap.parse_args()
    elfs = a.targets or sorted(glob.glob(os.path.join(HERE, '*.elf')) + glob.glob(os.path.join(HERE, '../compare_emlearn/out/*.elf')))
    print('| target | number | simavr | silicon | diff |\n|---|---|---|---|---|')
    worst = 0
    for elf in elfs:
        name = os.path.basename(elf)[:-4]
        sim = simavr(elf)
        dev = sim if a.sim_only else silicon(elf, a)          # --sim-only: the comparison half exercised against itself
        for n, k, s, d, diff in compare(name, sim, dev):
            print('| %s | %s | %s | %s | %s |' % (n, k, s, '-' if d is None else d, diff))
            if d is not None and not k.endswith(('.ok', '.sum')):
                worst = max(worst, abs(d - s))
    print('\nlargest |silicon - simavr| over the cycle numbers: %d cycles' % worst)


if __name__ == '__main__':
    main()
