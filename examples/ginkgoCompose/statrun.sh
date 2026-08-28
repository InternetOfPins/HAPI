#!/usr/bin/env bash
# Paired, interleaved timing for the stencil apply() delta. On a box
# without a fixed CPU governor, absolute ns/apply is noisy; the paired
# difference base_i - fixed_i is not (each pair runs back-to-back).
# Reports median + a trimmed mean over N trials. Core freq logged.
#
#   ./statrun.sh [trials]     (default 25; needs ./setup_ginkgo.sh first)
set -euo pipefail
cd "$(dirname "$0")"

GK=${GINKGO_DIR:-$HOME/ginkgo-src}; GKB=$GK/build
[ -d "$GKB/lib" ] || { echo "run ./setup_ginkgo.sh first"; exit 1; }
INC="-I$GK/include -I$GKB/include -I../../include"
LIB="-L$GKB/lib -Wl,-rpath,$GKB/lib -lginkgo -lginkgo_reference -lginkgo_omp \
     -lginkgo_cuda -lginkgo_hip -lginkgo_dpcpp -lginkgo_device"
CORE=${CORE:-2}; TRIALS=${1:-25}
FREQ=/sys/devices/system/cpu/cpu$CORE/cpufreq/scaling_cur_freq

g++ -std=c++17 -O2 $INC             src/stencil.cpp -o /tmp/gc_base  $LIB
g++ -std=c++17 -O2 $INC -DFIXED_EXEC src/stencil.cpp -o /tmp/gc_fixed $LIB

: > /tmp/gc_statrun.dat
for t in $(seq 1 "$TRIALS"); do
  f0=$([ -r "$FREQ" ] && cat "$FREQ" || echo 0)
  b=$(taskset -c $CORE /tmp/gc_base  | awk '/ns\/apply/{print $(NF-1)}')
  h=$(taskset -c $CORE /tmp/gc_fixed | awk '/ns\/apply/{print $(NF-1)}')
  f1=$([ -r "$FREQ" ] && cat "$FREQ" || echo 0)
  echo "$b $h $f0 $f1" | tee -a /tmp/gc_statrun.dat
done

python3 - <<'PY'
import statistics as st
r=[list(map(float,l.split())) for l in open('/tmp/gc_statrun.dat') if l.strip()]
b=[x[0] for x in r]; h=[x[1] for x in r]; d=sorted(x-y for x,y in zip(b,h))
n=len(d); k=max(1,n//10); trim=st.mean(d[k:-k])
f=[x[2]/1e6 for x in r if x[2]]+[x[3]/1e6 for x in r if x[3]]
print(f"n={n}  baseline median {st.median(b):.1f}  fixed-exec median {st.median(h):.1f} ns")
print(f"paired baseline-fixed: median {st.median(d):.1f}  10%-trimmed mean {trim:.1f} ns")
print(f"  spread: {[round(x,1) for x in d]}")
if f: print(f"  pinned-core freq seen: {min(f):.2f}-{max(f):.2f} GHz")
PY
