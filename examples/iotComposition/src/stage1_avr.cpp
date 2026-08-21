// Stage 1 smoke test: AVR (atmega328p) IotDevice composition using ONLY
// existing, real InternetOfPins headers (HAPI + OneChip + OneBus + OneIO +
// OneData + OneHLS), no new component code written.
//
// Sensor(AHT)+Actuator(PCA9685)+Network(Serial) spliced into one Chain<>;
// Storage(AT24C) used directly (it's a closed static utility, not a
// Chain-splice-able Part -- see HANDOFF for why); Control(oneHLS::Pid) held
// as a plain member, exactly like the hls_core_components example.

#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneHLS/oneHLS.h>
#include <chips/avr/avrDevice.h>
#include <oneIO/sensor/aht.h>
#include <oneIO/eeprom/at24c.h>
#include <oneIO/pwm/pca9685.h>

using namespace hapi;

using I2c    = hw::avr::mega::Twi<>;          // shared I2C bus
using Serial = hw::avr::mega::Serial0<9600>;  // "Network" leg for the AVR config

using Sensor = oneIO::sensor::AHT<I2c>;
using Pwm    = oneIO::pwm::PCA9685<I2c>;
using Store  = oneIO::eeprom::AT24C<I2c>;     // used directly, not chain-spliced

struct DeviceTerminal { static void begin() {} };

using IotDevice = Chain<Sensor, Pwm, Serial>::Part<DeviceTerminal>;

// Control: plain member, same idiom as hls_core_components/src/main.cpp.
// Sample/Accum are plain ints here (MCU firmware leg), vs ac_fixed/ap_fixed
// on the HLS leg -- same header, same Pid<>, different Sample/Accum choice.
static oneHLS::Pid<int16_t, int32_t, 256, 32, 16> heaterPid;

int main() {
  IotDevice::begin();
  IotDevice::measure();                 // AHT: trigger + block + read

  int16_t setpointX10 = 235;            // 23.5C -- would be restored from Store
  int16_t errorX10     = int16_t(setpointX10 - IotDevice::tempC10());
  int32_t u            = heaterPid.step(errorX10);
  uint16_t duty         = uint16_t(u < 0 ? 0 : (u > 4095 ? 4095 : u));

  IotDevice::set(0, duty);              // drive heater PWM channel 0

  uint8_t buf[2] = { uint8_t(setpointX10 >> 8), uint8_t(setpointX10) };
  Store::write(0, buf, 2);              // persist setpoint

  IotDevice::println("report");         // Network leg placeholder

  return 0;
}

// Regression pin -- fails to compile if composition RAM footprint regresses.
static_assert(sizeof(IotDevice) <= 4, "IotDevice composition grew unexpectedly");
