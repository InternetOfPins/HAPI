# Silicon against simavr, the same ELFs

Run 2026-09-24 with `python3 silicon.py --port /dev/ttyUSB1 --baud 115200 <the ELFs of run.sh and ../compare_emlearn/run.sh>`: each ELF flashed with avrdude (`-c arduino`, 115200) to an
Arduino Nano (ATmega328p, 16 MHz, CH340 USB-serial), its UART report read back and put next to simavr's output for the same ELF. Timer1 counts CPU cycles (prescale 1), so a cycle
count here is a count on the chip.

**All 100 numbers are identical to simavr's: 0 cycles of difference anywhere** (11 numbers of `measure/run.sh`, and for each of the 23 programs of `compare_emlearn/run.sh` the correct count, the min, the max and the
sum of cycles over 274 rows). The timed code has no interrupts, no caches and no wait states on an ATmega328p, so exact agreement is what a cycle-accurate simulator should give; this
confirms it on the chip rather than assuming it.

| target | number | simavr | silicon | diff |
|---|---|---|---|---|
| bn_wave | wave4 | 23 | 23 | +0 |
| bn_lin | lin4 | 33 | 33 | +0 |
| bn_table | table4 | 100 | 100 | +0 |
| u32 | u32 | 2741 | 2741 | +0 |
| u16 | u16 | 959 | 959 | +0 |
| t60 | table60 | 1545 | 1545 | +0 |
| sonar_lin | slnarrow | 917 | 917 | +0 |
| sonar_lin | slwide | 2565 | 2565 | +0 |
| ru | ru | 950 | 950 | +0 |
| rr | rr | 1542 | 1542 | +0 |
| rs | rs | 2143 | 2143 | +0 |
| null | null.ok | 28 | 28 | +0 |
| null | null.min | 19 | 19 | +0 |
| null | null.max | 19 | 19 | +0 |
| null | null.sum | 5206 | 5206 | +0 |
| wave4 | wave4.ok | 274 | 274 | +0 |
| wave4 | wave4.min | 44 | 44 | +0 |
| wave4 | wave4.max | 44 | 44 | +0 |
| wave4 | wave4.sum | 12056 | 12056 | +0 |
| lin4 | lin4.ok | 273 | 273 | +0 |
| lin4 | lin4.min | 81 | 81 | +0 |
| lin4 | lin4.max | 81 | 81 | +0 |
| lin4 | lin4.sum | 22194 | 22194 | +0 |
| table4 | table4.ok | 273 | 273 | +0 |
| table4 | table4.min | 146 | 146 | +0 |
| table4 | table4.max | 146 | 146 | +0 |
| table4 | table4.sum | 40004 | 40004 | +0 |
| eml_mlp_h2 | eml_mlp_h2.ok | 270 | 270 | +0 |
| eml_mlp_h2 | eml_mlp_h2.min | 7997 | 7997 | +0 |
| eml_mlp_h2 | eml_mlp_h2.max | 9164 | 9164 | +0 |
| eml_mlp_h2 | eml_mlp_h2.sum | 2392186 | 2392186 | +0 |
| eml_mlp_h4 | eml_mlp_h4.ok | 273 | 273 | +0 |
| eml_mlp_h4 | eml_mlp_h4.min | 11831 | 11831 | +0 |
| eml_mlp_h4 | eml_mlp_h4.max | 12964 | 12964 | +0 |
| eml_mlp_h4 | eml_mlp_h4.sum | 3447154 | 3447154 | +0 |
| eml_mlp_h8 | eml_mlp_h8.ok | 273 | 273 | +0 |
| eml_mlp_h8 | eml_mlp_h8.min | 18269 | 18269 | +0 |
| eml_mlp_h8 | eml_mlp_h8.max | 19527 | 19527 | +0 |
| eml_mlp_h8 | eml_mlp_h8.sum | 5175831 | 5175831 | +0 |
| eml_mlp_h16 | eml_mlp_h16.ok | 273 | 273 | +0 |
| eml_mlp_h16 | eml_mlp_h16.min | 32317 | 32317 | +0 |
| eml_mlp_h16 | eml_mlp_h16.max | 34524 | 34524 | +0 |
| eml_mlp_h16 | eml_mlp_h16.sum | 9310383 | 9310383 | +0 |
| eml_tree_d2_f32 | eml_tree_d2_f32.ok | 254 | 254 | +0 |
| eml_tree_d2_f32 | eml_tree_d2_f32.min | 534 | 534 | +0 |
| eml_tree_d2_f32 | eml_tree_d2_f32.max | 587 | 587 | +0 |
| eml_tree_d2_f32 | eml_tree_d2_f32.sum | 150650 | 150650 | +0 |
| eml_tree_d2_u8 | eml_tree_d2_u8.ok | 254 | 254 | +0 |
| eml_tree_d2_u8 | eml_tree_d2_u8.min | 192 | 192 | +0 |
| eml_tree_d2_u8 | eml_tree_d2_u8.max | 198 | 198 | +0 |
| eml_tree_d2_u8 | eml_tree_d2_u8.sum | 53580 | 53580 | +0 |
| eml_tree_d3_f32 | eml_tree_d3_f32.ok | 260 | 260 | +0 |
| eml_tree_d3_f32 | eml_tree_d3_f32.min | 544 | 544 | +0 |
| eml_tree_d3_f32 | eml_tree_d3_f32.max | 646 | 646 | +0 |
| eml_tree_d3_f32 | eml_tree_d3_f32.sum | 154897 | 154897 | +0 |
| eml_tree_d3_u8 | eml_tree_d3_u8.ok | 260 | 260 | +0 |
| eml_tree_d3_u8 | eml_tree_d3_u8.min | 195 | 195 | +0 |
| eml_tree_d3_u8 | eml_tree_d3_u8.max | 207 | 207 | +0 |
| eml_tree_d3_u8 | eml_tree_d3_u8.sum | 54168 | 54168 | +0 |
| eml_tree_d4_f32 | eml_tree_d4_f32.ok | 262 | 262 | +0 |
| eml_tree_d4_f32 | eml_tree_d4_f32.min | 646 | 646 | +0 |
| eml_tree_d4_f32 | eml_tree_d4_f32.max | 753 | 753 | +0 |
| eml_tree_d4_f32 | eml_tree_d4_f32.sum | 184202 | 184202 | +0 |
| eml_tree_d4_u8 | eml_tree_d4_u8.ok | 263 | 263 | +0 |
| eml_tree_d4_u8 | eml_tree_d4_u8.min | 199 | 199 | +0 |
| eml_tree_d4_u8 | eml_tree_d4_u8.max | 211 | 211 | +0 |
| eml_tree_d4_u8 | eml_tree_d4_u8.sum | 56136 | 56136 | +0 |
| eml_tree_d5_f32 | eml_tree_d5_f32.ok | 263 | 263 | +0 |
| eml_tree_d5_f32 | eml_tree_d5_f32.min | 664 | 664 | +0 |
| eml_tree_d5_f32 | eml_tree_d5_f32.max | 830 | 830 | +0 |
| eml_tree_d5_f32 | eml_tree_d5_f32.sum | 205434 | 205434 | +0 |
| eml_tree_d5_u8 | eml_tree_d5_u8.ok | 263 | 263 | +0 |
| eml_tree_d5_u8 | eml_tree_d5_u8.min | 201 | 201 | +0 |
| eml_tree_d5_u8 | eml_tree_d5_u8.max | 217 | 217 | +0 |
| eml_tree_d5_u8 | eml_tree_d5_u8.sum | 57320 | 57320 | +0 |
| eml_tree_d6_f32 | eml_tree_d6_f32.ok | 264 | 264 | +0 |
| eml_tree_d6_f32 | eml_tree_d6_f32.min | 662 | 662 | +0 |
| eml_tree_d6_f32 | eml_tree_d6_f32.max | 863 | 863 | +0 |
| eml_tree_d6_f32 | eml_tree_d6_f32.sum | 205757 | 205757 | +0 |
| eml_tree_d6_u8 | eml_tree_d6_u8.ok | 266 | 266 | +0 |
| eml_tree_d6_u8 | eml_tree_d6_u8.min | 201 | 201 | +0 |
| eml_tree_d6_u8 | eml_tree_d6_u8.max | 219 | 219 | +0 |
| eml_tree_d6_u8 | eml_tree_d6_u8.sum | 57216 | 57216 | +0 |
| eml_tree_d8_f32 | eml_tree_d8_f32.ok | 270 | 270 | +0 |
| eml_tree_d8_f32 | eml_tree_d8_f32.min | 698 | 698 | +0 |
| eml_tree_d8_f32 | eml_tree_d8_f32.max | 1035 | 1035 | +0 |
| eml_tree_d8_f32 | eml_tree_d8_f32.sum | 219792 | 219792 | +0 |
| eml_tree_d8_u8 | eml_tree_d8_u8.ok | 272 | 272 | +0 |
| eml_tree_d8_u8 | eml_tree_d8_u8.min | 201 | 201 | +0 |
| eml_tree_d8_u8 | eml_tree_d8_u8.max | 223 | 223 | +0 |
| eml_tree_d8_u8 | eml_tree_d8_u8.sum | 57299 | 57299 | +0 |
| eml_tree_d10_f32 | eml_tree_d10_f32.ok | 270 | 270 | +0 |
| eml_tree_d10_f32 | eml_tree_d10_f32.min | 698 | 698 | +0 |
| eml_tree_d10_f32 | eml_tree_d10_f32.max | 1035 | 1035 | +0 |
| eml_tree_d10_f32 | eml_tree_d10_f32.sum | 219792 | 219792 | +0 |
| eml_tree_d10_u8 | eml_tree_d10_u8.ok | 272 | 272 | +0 |
| eml_tree_d10_u8 | eml_tree_d10_u8.min | 201 | 201 | +0 |
| eml_tree_d10_u8 | eml_tree_d10_u8.max | 223 | 223 | +0 |
| eml_tree_d10_u8 | eml_tree_d10_u8.sum | 57299 | 57299 | +0 |

largest |silicon - simavr| over the cycle numbers: 0 cycles
