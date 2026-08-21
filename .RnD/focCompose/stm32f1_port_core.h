// Minimal F1 GPIO port core -- just the bsrr_set/bsrr_clr/dir_out/
// clockEnable surface OneBit's Mask<>/Inverted<> composition expects
// (method names match STM32Port<>'s own, chips/stm32/stm32Port.h), for
// the one pin this round actually needs (PA4, magnetic sensor CS).
//
// NOT a full STM32Port<>-equivalent for F1 -- that's real, general
// library work: F1's GPIO block is the legacy CRL/CRH scheme, genuinely
// different from the MODER/OSPEEDR/PUPDR/AFR scheme STM32Port<> already
// covers for F0/F4/L4/H7 (see HANDOFF.md's earlier finding: no f1::
// namespace exists in stm32Port.h despite its header comment claiming
// "identical layout across all STM32 families"). This core covers pins
// 0-15 (CRL for 0-7, CRH for 8-15), push-pull 50MHz output mode only --
// exactly what CS/enable pins need, not a general-purpose replacement.
//
// Real bug, found via real hardware register readback (see HANDOFF.md):
// an earlier version only handled pins 0-7 (CRL) and silently did
// nothing for pin 10 when composed for an enable pin on that pin number
// -- BSRR/ODR writes landed on a pin still in its CRH reset-default
// input mode, so they never actually drove anything. CRL/CRH split is
// now handled explicitly instead of assumed away.
#pragma once
#include <hapi/hapi.h>
#include <stdint.h>

namespace focCompose {

  template<uint32_t GPIO_BASE, uint8_t RccApb2EnBit>
  struct Stm32F1PortCore {
    static constexpr uint32_t CRL  = GPIO_BASE + 0x00u;
    static constexpr uint32_t CRH  = GPIO_BASE + 0x04u;
    static constexpr uint32_t BSRR = GPIO_BASE + 0x10u;
    static constexpr uint32_t RCC_APB2ENR = 0x40021018u;

    template<typename O>
    struct Part : O {
      using Type = unsigned int;

      static void clockEnable() {
        *reinterpret_cast<volatile uint32_t*>(RCC_APB2ENR) |= (1u << RccApb2EnBit);
      }

      // Atomic set/reset -- matches STM32Port<>'s own bsrr_set/bsrr_clr
      // naming exactly, so OneBit's Mask<>::Part::on()/off() (which
      // detects these via has_bsrr_set_fn/has_bsrr_clr_fn) picks them up
      // automatically.
      static void bsrr_set(Type mask) { *reinterpret_cast<volatile uint32_t*>(BSRR) = mask;        }
      static void bsrr_clr(Type mask) { *reinterpret_cast<volatile uint32_t*>(BSRR) = mask << 16;  }

      // Configure every pin in mask (0-15) as general-purpose push-pull
      // output, 50MHz (CNF=00, MODE=11 -> field 0x3). Pins 0-7 live in
      // CRL, pins 8-15 in CRH -- same 4-bit-per-pin field shape, two
      // separate registers, handled explicitly (not folded into one
      // loop over a fabricated 64-bit view, to keep this readable and
      // matching stm32Spi.h's own plain-register-write style).
      static void dir_out(Type mask) {
        clockEnable();
        volatile uint32_t& crl = *reinterpret_cast<volatile uint32_t*>(CRL);
        volatile uint32_t& crh = *reinterpret_cast<volatile uint32_t*>(CRH);
        uint32_t vl = crl, vh = crh;
        for (int p = 0; p < 8; p++)
          if (mask & (1u << p)) vl = (vl & ~(0xFu << (p*4))) | (0x3u << (p*4));
        for (int p = 8; p < 16; p++)
          if (mask & (1u << p)) vh = (vh & ~(0xFu << ((p-8)*4))) | (0x3u << ((p-8)*4));
        crl = vl;
        crh = vh;
      }

      static void begin() { O::begin(); }
    };
  };

}
