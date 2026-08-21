// focCompose Step 4 prep: real AS5047/AS5048/AS5147-class SPI magnetic
// angle sensor on real STM32F103 (Blue Pill) hardware, reusing OneChip's
// existing hw::stm32::f1::Spi<> driver unmodified (same one already
// verified real in this project's own STM32 addendum build).
//
// Protocol ported from Arduino-FOC's own real upstream source
// (Arduino-FOC/src/sensors/MagneticSensorSPI.cpp, AS5147_SPI/AS5048_SPI/
// AS5047_SPI configs -- all three chips share one config): 16-bit SPI
// command/response (sent here as two 8-bit transfers back-to-back within
// one CS-low window -- Stm32SpiCore's transfer() is byte-wise, DFF=1
// 16-bit mode isn't wired up; electrically identical on the wire), angle
// register 0x3FFF, read bit at position 14, even-parity bit at 15,
// 14-bit data.
//
// CS pin: PA4, composed from OneBit/OnePin primitives, not hand-rolled
// register pokes -- an earlier version of this file wrote raw pointers
// directly and got the BSRR offset wrong (0x0C, which is actually ODR),
// a real bug confirmed via real hardware register readback: cs_low()
// drove the pin HIGH instead of LOW, and clobbered every other GPIOA
// output pin each call since it was a whole-register write, not an
// atomic set/reset. Composing oneBit::Inverted<> over oneBit::Mask<> --
// active-low CS as inverted logic over an ordinary active-high pin --
// reuses OneBit's own tested get/set/on/off machinery (Mask<>::on()
// already auto-detects and uses bsrr_set/bsrr_clr atomically) instead of
// re-deriving BSRR semantics by hand a second time.
//
// Neither oneBus::ChipSelect<> (AVR-only: a PORTx/DDRx-adjacent-register
// trick, no STM32 equivalent) nor stm32Port.h's STM32Port<> (MODER/
// OSPEEDR/PUPDR/AFR scheme -- real for F0/F4/L4/H7, NOT F1's legacy
// CRL/CRH scheme; no f1:: namespace exists there at all, despite that
// file's header comment claiming universal layout) cover this chip
// family -- stm32f1_port_core.h's Stm32F1PortCore<> is a minimal,
// local, F1-specific substitute exposing just the bsrr_set/bsrr_clr/
// dir_out surface Mask<>/Inverted<> expect, same method names
// STM32Port<> already uses so the composition is a drop-in once F1
// support is added there for real.
//
// UNTESTED AGAINST REAL HARDWARE (sensor side): no sensor chip, no
// scope/logic analyzer available in this sandbox. The CS pin composition
// itself HAS been verified against real silicon (STM32F103 over
// ST-Link/OpenOCD, register readback confirmed correct polarity/no
// collateral damage to other GPIOA pins after the Inverted<>/Mask<>
// rewrite -- see HANDOFF.md). What's still unverified: the command/
// response gap timing (_gap(), a placeholder busy-wait) and whether an
// actual AS5x47-class chip responds as expected to this exact sequence.
//
// Deliberately NOT using oneChip/clock.h's hw::delay_ms() here -- real,
// confirmed gap found while building this: clock.h's platform dispatch
// only branches on __AVR__ and ARDUINO; a bare framework=cmsis STM32
// build (this one) falls through to the "else" branch, which assumes a
// hosted/native environment (std::chrono/std::this_thread) -- doesn't
// exist in freestanding ARM, fails to compile. clock.h's own doc comment
// already carves out an AVR "IOP mode" (#ifndef IOP, user supplies
// millis() via their board's SysTick) but never extends that carve-out
// to non-AVR bare-metal targets in the else branch. Not fixed here --
// that needs the project's actual STM32 SysTick/IOP-mode convention,
// which this sandbox doesn't have visibility into; flagged in HANDOFF.md
// instead of guessed at.
#pragma once
#include <hapi/hapi.h>
#include <onePin/onePin.h>
#include <oneBit/oneBit.h>
#include <chips/stm32/stm32Spi.h>
#include "stm32f1_port_core.h"
#include <stdint.h>

namespace focCompose {

  using CsPin = hapi::APIOf<onePin::Stm32OutPin, oneBit::Inverted<>,
                             oneBit::Mask<oneBit::Pins<4>>,
                             Stm32F1PortCore<0x40010800u, /*IOPAEN*/2>>;

  template<typename Spi>
  struct MagneticSensorAS5x47 {
    template<typename O>
    struct Part : O {
      static void begin() {
        CsPin::dir_out(); // configure PA4 as GP push-pull output (Mask<> forwards its own compile-time mask)
        CsPin::off();                       // idle: logically deselected -> physically HIGH
        Spi::begin();
        O::begin();
      }

      static uint8_t _evenParity(uint16_t v) {
        uint8_t p = 0;
        for (int i = 0; i < 16; i++) { if (v & 1u) p++; v >>= 1; }
        return p & 1u;
      }

      // Placeholder busy-wait -- see file header. ~72 cycles at 72MHz
      // core clock (~1us), generously over the ~350ns AMS datasheets
      // ask for; not calibrated against a real scope.
      static void _gap() { for (volatile int i = 0; i < 72; i++) {} }

      // Read a 14-bit register (default: angle register 0x3FFF).
      static uint16_t readRegister(uint16_t reg = 0x3FFF) {
        uint16_t command = reg | (1u << 14);                 // RW=1 (read)
        command |= uint16_t(_evenParity(command)) << 15;      // even parity bit

        CsPin::on();  // logically selected -> physically LOW (Inverted<>)
        Spi::transfer(uint8_t(command >> 8));
        Spi::transfer(uint8_t(command & 0xFF));
        CsPin::off(); // physically HIGH

        _gap(); // TODO verify against real hardware -- see file header

        CsPin::on();
        uint16_t hi = Spi::transfer(0x00);
        uint16_t lo = Spi::transfer(0x00);
        CsPin::off();

        return uint16_t((hi << 8) | lo) & 0x3FFF; // 14 data bits
      }

      static float getMechanicalAngle() {
        return (float(readRegister()) / 16384.0f) * 6.2831853f; // cpr=2^14, x 2*pi
      }
    };
  };

}
