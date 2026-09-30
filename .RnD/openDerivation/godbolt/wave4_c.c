// Banknote wave4 (static_net), written by hand in plain C: no templates, no HAPI, nothing but <stdint.h> and <stdbool.h>.
// The same trained cell as wave4_apiof.cpp and wave4_od.cpp (models/banknote/wave_params.h, fold 1), spelled out:
// each input is shifted right, offset by a phase and masked; the four phases and the bias add up modulo 256, and the
// class is bit 7 of the sum (sum < 128). It compiles as C or as C++; for the comparison, compile it as the other two
// are, as C++ with -std=c++17 -Os -mmcu=atmega328p (see README.md).
#include <stdint.h>
#include <stdbool.h>

bool wave4(const uint8_t* x) {
  uint8_t s = 175;                      // WAVE_K
  s += ((x[0] >> 2) + 110) & 0xba;      // WAVE_F0: shift -2, phase 110, mask 0xba
  s += ((x[1] >> 2) +  80) & 0xbf;      // WAVE_F1: shift -2, phase  80, mask 0xbf
  s += (x[2] >> 2) & 0x3f;              // WAVE_F2: shift -2, phase   0, mask 0x3f
  s += (x[3] >> 7) & 0x01;              // WAVE_F3: shift -7, phase   0, mask 0x01
  return s < 128;
}
