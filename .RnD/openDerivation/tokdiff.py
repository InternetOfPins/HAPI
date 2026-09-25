#!/usr/bin/env python3
"""tokdiff.py A B -- are two C++ sources textually equivalent modulo whitespace?
Compares the token streams (translate.py's lexer) and, separately, the comments and preprocessor lines
(whitespace-normalized). Prints EQUIVALENT, or the first differing token with its line in each file."""
import re
import sys
from translate import lex

def extras(src):
    out = []
    for m in re.finditer(r'//[^\n]*|/\*.*?\*/|^[ \t]*#[^\n]*', src, re.S | re.M):
        out.append(' '.join(m.group(0).split()))
    return out

def main(a, b):
    sa, sb = open(a).read(), open(b).read()
    ta, tb = lex(sa, a), lex(sb, b)
    for x, y in zip(ta, tb):
        if x.text != y.text:
            print(f"DIFFER: {a}:{x.line} {x.text!r} vs {b}:{y.line} {y.text!r}")
            return 1
    if len(ta) != len(tb):
        print(f"DIFFER: {len(ta)} vs {len(tb)} tokens")
        return 1
    ea, eb = extras(sa), extras(sb)
    if ea != eb:
        d = next((i for i, (x, y) in enumerate(zip(ea, eb)) if x != y), min(len(ea), len(eb)))
        print(f"DIFFER in comments/preprocessor #{d}: {ea[d:d+1]} vs {eb[d:d+1]}")
        return 1
    print(f"EQUIVALENT: {a} == {b} ({len(ta)} tokens, {len(ea)} comments/directives; whitespace ignored)")
    return 0

if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:3]))
