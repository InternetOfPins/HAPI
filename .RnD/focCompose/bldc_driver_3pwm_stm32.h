// Real BLDCDriver3PWM on STM32F103: same shape as SimpleFOC's real
// BLDCDriver3PWM.cpp (Arduino-FOC/src/drivers/BLDCDriver3PWM.cpp, read
// before writing this, not assumed) -- voltage->duty conversion is
// Ua/voltage_power_supply clamped to [0,1], enable pins driven
// active-high (matches FOCDriver's real enable_active_high default),
// setPhaseState() toggles each phase's enable pin independently.
//
// PWM: Stm32F1Tim1Pwm3 (TIM1, PA8/PA9/PA10). Enable pins: PB0/PB1/PB10,
// plain GPIO, composed via the same oneBit::Mask<>/Stm32F1PortCore
// pattern the CS pin already used and hardware-verified -- reused here
// rather than hand-rolling BSRR pokes a third time. NOT Inverted<> --
// enable pins are active-high, unlike CS.
//
// Board-specific: PB0/PB1/PB10 are a placeholder choice (free pins on a
// bare Blue Pill, no known driver board to match yet -- see HANDOFF.md).
// Change per whatever real driver board's enable-pin wiring turns out to
// be once one is on the bench.
#pragma once
#include <hapi/hapi.h>
#include <onePin/onePin.h>
#include <oneBit/oneBit.h>
#include "stm32f1_tim1_pwm3.h"
#include "stm32f1_port_core.h"
#include "focAPI.h"

namespace focCompose {

  struct PwmTerminal { static void begin() {} };
  using Pwm = hapi::Chain<Stm32F1Tim1Pwm3<>>::Part<PwmTerminal>;

  using EnA = hapi::APIOf<onePin::Stm32OutPin, oneBit::Mask<oneBit::Pins<0>>,  Stm32F1PortCore<0x40010C00u, /*IOPBEN*/3>>;
  using EnB = hapi::APIOf<onePin::Stm32OutPin, oneBit::Mask<oneBit::Pins<1>>,  Stm32F1PortCore<0x40010C00u, /*IOPBEN*/3>>;
  using EnC = hapi::APIOf<onePin::Stm32OutPin, oneBit::Mask<oneBit::Pins<10>>, Stm32F1PortCore<0x40010C00u, /*IOPBEN*/3>>;

  struct BLDCDriver3PWM_STM32 {
    template<typename O>
    struct Part : O {
      // TODO: match real supply voltage once a driver board is on hand --
      // SimpleFOC's own DEF_POWER_SUPPLY is 12V, matched here as a
      // starting default, not measured against anything real.
      static constexpr float voltage_power_supply = 12.0f;

      static int init() {
        Pwm::begin();
        EnA::dir_out(); EnB::dir_out(); EnC::dir_out();
        EnA::off(); EnB::off(); EnC::off(); // start disabled -- safe default
        return 1;
      }

      static void enable() {
        EnA::on(); EnB::on(); EnC::on();
        setPwm(0.0f, 0.0f, 0.0f);
      }

      static void disable() {
        setPwm(0.0f, 0.0f, 0.0f);
        EnA::off(); EnB::off(); EnC::off();
      }

      static float _clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

      static void setPwm(float Ua, float Ub, float Uc) {
        float dcA = _clamp01(Ua / voltage_power_supply);
        float dcB = _clamp01(Ub / voltage_power_supply);
        float dcC = _clamp01(Uc / voltage_power_supply);
        Pwm::ccr1(uint16_t(dcA * Pwm::maxDuty()));
        Pwm::ccr2(uint16_t(dcB * Pwm::maxDuty()));
        Pwm::ccr3(uint16_t(dcC * Pwm::maxDuty()));
      }

      static void setPhaseState(PhaseState sa, PhaseState sb, PhaseState sc) {
        sa == PhaseState::PHASE_ON ? EnA::on() : EnA::off();
        sb == PhaseState::PHASE_ON ? EnB::on() : EnB::off();
        sc == PhaseState::PHASE_ON ? EnC::on() : EnC::off();
      }

      static DriverType type() { return DriverType::BLDC; }
    };
  };

}
