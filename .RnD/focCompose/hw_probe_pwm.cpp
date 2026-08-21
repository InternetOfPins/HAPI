// Real-hardware probe: BLDCDriver3PWM_STM32 only (no sensor, no motor
// connected -- TIM1 PWM + GPIOB enable pins verified via OpenOCD register
// readback, same technique that already caught two real bugs on the
// sensor side). Parks after a known setPwm() call so a halt deterministically
// catches a known-good register state. See HANDOFF.md.
#include "focAPI.h"
#include "bldc_driver_3pwm_stm32.h"

using namespace focCompose;
using namespace hapi;

using Compose = FocCompose<BLDCDriver3PWM_STM32>::Part<DriverAPI>;

int main() {
  Compose::init();
  Compose::enable();
  Compose::setPwm(6.0f, 3.0f, 0.0f); // 50%, 25%, 0% duty at 12V supply -- known values to verify against CCR1/2/3
  while (1) {}
}
