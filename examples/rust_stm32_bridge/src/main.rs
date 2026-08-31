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
//! character LCD over I2C1 (PB6/PB7) via a PCF8574 backpack, using the
//! unmodified oneIO::display::I2cLcd -> Hd44780 -> oneBus::I2cGpio ->
//! hw::stm32::Stm32I2cCore stack -- composition over a real peripheral,
//! not a toy counter. Needs a PLL clock (hd44780.h's delay loop assumes
//! ~72MHz; the 8MHz HSI reset default would under-delay LCD init ~9x).
//! See README.md.
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
    // sysclk(72.MHz()) on HSI silently clamps to 64 MHz -- HSI/2 = 4 MHz
    // caps the PLL at x16 (PLLSRC=0, PLLMUL=x16, register-confirmed). The
    // HSE path for a literal 72 MHz is avoided: the HAL's HSE-ready wait
    // is an un-timeout'd busy-loop, so a dead crystal hangs the firmware.
    // 64 MHz is fine for hd44780.h -- a slower clock only over-delays.
    // lcd_shim.cpp pins the I2C core to the matching 32 MHz APB1.
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
