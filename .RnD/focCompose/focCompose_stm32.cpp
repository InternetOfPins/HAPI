// focCompose Step 4 prep: same Branch A composition as focCompose.cpp,
// but with the real AS5047/AS5048/AS5147 SPI sensor (magnetic_sensor_spi_
// stm32.h) in place of the stub MagneticSensorSPI. BLDCDriver3PWM is
// still stubbed -- no real STM32 PWM/timer driver exists anywhere in
// OneChip yet (checked, not assumed -- see HANDOFF.md), and hand-rolling
// 3-phase center-aligned PWM register config from scratch with no
// hardware to validate against is a real motor/driver-board damage risk,
// not something to freehand blindly. Sensor-only for real, driver stays
// a stub until that's addressed deliberately.
#include "focAPI.h"
#include "stub_foc.h"
#include "magnetic_sensor_spi_stm32.h"
#include <chips/stm32/stm32Spi.h>

using namespace focCompose;
using namespace hapi;

using Spi = hw::stm32::f1::Spi<>; // real SPI1, PA5(SCK)/PA6(MISO)/PA7(MOSI), Blue Pill

using MotorTerminal = SensorAPI;
using Compose = FocCompose<MagneticSensorAS5x47<Spi>, BLDCDriver3PWM>::Part<MotorTerminal>;

static_assert(!__is_polymorphic(Compose), "Compose must not be polymorphic");

int main() {
  Compose::begin();                          // real: SPI1 init + PA4 CS config
  volatile float angle = Compose::getMechanicalAngle(); // REAL SPI transaction, untested against real hardware -- see file header
  Compose::enable();
  Compose::setPwm(1.0f, 2.0f, 3.0f);          // stub, no real PWM yet
  (void)angle;
  return 0;
}
