#pragma once
#include <Arduino.h>

// EPD control pins
static const int PIN_EPD_CS   = 21;
static const int PIN_EPD_DC   = 14;
static const int PIN_EPD_RST  = 13;
static const int PIN_EPD_BUSY = 38;
static const int PIN_EPD_PWR  = 16;  // 屏幕电源控制引脚

// SPI pins (match your wiring)
static const int PIN_SPI_SCK  = 18;
static const int PIN_SPI_MOSI = 17;
// MISO not used for most e-paper modules

