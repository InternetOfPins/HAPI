#!/usr/bin/env python3
"""translate.py -- Open Derivation prototype: lower `A:B` (late derivation) into HAPI's Part form.

`:` derivation is valid only in a base clause. The syntax says whether a composition is closed:
    struct P { ...super::f()... };            an open layer: struct P {template<typename O> struct Part:O { using Base=O; using Base::Base; ...Base::f()... };};
    struct W : A:B {};                        a component (no `final`): struct W : hapi::Chain<A,B> {static_assert(hapi::Distinct<...>);};
    struct Z : A:B:final T {...};             closed on the terminal API T: struct Z : hapi::APIOf<T,A,B> {using Base=hapi::APIOf<T,A,B>; using Base::Base;
                                              static_assert(hapi::Distinct<hapi::Chain<A,B,T>>, "duplicate layer in Z"); ...};
    struct Z : W:final Nil {};                a component closed later, on the user's own terminal: hapi::APIOf<Nil,W>
    template<class... OO> struct Z : (OO : ... : P : final T) {};   ->  hapi::APIOf<T,OO...,P>
    struct Z : final T {};                    closed on T with no layers: hapi::APIOf<T>
    inside a closed struct, `super` names its base (-> Base); APIOf runs HAPI's rule walk (components' rules<Before,After>())
    using X = A:B;                            error [od-rule4]: ':' is only valid in a base clause

Usage:
    translate.py IN [-o OUT]                  one file (stdout when no -o)
    translate.py --outdir DIR IN...           several files, same basenames under DIR
    --report                                  one line per lowering on stderr

Errors are printed compiler-style (file:line:col: error: [rule] message) and the exit status is 1.
The grammar is deliberately restricted (see README.md); anything outside it is either passed through untouched
or refused with a message, never silently guessed.
"""
import argparse
import os
import re
import sys

KEYWORDS_NOT_NAMES = {"typename", "class", "struct", "template", "const", "volatile", "unsigned", "signed",
                      "int", "char", "bool", "long", "short", "auto", "..."}


class ODError(Exception):
    def __init__(self, tok, msg, path="<input>"):
        super().__init__(msg)
        self.tok, self.msg, self.path = tok, msg, path

    def __str__(self):
        if self.tok is None:
            return f"{self.path}: error: {self.msg}"
        return f"{self.path}:{self.tok.line}:{self.tok.col}: error: {self.msg}"


class Tok:
    __slots__ = ("kind", "text", "start", "end", "line", "col")

    def __init__(self, kind, text, start, end, line, col):
        self.kind, self.text, self.start, self.end, self.line, self.col = kind, text, start, end, line, col

    def __repr__(self):
        return f"{self.text!r}@{self.line}:{self.col}"


# ---------------------------------------------------------------- lexer
# comments, preprocessor lines and whitespace are not tokens: they stay in the gaps and are copied verbatim.
_LEX = re.compile(r"""
   (?P<ws>\s+)
  |(?P<lc>//[^\n]*)
  |(?P<bc>/\*.*?\*/)
  |(?P<raw>(?:u8|u|U|L)?R"(?P<delim>[^()\\\s]{0,16})\(.*?\)(?P=delim)")
  |(?P<str>(?:u8|u|U|L)?"(?:\\.|[^"\\\n])*")
  |(?P<chr>(?:u8|u|U|L)?'(?:\\.|[^'\\\n])+')
  |(?P<num>\.?\d(?:[eEpP][+-]|['\w.])*)
  |(?P<id>[A-Za-z_]\w*)
  |(?P<punct>::|\.\.\.|->|[^\s])
""", re.S | re.X)


def lex(src, path):
    toks = []
    i, n = 0, len(src)
    line_starts = [0]
    for m in re.finditer(r"\n", src):
        line_starts.append(m.end())

    def pos(off):
        import bisect
        ln = bisect.bisect_right(line_starts, off) - 1
        return ln + 1, off - line_starts[ln] + 1

    at_line_start = True
    while i < n:
        if at_line_start:
            m = re.match(r"[ \t]*#", src[i:])
            if m:  # preprocessor directive, with backslash continuations: opaque
                j = i
                while True:
                    k = src.find("\n", j)
                    if k < 0:
                        j = n
                        break
                    if src[k - 1] == "\\":
                        j = k + 1
                        continue
                    j = k
                    break
                i = j
                continue
        m = _LEX.match(src, i)
        if not m:
            ln, col = pos(i)
            raise ODError(Tok("?", src[i], i, i + 1, ln, col), "cannot tokenize", path)
        kind = m.lastgroup if m.lastgroup != "delim" else "raw"
        if m.group("raw"):
            kind = "raw"
        text = m.group(0)
        if kind == "ws":
            at_line_start = "\n" in text
        elif kind in ("lc", "bc"):
            at_line_start = kind == "lc"
        else:
            ln, col = pos(i)
            toks.append(Tok(kind, text, i, m.end(), ln, col))
            at_line_start = False
        i = m.end()
    return toks


# ---------------------------------------------------------------- translator
class Translator:
    def __init__(self, src, path="<input>", report=False):
        self.src, self.path, self.report = src, path, report
        self.t = lex(src, path)
        self.match = self._match_groups()
        self.subst = {}        # token index -> replacement text
        self.drop = set()      # token indices removed
        self.pre = {}          # token index -> text inserted before the token
        self.span = {}         # first token index -> (last token index, replacement text)
        self.packs = set()     # parameter packs in scope at the site being lowered
        self._last_operand = None
        self.gap_override = {} # token index -> replacement for the whitespace/comments that follow it
        self.log = []

    def err(self, i, msg):
        return ODError(self.t[i] if i is not None and 0 <= i < len(self.t) else None, msg, self.path)

    def txt(self, i):
        return self.t[i].text

    # ---- bracket matching: (), [], {} over the whole file; <> on demand, in type contexts only
    def _match_groups(self):
        m, st = {}, []
        pairs = {")": "(", "]": "[", "}": "{"}
        for i, tk in enumerate(self.t):
            if tk.text in "([{" and tk.kind == "punct":
                st.append(i)
            elif tk.text in ")]}" and tk.kind == "punct":
                if not st or self.t[st[-1]].text != pairs[tk.text]:
                    raise self.err(i, f"unbalanced '{tk.text}'")
                j = st.pop()
                m[i], m[j] = j, i
        if st:
            raise self.err(st[-1], f"unclosed '{self.t[st[-1]].text}'")
        return m

    def angle_fwd(self, i, stop=None):
        """tokens[i] is '<': index of the matching '>' (parens/brackets/braces skipped), or None."""
        d, j = 0, i
        stop = len(self.t) if stop is None else stop
        while j < stop:
            x = self.t[j].text
            if x in ("(", "[", "{"):
                j = self.match[j]
            elif x == "<":
                d += 1
            elif x == ">":
                d -= 1
                if d == 0:
                    return j
            elif x in (";", ")", "]", "}"):
                return None
            j += 1
        return None

    def angle_back(self, i):
        """tokens[i] is '>': index of the matching '<', or None."""
        d, j = 0, i
        while j >= 0:
            x = self.t[j].text
            if x in (")", "]", "}"):
                j = self.match[j]
            elif x == ">":
                d += 1
            elif x == "<":
                d -= 1
                if d == 0:
                    return j
            elif x in (";", "(", "[", "{"):
                return None
            j -= 1
        return None

    def template_head_before(self, i):
        """if tokens[i-1] closes `template<...>`, return (template_idx, lt_idx, gt_idx)."""
        if i - 1 < 0 or self.txt(i - 1) != ">":
            return None
        lt = self.angle_back(i - 1)
        if lt is None or lt == 0 or self.txt(lt - 1) != "template":
            return None
        return lt - 1, lt, i - 1

    def packs_of(self, lt, gt):
        """names of the parameter packs declared in template<...> between lt and gt."""
        packs = set()
        for j in range(lt + 1, gt):
            if self.txt(j) == "..." and j + 1 < gt and self.t[j + 1].kind == "id":
                packs.add(self.txt(j + 1))
        return packs

    def params_of(self, lt, gt):
        names, cur = [], []
        j = lt + 1
        while j <= gt:
            x = self.txt(j)
            if j == gt or x == ",":
                ids = []
                for k in cur:
                    if self.txt(k) == "=":
                        break
                    if self.t[k].kind == "id" and self.txt(k) not in KEYWORDS_NOT_NAMES:
                        ids.append(k)
                if ids:
                    names.append(self.txt(ids[-1]))
                cur = []
            elif x in ("(", "[", "{"):
                cur.extend(range(j, self.match[j] + 1))
                j = self.match[j]
            elif x == "<":
                e = self.angle_fwd(j, gt + 1) or j
                cur.extend(range(j, e + 1))
                j = e
            else:
                cur.append(j)
            j += 1
        return names

    # ---- structure: class definitions and brace kinds
    def scan_classes(self):
        self.classes = []   # dicts
        self.by_open = {}
        t = self.t
        for k, tk in enumerate(t):
            if tk.kind != "id" or tk.text not in ("struct", "class", "union"):
                continue
            if k > 0 and t[k - 1].text in ("enum", "friend"):
                continue
            j = k + 1
            while j < len(t) and t[j].text == "[" and self.match.get(j):   # [[attributes]]
                j = self.match[j] + 1
            if j < len(t) and t[j].text == "alignas":
                j = self.match[j + 1] + 1
            name, name_i = None, None
            if j < len(t) and t[j].kind == "id":
                name, name_i = t[j].text, j
                j += 1
                if j < len(t) and t[j].text == "<":        # specialization
                    e = self.angle_fwd(j)
                    if e is None:
                        continue
                    j = e + 1
                if j < len(t) and t[j].text == "final":
                    j += 1
            base_i = None
            if j < len(t) and t[j].text == ":":
                base_i = j
                j += 1
                ok = False
                while j < len(t):
                    x = t[j].text
                    if x == "{":
                        ok = True
                        break
                    if x in (";", ")", "]", "}", "="):
                        break
                    if x in ("(", "["):
                        j = self.match[j]
                    elif x == "<":
                        e = self.angle_fwd(j)
                        if e is None:
                            break
                        j = e
                    j += 1
                if not ok:
                    continue
            if j >= len(t) or t[j].text != "{":
                continue
            head = self.template_head_before(k)
            c = dict(kw=k, name=name, name_i=name_i, base_i=base_i, open=j, close=self.match[j],
                     head=head, params=self.params_of(head[1], head[2]) if head else [],
                     packs=self.packs_of(head[1], head[2]) if head else set())
            self.classes.append(c)
            self.by_open[j] = c

        # innermost class / namespace-scope for every token
        self.owner = [None] * len(t)
        self.ns_scope = [True] * len(t)
        stack = []     # entries: ('class', c) | ('ns', None) | ('block', None)
        for i, tk in enumerate(t):
            if tk.text == "}" and tk.kind == "punct" and stack:
                stack.pop()
            cls = next((c for kind, c in reversed(stack) if kind == "class"), None)
            self.owner[i] = cls
            self.ns_scope[i] = all(kind == "ns" for kind, _ in stack)
            if tk.text == "{" and tk.kind == "punct":
                if i in self.by_open:
                    stack.append(("class", self.by_open[i]))
                elif self._opens_namespace(i):
                    stack.append(("ns", None))
                else:
                    stack.append(("block", None))

    def _opens_namespace(self, i):
        j = i - 1
        while j >= 0 and (self.t[j].kind == "id" or self.txt(j) == "::"):
            if self.txt(j) == "namespace":
                return True
            j -= 1
        return j >= 0 and self.t[j].kind == "str" and j > 0 and self.txt(j - 1) == "extern"

    def params_in_scope(self, i):
        names = set()
        for c in self.classes:
            if c["open"] < i < c["close"]:
                names.update(c["params"])
        return names

    def unqualified(self, i):
        return i == 0 or self.txt(i - 1) not in (".", "->", "::")

    # ---- rule 7: which classes are open (name `super`, and ordinary lookup does not find one)
    def classify(self):
        t = self.t
        # a namespace-scope declaration of `super` makes every later `super` an ordinary name (approximation: by position)
        self.ordinary_super_from = None
        for i, tk in enumerate(t):
            if tk.text == "super" and self.ns_scope[i] and self._declares_super(i):
                self.ordinary_super_from = i
                print(f"{self.path}:{tk.line}:{tk.col}: warning: `super` declared at namespace scope; "
                      f"later uses of `super` are ordinary names, not the open base (rule 7)", file=sys.stderr)
                break
        for c in self.classes:
            c["is_open"] = False
            c["super_uses"] = []
            c["user_super"] = False
        for i, tk in enumerate(t):
            if tk.text != "super" or tk.kind != "id" or not self.unqualified(i):
                continue
            c = self.owner[i]
            if self.ordinary_super_from is not None and i > self.ordinary_super_from:
                continue
            if c is None:
                raise self.err(i, "[od-rule7] `super` used outside a class body")
            if self._declares_super(i) and self.owner[i] is c:
                c["user_super"] = True
                continue
            c["super_uses"].append(i)
        for c in self.classes:
            if c["user_super"]:
                c["super_uses"] = []   # ordinary lookup finds the user's `super`: that one wins
                continue
            c["is_open"] = bool(c["super_uses"])

    def _declares_super(self, i):
        """is tokens[i] (`super`) the declarator of a typedef / alias / class named super?"""
        t = self.t
        if i + 1 < len(t) and t[i + 1].text == "=" and i > 0 and t[i - 1].text == "using":
            return True
        if i + 1 < len(t) and t[i + 1].text == ";":
            j = i - 1
            while j >= 0 and t[j].text not in (";", "{", "}"):
                if t[j].text == "typedef":
                    return True
                j -= 1
        if i > 0 and t[i - 1].text in ("struct", "class") and i + 1 < len(t) and t[i + 1].text in ("{", ":", ";"):
            return True
        return False

    # ---- lowering of an open class: the Part holder
    def lower_part(self, c):
        t = self.t
        name = c["name"]
        if name is None:
            raise self.err(c["kw"], "[od-rule5] an anonymous class cannot be a left operand of ':'")
        if self.txt(c["kw"]) == "union":
            raise self.err(c["kw"], f"[od-rule5] union '{name}' cannot be a left operand of ':'")
        if c["base_i"] is not None:
            raise self.err(c["base_i"], f"[od-rule8] '{name}' already has a base and uses `super`: "
                                        f"rebasing a class that has a base is rejected (A:E with `class A : Y`)")
        if name in ("O", "Base", "Part"):
            raise self.err(c["name_i"], f"class name '{name}' collides with the names the Part lowering introduces")
        for p in c["params"]:
            if p in ("O", "Base", "Part"):
                raise self.err(c["head"][1], f"template parameter '{p}' of '{name}' collides with the Part lowering")
        for i in range(c["open"] + 1, c["close"]):
            if t[i].kind == "id" and t[i].text in ("Base", "Part") and self.unqualified(i):
                raise self.err(i, f"'{t[i].text}' inside open class '{name}' would be captured by the Part lowering "
                                  f"(write `super` for the base)")
        self.check_out_of_line(c)

        # super -> Base; `using super::super;` is implicit (rule 6) and dropped
        for i in c["super_uses"]:
            if (t[i - 1].text == "using" and i + 3 < len(t) and t[i + 1].text == "::"
                    and t[i + 2].text == "super" and t[i + 3].text == ";"):
                for k in range(i - 1, i + 4):
                    self.drop.add(k)
                continue
            if i in self.drop:
                continue
            self.subst[i] = "Base"
        # the injected-class-name, unqualified and without template arguments, is rebound to A:B (rule 1)
        for i in range(c["open"] + 1, c["close"]):
            if (t[i].kind == "id" and t[i].text == name and self.unqualified(i)
                    and not (i + 1 < len(t) and t[i + 1].text == "<")):
                self.subst[i] = "Part"
        # holder
        o = c["open"]
        body_indent = self._indent_after(o)
        self.pre.setdefault(o + 1, "")
        g = self._gap(o, o + 1)
        same_line, nl, rest = g.partition("\n")   # a comment on the brace's line stays there
        if not nl:
            same_line, rest = "", g or " "
        self.pre[o + 1] = ("template<typename O> struct Part:O {" + same_line + "\n" + body_indent
                           + "using Base=O; using Base::Base;" + ("\n" if nl else "") + rest + self.pre[o + 1])
        self.gap_override[o] = ""   # the gap after '{' was moved after the inserted line
        cl = c["close"]
        hoisted = self.hoist_rules(c)
        self.pre[cl] = self.pre.get(cl, "") + "};" + hoisted
        self.log.append(f"{self.path}:{t[c['kw']].line}: part  {name}  ({len(c['super_uses'])} super"
                        f"{', rules() kept on the holder' if hoisted else ''})")

    def hoist_rules(self, c):
        """HAPI's rule walk (BuildRules) asks the HOLDER for rules<Before,After>(): a member named `rules` stays outside
        Part, verbatim (inside it the class's own name means the holder, the layer type the rule lists hold)."""
        t, out = self.t, []
        i, start = c["open"] + 1, c["open"] + 1
        while i < c["close"]:
            x = t[i].text
            if x in ("(", "["):
                i = self.match[i] + 1
                continue
            if x == ";" or (x == ":" and t[i - 1].text in ("public", "private", "protected")):
                start = i + 1
            elif x == "{":
                end = self.match[i]
                names = [k for k in range(start, i) if t[k].text == "rules" and k + 1 < i and t[k + 1].text == "("]
                if names:
                    for k in range(start, end + 1):
                        if k in c["super_uses"]:
                            raise self.err(k, f"`super` inside rules() of '{c['name']}': rules are asked of the holder, "
                                              f"which has no base")
                        self.drop.add(k)
                        self.subst.pop(k, None)
                        if k < end:
                            self.gap_override[k] = ""
                    out.append(self.src[t[start].start:t[end].end])
                i = end + 1
                start = i
                continue
            i += 1
        return "".join(" " + r for r in out)

    def _gap(self, a, b):
        return self.src[self.t[a].end:self.t[b].start]

    def _indent_after(self, o):
        """indentation of the first body line (a single space when the body starts on the brace's line)."""
        g = self._gap(o, o + 1)
        last = g.rsplit("\n", 1)[-1] if "\n" in g else None
        return last if last is not None and last.strip() == "" else " "

    def check_out_of_line(self, c):
        t, name = self.t, c["name"]
        # a member declared in the body with no definition there (its definition would be in another TU)
        i = c["open"] + 1
        stmt = []
        while i < c["close"]:
            x = t[i].text
            if x == "{" :
                i = self.match[i] + 1
                stmt = []
                continue
            if x in ("(", "["):
                stmt.append(i)
                i = self.match[i] + 1
                continue
            if x == ";" or x == ":" and i > 0 and t[i - 1].text in ("public", "private", "protected"):
                self._check_decl_stmt(c, stmt)
                stmt = []
            else:
                stmt.append(i)
            i += 1
        # a definition of one of its members at namespace scope
        for i, tk in enumerate(t):
            if (tk.text == name and tk.kind == "id" and self.ns_scope[i] and self.unqualified(i)
                    and not (c["open"] <= i <= c["close"]) and i != c["name_i"]):
                j = i + 1
                if j < len(t) and t[j].text == "<":
                    e = self.angle_fwd(j)
                    j = (e + 1) if e is not None else j
                if j + 1 < len(t) and t[j].text == "::" and (t[j + 1].kind == "id" or t[j + 1].text == "~"):
                    if not self._in_using(i):
                        raise self.err(i, f"[od-scope] member of open class '{name}' defined out of line: "
                                          f"classes with out-of-line members are out of scope for this translator")

    def _in_using(self, i):
        j = i
        while j >= 0 and self.txt(j) not in (";", "{", "}"):
            if self.txt(j) in ("using", "typedef", "static_assert", "decltype", "sizeof"):
                return True
            j -= 1
        return False

    def _check_decl_stmt(self, c, stmt):
        if not stmt:
            return
        words = [self.txt(k) for k in stmt]
        if words[0] in ("using", "typedef", "friend", "static_assert", "public", "private", "protected"):
            return
        if "=" in words:
            return
        # function declarator: an identifier (or ~X / operator...) directly followed by a (...) group
        for n, k in enumerate(stmt):
            if self.txt(k) == "(" and n > 0:
                prev = self.txt(stmt[n - 1])
                if self.t[stmt[n - 1]].kind == "id" and prev not in ("decltype", "alignas", "noexcept", "sizeof"):
                    raise self.err(stmt[0], f"[od-scope] member '{prev}' of open class '{c['name']}' is declared "
                                            f"but not defined in the class: out-of-line members (.cpp) are out of scope")
                return

    # ---- `:` expressions (rule 4: alias RHS and base clause only)
    def find_sites(self):
        t = self.t
        sites = []
        for i, tk in enumerate(t):
            if tk.text == "using" and i + 2 < len(t) and t[i + 1].kind == "id" and t[i + 1].text != "namespace":
                j = i + 2
                while j < len(t) and t[j].text == "[" and self.match.get(j):
                    j = self.match[j] + 1
                if j < len(t) and t[j].text == "=":
                    e = j + 1
                    while e < len(t) and t[e].text != ";":
                        if t[e].text in ("(", "[", "{"):
                            e = self.match[e]
                        e += 1
                    head = self.template_head_before(i)
                    sites.append(("alias", j + 1, e, set(self.params_of(head[1], head[2])) if head else set(),
                                  self.packs_of(head[1], head[2]) if head else set(), None))
        for c in self.classes:
            if c["base_i"] is None:
                continue
            a = c["base_i"] + 1
            e = c["open"]
            # base-specifier-list: split on depth-0 commas
            cur = a
            j = a
            while j <= e:
                x = t[j].text
                if j == e or x == ",":
                    s = cur
                    while s < j and t[s].text in ("public", "protected", "private", "virtual"):
                        s += 1
                    if s < j:
                        sites.append(("base", s, j, set(c["params"]), set(c["packs"]), c))
                    cur = j + 1
                elif x in ("(", "["):
                    j = self.match[j]
                elif x == "<":
                    j = self.angle_fwd(j) or j
                j += 1
        return sites

    def split_colons(self, a, b):
        """depth-0 ':' positions in [a,b), or raise on stray tokens; angles are tracked (type context)."""
        cols, j = [], a
        while j < b:
            x = self.txt(j)
            if x in ("(", "[", "{"):
                j = self.match[j]
            elif x == "<":
                e = self.angle_fwd(j, b)
                if e is None:
                    raise self.err(j, "unbalanced '<' in a ':' expression")
                j = e
            elif x == ":":
                cols.append(j)
            j += 1
        return cols

    def is_paren_group(self, a, b):
        return b - a >= 2 and self.txt(a) == "(" and self.match[a] == b - 1

    def parse_chain(self, a, b, top=True):
        """returns ('ops', [items], terminal) or None when [a,b) is not a ':' expression.
        items are strings (a pack element is rendered 'P...'); dependent flag is computed by the caller."""
        cols = self.split_colons(a, b)
        if not cols and top and self.txt(a) == "final":
            self._last_operand = (a + 1, b)
            return [], self.render(a, b)        # struct X : final T {}: closed on T, no layers
        if not cols:
            if self.is_paren_group(a, b):
                inner = self.parse_chain(a + 1, b - 1, top=False)
                if inner is not None:
                    return inner
            return None
        bounds = [a] + [c + 1 for c in cols]
        ends = cols + [b]
        ops = list(zip(bounds, ends))
        for s, e in ops:
            if s >= e:
                raise self.err(s if s < len(self.t) else s - 1, "empty operand of ':'")
        is_fold = any(e - s == 1 and self.txt(s) == "..." for s, e in ops)
        if is_fold:
            if top:
                raise self.err(a, "[od-rule9] a pack fold over ':' must be parenthesized: (PP : ... : T)")
            if not (len(ops) >= 3 and self.txt(ops[1][0]) == "..." and ops[1][1] - ops[1][0] == 1):
                raise self.err(a, "[od-rule9] only the right fold (PP : ... : T) / (PP : ... : P : T) is supported")
            if any(e - s == 1 and self.txt(s) == "..." for s, e in ops[2:]):
                raise self.err(a, "[od-rule9] one '...' per fold")
            mentions = lambda s, e: any(self.txt(k) in self.packs for k in range(s, e))
            if not mentions(*ops[0]):
                what = "a left fold (T : ... : PP)" if any(mentions(*op) for op in ops[2:]) else \
                       f"'{self.render(*ops[0])}' is not a template parameter pack"
                raise self.err(ops[0][0], f"[od-rule9] only the right fold (PP : ... : T) / (PP : ... : P : T) "
                                          f"is supported: {what}")
            ops = [ops[0], None] + ops[2:]
        items = []
        for n, op in enumerate(ops):
            if op is None:
                items[-1] = items[-1] + "..."
                continue
            s, e = op
            last = n == len(ops) - 1
            if self.is_paren_group(s, e) and self.split_colons(s + 1, e - 1):
                sub = self.parse_chain(s + 1, e - 1, top=False)
                if not last:
                    raise self.err(s, "[od-rule8] a composed class as a left operand ((A:B):C) is a rebase of A:B, rejected")
                items.extend(sub[0])
                terminal = sub[1]
                return items, terminal
            if last:
                self._last_operand = (s + 1, e) if self.txt(s) == "final" else (s, e)
                return items, self.render(s, e)
            if self.txt(s) == "final":
                raise self.err(s, "[od-final] `final` marks the terminal: only the last operand of ':' can be `final T`")
            items.append(self.render(s, e))
            self.check_left_operand(s, e)
        raise AssertionError

    def check_left_operand(self, s, e, closing_hint=False):
        t = self.t
        if t[s].kind != "id":
            return
        k = s
        while k + 2 < e and t[k + 1].text == "::" and t[k + 2].kind == "id":
            k += 2
        nm = t[k].text
        rest = e - (k + 1)
        if rest != 0 and not (t[k + 1].text == "<" and self.angle_fwd(k + 1, e) == e - 1):
            return
        defs = [c for c in self.classes if c["name"] == nm]
        if defs and not any(c["is_open"] or c.get("is_component") for c in defs):
            c = defs[0]
            if c.get("chain_bases"):
                raise self.err(s, f"[od-rule8] '{nm}' is a closed composition (it ends on `final ...`): only a component "
                                  f"(a chain with no `final`) can be a layer")
            if c["base_i"] is not None:
                raise self.err(s, f"[od-rule8] '{nm}' already has a base: rebasing it ({nm}:X) is rejected")
            if closing_hint:
                raise self.err(s, f"[od-final] '{nm}' is closed (its body never names `super`): close the composition "
                                  f"on it with `final {nm}`, or end the chain on an open layer (a component)")
            raise self.err(s, f"[od-rule7] '{nm}' is closed (its body never names `super`): only the terminal "
                              f"(`final {nm}`) may be closed")

    def render(self, s, e):
        out = []
        for k in range(s, e):
            if k in self.drop:
                continue
            if k > s:
                g = self._gap(k - 1, k)
                out.append(" " if g.strip() == "" and g else g)
            out.append(self.subst.get(k, self.txt(k)))
        return "".join(out).strip()

    def lower_site(self, kind, a, b, alias_params):
        res = self.parse_chain(a, b)
        if res is None:
            return None
        if kind == "alias":
            raise self.err(a, "[od-rule4] ':' is only valid in a base clause; name the composition: "
                              "struct X : A:B {};")
        items, terminal = res
        # the syntax says it: `final T` as the last operand closes the composition on the terminal API T, and APIOf starts
        # the Part collapse there (hapi::APIOf<T,layers...>); without `final` the chain stays open: a component
        # (hapi::Chain<layers...>), reusable as a layer and closed later by whoever uses it
        closed = terminal.startswith("final ")
        if closed:
            terminal = terminal[len("final "):].strip()
            text = f"hapi::APIOf<{terminal}{''.join(',' + x for x in items)}>"
            operands = items + [terminal]
        else:
            self.check_left_operand(*self._last_operand, closing_hint=True)
            items = items + [terminal]
            text = f"hapi::Chain<{','.join(items)}>"
            operands = items
        self.span[a] = (b - 1, text)
        self.log.append(f"{self.path}:{self.t[a].line}: {kind:5} {self.render(a, b)}  ->  {text}")
        return text, not closed, operands

    @staticmethod
    def _norm(x):
        return re.sub(r"\s+", "", x)

    def is_chain(self, s, e):
        """does the base-specifier [s,e) use ':' (a chain, or a parenthesized chain / fold)?"""
        if self.split_colons(s, e) or self.txt(s) == "final":
            return True
        return self.is_paren_group(s, e) and bool(self.split_colons(s + 1, e - 1))

    # ---- a class whose base clause is a ':' chain: implicit constructor inheritance, `super` = that base
    def mark_chain_bases(self, sites):
        for kind, s, e, _, _, c in sites:
            if kind == "base" and self.is_chain(s, e):
                c.setdefault("chain_bases", []).append(s)
        for c in self.classes:
            cb = c.get("chain_bases", [])
            if len(cb) > 1:
                raise self.err(cb[1], f"[od-rule6] '{c['name']}' has {len(cb)} ':' bases: the implicit `Base` "
                                      f"(constructor inheritance, `super`) would be ambiguous")
            if cb:
                c["is_open"] = False   # a named composition: concrete, its `super` is the base it names
                for i in range(c["open"] + 1, c["close"]):
                    if self.t[i].kind == "id" and self.txt(i) == "Base" and self.unqualified(i) and self.owner[i] is c:
                        raise self.err(i, f"'Base' inside '{c['name']}' would be captured by the implicit "
                                          f"`using Base=...; using Base::Base;` (write `super` for the base)")

    def lower_chain_based(self, c, base_text, component, operands):
        t = self.t
        name = c["name"]
        # duplicate layers: checked by the compiler, on exact types (hapi::Distinct, rules.h)
        distinct = f"static_assert(hapi::Distinct<hapi::Chain<{','.join(operands)}>>, \"duplicate layer in {name}\");"
        if component:
            # a component is a type list: its own members would not be part of any composed object
            if c["close"] > c["open"] + 1:
                raise self.err(c["open"] + 1, f"[od-component] '{name}' is a component (its chain ends open): members "
                                              f"in its body would not be part of the composed object; put them in a layer, "
                                              f"or close it where it is used (struct X : {name}:final Terminal {{}})")
            line = distinct
        else:
            for i in c["super_uses"]:
                if (t[i - 1].text == "using" and i + 3 < len(t) and t[i + 1].text == "::"
                        and t[i + 2].text == "super" and t[i + 3].text == ";"):
                    for k in range(i - 1, i + 4):
                        self.drop.add(k)
                    continue
                if i not in self.drop:
                    self.subst[i] = "Base"
            # the rule walk (components' rules<Before,After>()) is APIOf's own static_assert
            line = f"using Base={base_text}; using Base::Base; " + distinct
        o = c["open"]
        g = self._gap(o, o + 1)
        same_line, nl, rest = g.partition("\n")
        text = (same_line + "\n" + self._indent_after(o) + line + "\n" + rest) if nl else (line + g)
        self.pre[o + 1] = text + self.pre.get(o + 1, "")
        self.gap_override[o] = ""
        what = "component" if component else f"closed by APIOf, Base + ctors, {len(c['super_uses'])} super"
        self.log.append(f"{self.path}:{t[c['kw']].line}: named {name}  ({what})")

    # ---- driver
    def run(self):
        self.scan_classes()
        self.classify()
        sites = self.find_sites()
        self.mark_chain_bases(sites)
        for c in self.classes:
            if c["is_open"]:
                self.lower_part(c)
        for kind, a, b, ap, pk, c in sites:
            self.packs = pk | {p for k in self.classes if k["open"] < a < k["close"] for p in k["packs"]}
            r = self.lower_site(kind, a, b, ap)
            if r is not None and kind == "base":
                c["is_component"] = r[1]
                self.lower_chain_based(c, *r)
        return self.emit()

    def emit(self):
        out = []
        t = self.t
        prev_end = 0
        gap_over = self.gap_override
        i = 0
        last = -1
        while i < len(t):
            gap = self.src[prev_end:t[i].start]
            if last in gap_over:
                gap = gap_over[last]
            out.append(gap)
            if i in self.pre:
                out.append(self.pre[i])
            if i in self.span:
                e, text = self.span[i]
                out.append(text)
                prev_end, last, i = t[e].end, e, e + 1
                continue
            if i not in self.drop:
                out.append(self.subst.get(i, t[i].text))
            prev_end, last = t[i].end, i
            i += 1
        out.append(self.src[prev_end:])
        return "".join(out)


def translate(src, path="<input>", report=False):
    tr = Translator(src, path, report)
    out = tr.run()
    if report:
        for line in tr.log:
            print(line, file=sys.stderr)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inputs", nargs="+")
    ap.add_argument("-o", "--output")
    ap.add_argument("--outdir")
    ap.add_argument("--report", action="store_true")
    a = ap.parse_args(argv)
    if a.output and len(a.inputs) != 1:
        ap.error("-o takes one input; use --outdir for several")
    status = 0
    for p in a.inputs:
        try:
            with open(p) as f:
                src = f.read()
            out = translate(src, p, a.report)
        except ODError as e:
            print(e, file=sys.stderr)
            status = 1
            continue
        if a.outdir:
            os.makedirs(a.outdir, exist_ok=True)
            with open(os.path.join(a.outdir, os.path.basename(p)), "w") as f:
                f.write(out)
        elif a.output:
            with open(a.output, "w") as f:
                f.write(out)
        else:
            sys.stdout.write(out)
    return status


if __name__ == "__main__":
    sys.exit(main())
