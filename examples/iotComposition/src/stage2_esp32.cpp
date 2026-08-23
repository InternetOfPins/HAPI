// Stage 2: ESP32 IotDevice -- same application-facing composition as
// Stage 1 (AVR), same components (Sensor/Actuator/Storage completely
// unchanged), Network swapped from Serial to the new Mqtt<>. Builds
// against the real framework-arduinoespressif32 core + real
// knolleary/PubSubClient (PlatformIO espressif32/esp32dev), not a stub
// SDK -- real flash/RAM numbers and an app_main()/Arduino-main.cpp
// conflict this build caught that a stub build couldn't.
#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneHLS/oneHLS.h>
#include <chips/esp32/esp32Device.h>
#include <oneIO/sensor/aht.h>
#include <oneIO/eeprom/at24c.h>
#include <oneIO/pwm/pca9685.h>
#include <oneIO/net/mqtt.h>

using namespace hapi;

using I2c    = hw::esp32::Esp32TwiMaster<>;   // real OneChip ESP32 I2C, Arduino path

static const char kSsid[]   = "iop-demo";
static const char kPass[]   = "iop-demo-pass";
static const char kBroker[] = "192.168.1.1";
using Net = oneIO::net::Mqtt<kSsid, kPass, kBroker>;

using Sensor = oneIO::sensor::AHT<I2c>;
using Pwm    = oneIO::pwm::PCA9685<I2c>;
using Store  = oneIO::eeprom::AT24C<I2c>;     // called directly, same as Stage 1

struct DeviceTerminal { static void begin() {} };
using IotDevice = Chain<Sensor, Pwm, Net>::Part<DeviceTerminal>;

static oneHLS::Pid<int16_t, int32_t, 256, 32, 16> heaterPid;

void setup() {
  IotDevice::begin();
  IotDevice::measure();

  int16_t setpointX10 = 235;
  int16_t errorX10 = int16_t(setpointX10 - IotDevice::tempC10());
  int32_t u = heaterPid.step(errorX10);
  uint16_t duty = uint16_t(u < 0 ? 0 : (u > 4095 ? 4095 : u));
  IotDevice::set(0, duty);

  uint8_t buf[2] = { uint8_t(setpointX10 >> 8), uint8_t(setpointX10) };
  Store::write(0, buf, 2);

  IotDevice::publish("iop/temp", "report");
}

void loop() {}
