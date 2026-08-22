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
#![no_std]
#![no_main]

use panic_halt as _;
use cortex_m_rt::entry;
use stm32f1xx_hal::{pac, prelude::*, rcc};

extern "C" {
    fn hapi_ticker_inc();
    fn hapi_ticker_get() -> i32;
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
    let _rcc = dp.RCC.freeze(rcc::Config::hsi(), &mut flash.acr);

    let mut led = gpioc.pc13.into_push_pull_output(&mut gpioc.crh);
    if value == 6 {
        led.set_low(); // correct C++ result -> LED on, ODR bit13 = 0
    } else {
        led.set_high(); // wrong/unreached -> LED off, ODR bit13 = 1
    }

    loop {}
}
