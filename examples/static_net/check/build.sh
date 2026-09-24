#!/bin/bash
# Every check of the example, in one script; each line says what it pins.
#   ./build.sh                        everything the machine can do
# Host checks need g++ (clang++ is used as well when present). The AVR ones need avr-g++, the row-by-row and cycle ones also simavr.
# HAPI=<hapi/include> overrides this repository's include/. Exit status 1 if any check FAILs; a "note" is not a failure.
cd "$(dirname "$0")"
H=${HAPI:-../../../include}
INC=(-I"$H" -I. -I../include -I../models/banknote -I../models/sonar -I../models/roll60)
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
pass=0; fail=0; note=0
ok()   { echo "  ok    $1"; pass=$((pass+1)); }
bad()  { echo "  FAIL  $1: $2"; fail=$((fail+1)); }
info() { echo "  note  $1: $2"; note=$((note+1)); }
have() { command -v "$1" >/dev/null 2>&1; }

# run NAME SRC EXPECT   compile and run on the host; the output must contain EXPECT (g++, and clang++ when present)
run() { local n=$1 s=$2 e=$3 o
  for cxx in g++ clang++; do have $cxx || continue
    $cxx -std=c++17 -O2 -Wall "${INC[@]}" "$s" -o "$W/x" 2>"$W/err" || { bad "$n [$cxx]" "does not compile: $(grep -m1 error "$W/err")"; continue; }
    o=$("$W/x" 2>&1); rc=$?
    if [ $rc -eq 0 ] && printf '%s' "$o" | grep -qF -- "$e"; then ok "$n [$cxx]: $e"; else bad "$n [$cxx]" "exit $rc, output: $(printf '%s' "$o" | head -c 160)"; fi
  done; }
# rej NAME SRC MSG      must NOT compile, and say MSG
rej() { local n=$1 s=$2 m=$3
  for cxx in g++ clang++; do have $cxx || continue
    if $cxx -std=c++17 -fsyntax-only "${INC[@]}" "$s" 2>"$W/err"; then bad "$n [$cxx]" "compiled, but must be rejected"
    elif grep -qiF -- "$m" "$W/err"; then ok "$n [$cxx]: rejected with \"$m\""; else bad "$n [$cxx]" "rejected, but not with \"$m\": $(grep -m1 error "$W/err")"; fi
  done; }
# compile-only checks (static_asserts), also for AVR when avr-g++ is present
syn() { local n=$1 s=$2
  for cxx in g++ clang++; do have $cxx || continue
    $cxx -std=c++17 -fsyntax-only "${INC[@]}" "$s" 2>"$W/err" && ok "$n [$cxx]" || bad "$n [$cxx]" "$(grep -m1 error "$W/err")"; done
  have avr-g++ && { avr-g++ -std=c++17 -mmcu=atmega328p -fsyntax-only "${INC[@]}" "$s" 2>"$W/err" && ok "$n [avr-g++]" || bad "$n [avr-g++]" "$(grep -m1 error "$W/err")"; }; }
# avr NAME SRC EXPECTED_SIZE [flags]   size (avr-gcc 7.3 numbers: a different toolchain may differ, which is only a note) and the disassembly md5 in $W/NAME.md5
avr() { local n=$1 s=$2 e=$3; shift 3
  avr-g++ -std=c++17 -Os -mmcu=atmega328p "$@" "${INC[@]}" "$s" -o "$W/$n.elf" 2>"$W/err" || { bad "$n [avr]" "$(grep -m1 error "$W/err")"; return; }
  local sz; sz=$(avr-size -C --mcu=atmega328p "$W/$n.elf" | awk '/^Program:/{p=$2}/^Data:/{d=$2}END{print p" B / "d" B"}')
  avr-objdump -d "$W/$n.elf" | grep -v 'file format' | cut -f2- | md5sum | cut -c1-12 > "$W/$n.md5"
  if [ "$sz" = "$e" ]; then ok "$n [avr]: $sz"; else info "$n [avr]" "$sz (expected $e with avr-gcc 7.3)"; fi; }
# same NAME A B         two avr() builds are the same program
same() { if [ -s "$W/$2.md5" ] && [ "$(cat "$W/$2.md5")" = "$(cat "$W/$3.md5" 2>/dev/null)" ]; then ok "$1: $2 == $3 (identical disassembly)"; else bad "$1" "$2 and $3 differ"; fi; }

echo "== composition on the host: engines, references, wiring by index / id / query"
run mixed_check          mixed_check.cpp        "ALL OK"
run refid_check          refid_check.cpp        "ALL OK"
run refid_order          refid_order.cpp        "ALL OK"
run refq_check           refq_check.cpp         "ALL OK"
syn refid_ids            refid_ids.cpp
syn net_expand           net_expand.cpp
echo "== what must not build"
rej cycle_reject_self    cycle_reject_self.cpp  "in-place reference must point to a lower net index"
rej cycle_reject_pair    cycle_reject_pair.cpp  "in-place reference must point to a lower net index"
rej refid_cycle          refid_cycle.cpp        "in-place reference must point to a lower net index"
rej refid_missing        refid_missing.cpp      "no cell in the net matches Q"
rej sugar_cycle          sugar_cycle.cpp        "in-place reference must point to a lower net index"
rej sugar_missing        sugar_missing.cpp      "no cell of that type in the net"
echo "== sugar: the net as constexpr factories, and the realization it picks"
run sugar_check          sugar_check.cpp        "ALL OK"
run sugar_twins          sugar_twins.cpp        "twins OK"
run sugar_roll           sugar_roll.cpp         "dense 20000/20000, gapped 20000/20000"
run roll_host            roll_host.cpp          "rolled==unrolled 10000/10000, sparse-order rolled==unrolled 10000/10000"
run roll_gaps            roll_gaps.cpp          "gapped rolled==unrolled 20000/20000"
echo "== the trained models on the host"
run banknote_host_check  banknote_host_check.cpp "274/274"
run sonar_lin_check      sonar_lin_check.cpp    "agreement 100.00"
run host_ref             host_ref.cpp           "narrow rows 010101101100001010010100010100011000101101"

if have avr-g++ && have avr-size; then
echo "== AVR: size, and that sugar / the by-id wiring cost nothing (avr-gcc 7.3 sizes; identical disassembly is checked on any toolchain)"
avr mixed_avr_size       mixed_avr_size.cpp     "304 B / 4 B"
avr refid_avr            refid_avr.cpp          "304 B / 4 B";           same "by id == by index" mixed_avr_size refid_avr
avr sugar_avr            sugar_avr.cpp          "274 B / 4 B"
avr sugar_hand_avr       sugar_hand_avr.cpp     "274 B / 4 B";           same "sugar == by hand" sugar_avr sugar_hand_avr
avr sonar_lin_avr_size       sonar_lin_avr_size.cpp       "1842 B / 61 B"
avr sonar_lin_avr_size_wide  sonar_lin_avr_size_wide.cpp  "1582 B / 61 B"
avr roll_avr_unrolled    roll_avr.cpp           "1784 B / 61 B" -DUSE=Unrolled
avr roll_avr_rolled      roll_avr.cpp           "332 B / 61 B"  -DUSE=Rolled
avr roll_avr_sparse      roll_avr.cpp           "410 B / 61 B"  -DUSE=Sparse
avr sugar_roll_avr       sugar_roll_avr.cpp     "322 B / 62 B"
avr sugar_roll_hand_avr  sugar_roll_hand_avr.cpp "322 B / 62 B";         same "sugar picks the rolled form a person would write" sugar_roll_avr sugar_roll_hand_avr
avr sugar_roll_avr_off   sugar_roll_avr.cpp     "1006 B / 62 B" -DSUGAR_ROLL_AT=1000
avr sugar_unroll_hand_avr sugar_unroll_hand_avr.cpp "1006 B / 62 B";     same "SUGAR_ROLL_AT huge == the unrolled cell by hand" sugar_roll_avr_off sugar_unroll_hand_avr
avr banknote_avr_check   banknote_avr_check.cpp "2346 B / 26 B"     # the Banknote cell on a board: it reports over UART
if have simavr && have python3; then
echo "== AVR: the same rows through the same function, simulated ATmega328p against the host, row by row"
  avr-g++ -std=c++17 -Os -mmcu=atmega328p -DSIM_ONCE "${INC[@]}" sonar_rows_avr.cpp -o "$W/rows.elf" 2>"$W/err" || bad sonar_rows_avr "$(grep -m1 error "$W/err")"
  timeout 30 simavr -m atmega328p -f 16000000 "$W/rows.elf" 2>&1 | sed -E 's/\x1b\[[0-9;]*m//g' > "$W/rows.txt"
  r=$(HAPI="$H" python3 bitexact.py --log "$W/rows.txt" 2>&1 | tail -1)
  case "$r" in BIT-EXACT*) ok "sonar_rows_avr in simavr: $r";; *) bad sonar_rows_avr_sim "$r";; esac
fi
else
  info "AVR checks" "avr-g++ not found: skipped (sizes, sugar/by-id cost, simulated rows)"
fi

echo
echo "$pass ok, $fail FAIL, $note note(s)"
[ $fail -eq 0 ]
