// Real device #2: a 16x2 character LCD over a PCF8574 I2C backpack, on
// the same Blue Pill focCompose already hardware-verified (SPI1, GPIOA)
// -- this uses I2C1 (PB6/PB7, GPIOB), a different port, no pin conflict.
//
// Zero new driver code needed in OneIO/OneBus themselves -- PCF8574,
// I2cGpio, Hd44780, and I2cLcd were all already platform-agnostic. One
// real gap WAS found and fixed in OneChip (chips/stm32/stm32Twi.h):
// oneBus::I2cGpio's own documented contract needs a single-byte
// `TwiMaster::send(addr, byte)`, but Stm32I2cCore only had the 3-arg
// buffer form -- added a one-line forwarding overload, the same shape as
// the pre-existing stm32Spi.h speed-clobber fix from focCompose's own
// hardware round.
#define STM32F1xx
#include <chips/stm32/stm32f103.h>
#include <oneIO/display/i2cLcd.h>

// ApbHz = 32 MHz: stm32f1xx-hal's rcc::Config::hsi().sysclk(72.MHz())
// clamps to 64 MHz sysclk (HSI/2 x16), so APB1 = HCLK/2 = 32 MHz, not the
// 36 MHz f1::Twi<> defaults to. Getting this right makes CCR/TRISE match
// the real bus clock (see OneIO/examples/i2cLcd's two-clock C++ version).
using Twi = hw::stm32::f1::Twi<100000, 32000000>; // I2C1, PB6(SCL)/PB7(SDA), 100 kHz
using Lcd = oneIO::display::I2cLcd<Twi>;          // default addr 0x27, 16x2, standard backpack wiring

extern "C" {
  void hapi_lcd_init() { Lcd::begin(); }
  void hapi_lcd_print(const char* line1, const char* line2) {
    Lcd::clear();
    Lcd::setCursor(0, 0); Lcd::print(line1);
    Lcd::setCursor(0, 1); Lcd::print(line2);
  }
}
