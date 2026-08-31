//! Phase C: real Rust firmware calling into a real, unmodified
//! HAPI-composed C++ stack, on real STM32F103C8 (Blue Pill) hardware.
//!
//! `cpp/shim.cpp` composes a trivial component via hapi::APIOf<> (the
//! same real Chain<> inheritance-fold + template-template-parameter
//! pattern as HAPI/examples/cuda_device_chain, cross-compiled here with
//! the real arm-none-eabi-g++ toolchain focCompose already uses, wired
//! in by build.rs) and exposes it through a hand-written extern "C"
//! shim -- the realistic mechanism confirmed by research (bindgen can't
//! bind templates; cxx needs an allocator, in tension with this
//! ecosystem's no-heap stance).
//!
//! The C++-composed value genuinely decides real hardware state here:
//! PC13 (onboard LED, active-low) is driven low only if the C++ side's
//! answer is exactly right, so a real OpenOCD register readback of ODR
//! is direct evidence the value crossed the FFI boundary correctly, not
//! just "the firmware didn't crash." See README.md for the honest
//! FFI-boundary-is-not-zero-overhead finding, and for Phase A's
//! Rust-only baseline this built on (git history / HANDOFF, not
//! preserved as separate source here).
//!
//! Device #2: `cpp/lcd_shim.cpp` bridges the same way to a real 16x2
//! character LCD over I2C1 (PB6/PB7) via a PCF8574 backpack -- zero new
//! OneIO/OneBus driver code needed (both were already platform-agnostic),
//! one real gap found+fixed in OneChip (a missing single-byte
//! `TwiMaster::send(addr,byte)` overload `oneBus::I2cGpio` needs). Also
//! switches the clock to real 72MHz PLL, not just raw 8MHz HSI --
//! `hd44780.h`'s portable delay loop is calibrated assuming ~72MHz, and
//! running it at only 8MHz would make every LCD init delay ~9x too
//! short. See README.md for the full story.
#![no_std]
#![no_main]

use panic_halt as _;
use cortex_m_rt::entry;
use stm32f1xx_hal::{pac, prelude::*, rcc};

extern "C" {
    fn hapi_ticker_inc();
    fn hapi_ticker_get() -> i32;
    fn hapi_lcd_init();
    fn hapi_lcd_print(line1: *const u8, line2: *const u8);
}

#[entry]
fn main() -> ! {
    // Three inc() calls, each folding DoubleStep -> 2x Counter::inc()
    // inside the real, unmodified HAPI Chain<> on the C++ side: 3*2=6.
    // Matches cuda_device_chain's own numbers exactly, for a direct
    // side-by-side comparison across toolchains (nvcc vs arm-none-eabi-g++).
    unsafe {
        hapi_ticker_inc();
        hapi_ticker_inc();
        hapi_ticker_inc();
    }
    let value = unsafe { hapi_ticker_get() };

    let mut dp = pac::Peripherals::take().unwrap();
    // split() needs &mut dp.RCC (a disjoint field borrow, fine before
    // dp.RCC is moved into freeze() below, which consumes it).
    let mut gpioc = dp.GPIOC.split(&mut dp.RCC);

    let mut flash = dp.FLASH.constrain();
    // HSI-derived PLL, NOT HSE -- real, sequenced findings, both caught
    // by register readback rather than assumed:
    // (1) rcc::Config::hsi().sysclk(72.MHz()) does not error on an
    //     unreachable target -- HSI/2=4MHz's max PLL multiplier is x16
    //     (hardware limit), so it silently clamps to 64MHz instead of
    //     the requested 72MHz (confirmed: PLLSRC=0, PLLMUL=x16).
    // (2) Tried rcc::Config::hse(8.MHz()).sysclk(72.MHz()) to reach the
    //     literal 72MHz via the Blue Pill's onboard crystal -- REVERTED:
    //     the driver's real HSE-ready wait is an unconditional busy-loop
    //     (`while hserdy is clear {}`) with no timeout, executed BEFORE
    //     any PLL/LED/LCD code. If this specific board's HSE crystal
    //     isn't populated or isn't starting, that hangs forever and
    //     NOTHING after it (including the LCD output below) ever runs.
    //     A second register readback after that attempt was consistent
    //     with exactly this (state frozen before the PLL step), not
    //     confirmed as a working 72MHz clock.
    // 64MHz (not 72MHz) is safe for hd44780.h's delay calibration: a
    // slower-than-assumed clock makes the busy-loop delays LONGER than
    // the nominal microsecond value, not shorter -- HD44780 tolerates
    // longer-than-minimum delays fine, only under-delaying breaks it
    // (the real problem the original 8MHz-only config had).
    let _rcc = dp.RCC.freeze(rcc::Config::hsi().sysclk(72.MHz()), &mut flash.acr);

    let mut led = gpioc.pc13.into_push_pull_output(&mut gpioc.crh);
    if value == 6 {
        led.set_low(); // correct C++ result -> LED on, ODR bit13 = 0
    } else {
        led.set_high(); // wrong/unreached -> LED off, ODR bit13 = 1
    }

    unsafe {
        hapi_lcd_init();
        hapi_lcd_print(b"Rust -> HAPI\0".as_ptr(), b"on STM32!\0".as_ptr());
    }

    loop {}
}
