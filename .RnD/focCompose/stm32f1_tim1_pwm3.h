// Real 3-independent-channel PWM on STM32F103's TIM1 (advanced-control
// timer), register-level, no ST HAL -- matching OneChip's own no-HAL
// convention (stm32Spi.h/stm32Port.h). Targets the same "3PWM" shape
// SimpleFOC's own real BLDCDriver3PWM.cpp uses (confirmed by reading it,
// Arduino-FOC/src/drivers/BLDCDriver3PWM.cpp): one PWM signal + one
// digital enable pin per phase -- for driver boards with an onboard
// gate-driver IC that generates the complementary high/low switching and
// dead-time in silicon (DRV8302/DRV8305/DRV8316-class). NOT the 6-PWM
// complementary-output-with-MCU-side-dead-time shape (BLDCDriver6PWM.cpp,
// a different real SimpleFOC driver class) -- that needs TIM1's BDTR
// dead-time field tuned for real, needs a specific driver board's timing
// to get right, and isn't attempted this round.
//
// Pins: TIM1_CH1=PA8, CH2=PA9, CH3=PA10 (default AF mapping, no remap
// register touched). Enable pins are separate, plain GPIO -- see
// bldc_driver_3pwm_stm32.h.
//
// PWM: edge-aligned (CMS=00, simpler than SimpleFOC's own center-aligned
// default -- a real simplification, not hidden: center-aligned halves
// ripple-frequency-doubling benefits this driver doesn't need since
// there's no MCU-side dead-time to align against), 25kHz default --
// matches SimpleFOC's own real SIMPLEFOC_STM32_PWM_FREQUENCY constant
// (stm32_mcu.h), not invented.
//
// Real TIM1-specific gotcha, easy to miss and worth flagging explicitly:
// BDTR.MOE (Main Output Enable) must be set or NO channel outputs
// anything, even with CCER/CCMR fully configured correctly -- the
// advanced-control timer's extra safety interlock, absent on
// general-purpose timers (TIM3/TIM4). Verified this is actually being
// hit correctly via real register readback -- see HANDOFF.md.
#pragma once
#include <hapi/hapi.h>
#include <stdint.h>

namespace focCompose {

  struct tim1_regs {
    volatile uint32_t cr1, cr2, smcr, dier, sr, egr, ccmr1, ccmr2,
                       ccer, cnt, psc, arr, rcr, ccr1, ccr2, ccr3, ccr4,
                       bdtr, dcr, dmar;
  };

  template<uint32_t ApbHz = 72000000UL, uint32_t FreqHz = 25000UL>
  struct Stm32F1Tim1Pwm3 {
    static constexpr uint32_t TIM1_BASE = 0x40012C00u;
    static constexpr uint32_t RCC_APB2ENR = 0x40021018u;
    static constexpr uint16_t Arr = uint16_t(ApbHz / FreqHz - 1);

    static tim1_regs& regs() { return *reinterpret_cast<tim1_regs*>(TIM1_BASE); }

    template<typename O>
    struct Part : O {
      static void begin() {
        *reinterpret_cast<volatile uint32_t*>(RCC_APB2ENR) |= (1u<<2) | (1u<<11); // IOPAEN | TIM1EN
        volatile uint32_t& crh = *reinterpret_cast<volatile uint32_t*>(0x40010804u); // GPIOA CRH
        crh = (crh & ~0xFFFu) | 0xBBBu; // PA8/PA9/PA10 = AF push-pull 50MHz

        regs().psc  = 0;
        regs().arr  = Arr;
        regs().ccr1 = regs().ccr2 = regs().ccr3 = 0;

        // CH1/CH2: PWM mode 1 (110), preload enable
        regs().ccmr1 = (6u<<4) | (1u<<3) | (6u<<12) | (1u<<11);
        // CH3: PWM mode 1, preload enable
        regs().ccmr2 = (6u<<4) | (1u<<3);
        // CC1E, CC2E, CC3E -- enable all three compare outputs
        regs().ccer  = (1u<<0) | (1u<<4) | (1u<<8);

        regs().cr1  = (1u<<7);   // ARPE (auto-reload preload enable)
        regs().bdtr = (1u<<15);  // MOE -- see file header, required on TIM1
        regs().egr  = 1u;        // UG -- force shadow-register load before first period
        regs().cr1 |= 1u;        // CEN -- start counting

        O::begin();
      }

      static void ccr1(uint16_t d) { regs().ccr1 = d; }
      static void ccr2(uint16_t d) { regs().ccr2 = d; }
      static void ccr3(uint16_t d) { regs().ccr3 = d; }
      static constexpr uint16_t maxDuty() { return Arr; }
    };
  };

}
