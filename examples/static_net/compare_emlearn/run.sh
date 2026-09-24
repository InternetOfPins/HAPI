#!/bin/bash
# Builds every model of the Banknote comparison with ONE set of flags, runs each on simavr (ATmega328p, 16 MHz) through the same harness
# (bnc_common.h: fold 1's held-out rows, one call each, timed with Timer1), and reports flash, RAM, on-device accuracy and cycles together
# (report.py), next to the accuracy on all five folds (results.json, from gen_models.py). Needs avr-g++, simavr, python3; HAPI=<hapi/include>.
#   ./run.sh            all models         ./run.sh null wave4 eml_tree_d5_u8     just these
cd "$(dirname "$0")"
H=${HAPI:-../../../include}
mkdir -p out
declare -A HDR=( [null]=bnc_null.h [wave4]=bnc_wave.h [lin4]=bnc_lin.h [table4]=bnc_table.h )
for f in models/eml_*.h; do n=${f#models/eml_}; HDR[eml_${n%.h}]=$f; done
ORDER=(null wave4 lin4 table4 $(ls models | sed 's/^eml_//; s/\.h$//; s/^/eml_/' | sort -V))
: > out/measure.txt
for n in ${@:-${ORDER[@]}}; do
  h=${HDR[$n]}; [ -n "$h" ] || { echo "unknown model $n" >&2; continue; }
  avr-g++ -std=c++17 -Os -mmcu=atmega328p -ffunction-sections -fdata-sections -Wl,--gc-sections -I. -I../include -I../models/banknote -I../measure -I"$H" -Iemlearn \
    -DBNC_NAME="\"$n\"" -DBNC_MODEL="\"$h\"" bnc.cpp -o out/$n.elf -lm 2> out/$n.err || { echo "$n: BUILD FAILED (out/$n.err)"; continue; }
  sz=$(avr-size -C --mcu=atmega328p out/$n.elf | awk '/^Program:/{p=$2}/^Data:/{d=$2}END{print p" "d}')
  res=$(timeout 30 simavr -m atmega328p -f 16000000 out/$n.elf 2>&1 | grep -aoE "$n: n=[0-9]+ ok=[0-9]+ min=[0-9]+ max=[0-9]+ sum=[0-9]+")
  [ -n "$res" ] || { echo "$n: no result from simavr"; continue; }
  echo "$n $sz $(echo $res | sed -E 's/^[^ ]+ n=([0-9]+) ok=([0-9]+) min=([0-9]+) max=([0-9]+) sum=([0-9]+)/\1 \2 \3 \4 \5/')" >> out/measure.txt
done
python3 report.py | tee results.md
