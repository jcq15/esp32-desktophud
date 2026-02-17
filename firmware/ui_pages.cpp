#include "ui_pages.h"
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

// 引用 display_hal.cpp 里那个全局 display 对象
extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;

void renderHome(const AppState& state) {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setRotation(1);

    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(20, 40);
    display.print(state.title);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, 80);
    display.print(state.line1);

    display.setCursor(20, 110);
    display.print(state.line2);
}
