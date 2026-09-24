#!/bin/sh
# cycle-accurate timing on simavr 1.6 (Ubuntu), ATmega328p @16MHz; results printed over simulated UART
# needs the HAPI include path (default: this repo's include/) and ../include, ../models
H=${HAPI:-../../../include}
INC="-I$H -I. -I../include -I../models/banknote -I../models/sonar -I../models/roll60"
for f in bn_wave bn_lin bn_table u32 u16 t60 sonar_lin ru rr rs; do
  avr-gcc -std=c++17 -Os -mmcu=atmega328p $INC $f.cpp -o $f.elf || continue
  timeout 10 simavr -m atmega328p -f 16000000 $f.elf 2>&1 | grep -aoE '[a-z0-9]+:[0-9]+'
done
