// focCompose: the resolved composition. Combines both real pieces built
// this round -- MagneticSensorAS5x47 (real SPI, hardware-verified CS
// pin) and BLDCDriver3PWM_STM32 (real TIM1 PWM+enable, hardware-verified
// registers) -- using the design the Branch A comparison actually
// settled on: named data members, not one flat Chain<Sensor,Driver>.
//
// Supersedes focCompose.cpp/focCompose_stm32.cpp's Chain<Sensor,Driver>
// shape, which had a real, confirmed flaw: Sensor's void init() and
// Driver's int init() collide in one linear chain, and the driver's
// silently shadows the sensor's (ordinary C++ name hiding, not an
// error) -- unreachable by name once folded together. See HANDOFF.md's
// "Branch A" comparison: tested data members, multiple inheritance
// (recovers EBO but turns the collision into a hard ambiguity error
// instead of silent hiding), and OneMenu's real Decor<>/Hidden<>
// explicit-redirect pattern. Named members won: `.sensor.init()`/
// `.driver.init()` are two different members, unambiguous by
// construction, and this is the one variant that actually matches real
// SimpleFOC's own `sensor->init()`/`driver->init()` two-pointer call
// shape -- not just a workaround, the more faithful design.
#include "focAPI.h"
#include "magnetic_sensor_spi_stm32.h"
#include "bldc_driver_3pwm_stm32.h"
#include <chips/stm32/stm32Spi.h>

using namespace focCompose;
using namespace hapi;

using Spi = hw::stm32::f1::Spi<>; // real SPI1, PA5(SCK)/PA6(MISO)/PA7(MOSI), Blue Pill

using SensorPart = Chain<MagneticSensorAS5x47<Spi>>::Part<SensorAPI>;
using DriverPart = Chain<BLDCDriver3PWM_STM32>::Part<DriverAPI>;

struct FocMotor {
  SensorPart sensor;
  DriverPart driver;
};

static FocMotor motor;

static_assert(!__is_polymorphic(FocMotor), "FocMotor must not be polymorphic");

int main() {
  motor.sensor.begin();   // real: SPI1 init + PA4 CS config (hardware-verified)
  motor.driver.init();    // real: TIM1 PWM init + PB0/PB1/PB10 enable pins (hardware-verified)
  motor.driver.enable();

  volatile float angle = motor.sensor.getMechanicalAngle(); // real SPI transaction -- untested against a real chip, see magnetic_sensor_spi_stm32.h
  motor.driver.setPwm(6.0f, 3.0f, 0.0f);                     // real TIM1 CCR writes -- no motor/driver board connected

  (void)angle;
  return 0;
}
