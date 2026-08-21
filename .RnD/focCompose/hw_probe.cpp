// Real-hardware probe build: identical composition to focCompose_stm32.cpp,
// wrapped so main() parks in an infinite loop after the real begin()/
// getMechanicalAngle() call -- lets OpenOCD halt and read back live
// peripheral register state (SPI1 CR1/SR, GPIOA CRL/ODR) to confirm the
// init code actually did what it's supposed to on real silicon, with no
// sensor chip required to be attached yet. Not part of the canonical
// focCompose_stm32.cpp -- this file exists only for this one-off hardware
// check, see HANDOFF.md.
#include "focAPI.h"
#include "stub_foc.h"
#include "magnetic_sensor_spi_stm32.h"
#include <chips/stm32/stm32Spi.h>

using namespace focCompose;
using namespace hapi;

using Spi = hw::stm32::f1::Spi<>;

using MotorTerminal = SensorAPI;
using Compose = FocCompose<MagneticSensorAS5x47<Spi>, BLDCDriver3PWM>::Part<MotorTerminal>;

volatile float    g_lastAngle = 0;
volatile uint32_t g_loops = 0;

int main() {
  Compose::begin();
  for (int i = 0; i < 5; i++) {
    g_lastAngle = Compose::getMechanicalAngle();
    g_loops++;
  }
  CsPin::off(); // explicit, stable idle-high -- park here so a halt is guaranteed to catch it
  while (1) {}
}
