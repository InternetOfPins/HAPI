#!/usr/bin/env python3
"""amalgamate.py -- HAPI as one header: include/hapi/*.h inlined into single/hapi.h.

    scripts/amalgamate.py            write single/hapi.h
    scripts/amalgamate.py --check    exit 1 if single/hapi.h is not what this script would write (stale)

Starts at include/hapi/hapi.h and inlines every quoted #include ("hapi/meta.h", "platform/avr/avr_std.h"), resolved
relative to the including file, then to include/. Each header is inlined once, at its first include, and keeps its own
comments; its `#pragma once` is dropped (the single header has one). An include inside #if/#else (avr_std.h) is
inlined in place, so it stays conditional. Angle-bracket includes (<type_traits>, ...) are left as they are.
Every header under include/hapi/ must be reached from hapi.h, or the script fails: a new header is never left out silently.
The output carries the MIT license and the library version (library.json), and no date or commit, so it only changes
when the headers do.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INC = os.path.join(ROOT, "include")
ENTRY = os.path.join(INC, "hapi", "hapi.h")
OUT = os.path.join(ROOT, "single", "hapi.h")

INCLUDE = re.compile(r'^(\s*)#\s*include\s*"([^"]+)"\s*(//.*)?$')
PRAGMA_ONCE = re.compile(r"^\s*#\s*pragma\s+once\b")


def resolve(name, from_dir):
    for base in (from_dir, INC):
        p = os.path.normpath(os.path.join(base, name))
        if os.path.isfile(p):
            return p
    raise SystemExit(f"amalgamate: cannot resolve #include \"{name}\" (from {os.path.relpath(from_dir, ROOT)})")


def inline(path, seen, out):
    seen.add(path)
    rel = os.path.relpath(path, INC)
    out.append(f"// ---- begin {rel} ----\n")
    with open(path) as f:
        for line in f:
            if PRAGMA_ONCE.match(line):
                continue
            m = INCLUDE.match(line)
            if m:
                target = resolve(m.group(2), os.path.dirname(path))
                if target not in seen:
                    inline(target, seen, out)
                else:
                    out.append(f"{m.group(1)}// #include \"{m.group(2)}\" -- inlined above\n")
                continue
            out.append(line)
    if out and not out[-1].endswith("\n"):
        out.append("\n")
    out.append(f"// ---- end {rel} ----\n")


def build():
    with open(os.path.join(ROOT, "LICENSE")) as f:
        license_text = f.read().strip()
    with open(os.path.join(ROOT, "library.json")) as f:
        version = json.load(f)["version"]
    seen, body = set(), []
    inline(ENTRY, seen, body)
    headers = {os.path.normpath(os.path.join(d, n))
               for d, _, ns in os.walk(os.path.join(INC, "hapi")) for n in ns if n.endswith(".h")}
    missing = sorted(os.path.relpath(h, INC) for h in headers - seen)
    if missing:
        raise SystemExit("amalgamate: not reached from hapi/hapi.h (include them, or teach this script): " + ", ".join(missing))
    head = ["/*\n"]
    head += [(" * " + l).rstrip() + "\n" for l in license_text.splitlines()]
    head += [" */\n",
             f"// HAPI {version}, single header: include/hapi/*.h inlined by scripts/amalgamate.py. Do not edit;\n",
             "// regenerate with `python3 scripts/amalgamate.py` (tests/single_header/run.sh checks it is current).\n",
             f"// Headers, in order: {', '.join(os.path.relpath(p, INC) for p in order(seen, body))}\n",
             "#pragma once\n", "\n"]
    return "".join(head + body)


def order(seen, body):
    names = [l[len("// ---- begin "):-len(" ----\n")] for l in body if l.startswith("// ---- begin ")]
    return [os.path.join(INC, n) for n in names]


def main():
    text = build()
    if "--check" in sys.argv[1:]:
        current = open(OUT).read() if os.path.isfile(OUT) else None
        if current != text:
            print(f"amalgamate: {os.path.relpath(OUT, ROOT)} is stale: run python3 scripts/amalgamate.py", file=sys.stderr)
            return 1
        print(f"amalgamate: {os.path.relpath(OUT, ROOT)} is current")
        return 0
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        f.write(text)
    print(f"amalgamate: wrote {os.path.relpath(OUT, ROOT)} ({text.count(chr(10))} lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
