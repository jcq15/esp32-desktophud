#include "display_hal.h"
#include "pins.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

// 这个是“真·display”，UI 层会 extern 引用它
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(
    GxEPD2_750_T7(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)
);

void display_begin() {
    SPI.begin(PIN_SPI_SCK, -1, PIN_SPI_MOSI, PIN_EPD_CS);
    display.init(115200);
}

void display_full_refresh(void (*drawFn)()) {
    display.setFullWindow();
    display.firstPage();
    do { drawFn(); } while (display.nextPage());
}
