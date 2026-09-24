#!/usr/bin/env python3
"""emlearn's inline MLP, as found while building the Banknote comparison (emlearn 0.23.2 from PyPI; the same code is on master, whose VERSION.txt is also 0.23.2).

    python emlearn_repro.py              what 0.23.2 does
    python emlearn_repro.py --with-fix   the same checks on a temporary copy of the package with two one-line changes (nothing installed is modified)
Needs numpy, scikit-learn, emlearn (and setuptools on Python 3.12, for the distutils that emlearn imports).

Findings:
  A. MLP convert(..., method='inline') silently returns the LOADABLE code: Wrapper.__init__ (net.py) calls self.save(name=name) with save()'s default
     inference=['loadable'], for the inline branch too. For floats it happens to work (the loadable code defines the same predict function), so the inline
     test passes for the wrong reason.
  B. Asking for real inline code, save(name, inference=['inline']), raises ValueError: c_generate_layer_data calls cgen.constant_declare(name, 'EmlNetActivationRelu')
     with the default dtype='int'.
  C. use_fixedpoint=True: convert() fails to compile (consequence of A: the fixed-point snippet is appended to loadable code), but behind that the fixed-point path
     is unfinished: eml_net_forward_q16 in eml_net_fixedpoint.h has its multiply, relu, logistic and softmax under '#if 0 / FIXME: implement', and emlearn's own
     test_sklearn_predict_fixedpoint is marked xfail. So C is work in progress, not a bug.
  The two one-line changes (--with-fix): A: self.save(name=name, inference=['inline'])   B: cgen.constant_declare(..., dtype='EmlNetActivationFunction')
  With them the inline float MLP generates real inline code and agrees with sklearn."""
import importlib.util, os, shutil, sys, tempfile, warnings
import numpy as np

WITH_FIX = '--with-fix' in sys.argv
warnings.filterwarnings('ignore')
np.seterr(all='ignore')                       # emlearn turns numpy's floating-point errors into exceptions; unrelated to the findings

pkg = os.path.dirname(importlib.util.find_spec('emlearn').origin)
if WITH_FIX:
    tmp = tempfile.mkdtemp()
    shutil.copytree(pkg, os.path.join(tmp, 'emlearn'))
    p = os.path.join(tmp, 'emlearn', 'net.py')
    s = open(p).read()
    for old, new in (("            code = self.save(name=name)\n            \n            if self.use_fixedpoint:" if False else
                      "        elif self.inference_type == 'inline' and return_type == 'classifier':\n            code = self.save(name=name)\n",
                      "        elif self.inference_type == 'inline' and return_type == 'classifier':\n            code = self.save(name=name, inference=['inline'])\n"),
                     ("cgen.constant_declare(activation_name, activation_func)",
                      "cgen.constant_declare(activation_name, activation_func, dtype='EmlNetActivationFunction')")):
        assert s.count(old) == 1, 'emlearn changed: %r not found once' % old[:50]
        s = s.replace(old, new)
    open(p, 'w').write(s)
    sys.path.insert(0, tmp)
    pkg = os.path.join(tmp, 'emlearn')

import emlearn
from sklearn.datasets import make_classification
from sklearn.neural_network import MLPClassifier

X, y = make_classification(n_features=4, n_informative=4, n_redundant=0, n_samples=300, random_state=0)
m = MLPClassifier(hidden_layer_sizes=(4,), max_iter=500, random_state=1).fit(X, y)
print('emlearn %s%s' % (open(os.path.join(pkg, 'VERSION.txt')).read().strip(), '  (temporary copy with the two one-line changes)' if WITH_FIX else ''))


def attempt(label, fn):
    try:
        r = fn()
        print('%-62s ok%s' % (label, (': ' + r) if r else ''))
    except Exception as e:
        print('%-62s FAILS: %s: %s' % (label, type(e).__name__, str(e).splitlines()[0][:80]))


state = {}
attempt("convert(mlp, method='inline')", lambda: state.setdefault('c', emlearn.convert(m, method='inline')) and '')
if 'c' in state:
    c = state['c']
    attempt("  .predict(X) agrees with sklearn", lambda: '%d/%d rows' % ((c.predict(X.astype(np.float32)) == m.predict(X)).sum(), len(X)))
    attempt("  .save('net')  (default inference=['loadable'])",
            lambda: 'emits ' + ('the LOADABLE code (EmlNetLayer table)' if 'EmlNetLayer' in c.save('net') else 'inline code'))
    attempt("  .save('net', inference=['inline'])  (the real inline generator)",
            lambda: 'inline code' if 'EmlNetLayer' not in c.save('net', inference=['inline']) else 'but it is loadable code')
fx = {}
attempt("convert(mlp, method='inline', use_fixedpoint=True)", lambda: fx.setdefault('c', emlearn.convert(m, method='inline', use_fixedpoint=True)) and '')
if 'c' in fx:
    attempt("  .predict(X) agrees with sklearn", lambda: '%d/%d rows (the forward pass is a stub, see below)' % ((fx['c'].predict(X.astype(np.float32)) == m.predict(X)).sum(), len(X)))
hdr = open(os.path.join(pkg, 'eml_net_fixedpoint.h')).read()
print('%-62s %d x "FIXME: implement" (multiply, relu, logistic, softmax)' % ('eml_net_fixedpoint.h: eml_net_forward_q16', hdr.count('FIXME: implement')))
