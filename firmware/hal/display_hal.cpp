#include "display_hal.h"
#include "pins.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

// 这个是"真·display"，UI 层会 extern 引用它
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(
    GxEPD2_750_T7(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)
);

void display_begin() {
    // 初始化PWR引脚并设置为高电平
    pinMode(PIN_EPD_PWR, OUTPUT);
    digitalWrite(PIN_EPD_PWR, HIGH);
    
    SPI.begin(PIN_SPI_SCK, -1, PIN_SPI_MOSI, PIN_EPD_CS);
    display.init(115200);
    display.setRotation(0);  // 横向显示（800x480）
}

void display_full_refresh(void (*drawFn)()) {
    display.setFullWindow();
    display.firstPage();
    do { drawFn(); } while (display.nextPage());
}

void display_partial_refresh(void (*drawFn)()) {
    // 局部刷新整个窗口，避免闪烁
    // 注意：GxEPD2 库的 nextPage() 会自动完成刷新，不需要额外调用 displayPartial()
    display.setPartialWindow(0, 0, display.width(), display.height());
    display.firstPage();
    do { drawFn(); } while (display.nextPage());
}

void display_partial_refresh_area(void (*drawFn)(), int x, int y, int w, int h) {
    // 局部刷新指定区域
    display.setPartialWindow(x, y, w, h);
    display.firstPage();
    do { drawFn(); } while (display.nextPage());
}

// 绘制边框的辅助函数（在绘制回调中使用）
void display_draw_borders_in_callback() {
    // 绘制所有区域的边框
    // SENTENCE: (0, 0, 800, 56)
    display.drawRect(0, 0, 800, 56, GxEPD_BLACK);
    
    // TIME: (0, 176, 496, 176)
    display.drawRect(0, 176, 496, 176, GxEPD_BLACK);
    
    // CALENDAR: (0, 56, 496, 120)
    display.drawRect(0, 56, 496, 120, GxEPD_BLACK);
    
    // WEATHER: (496, 56, 304, 176)
    display.drawRect(496, 56, 304, 176, GxEPD_BLACK);
    
    // NOTE: (0, 352, 496, 128)
    display.drawRect(0, 352, 496, 128, GxEPD_BLACK);
    
    // STATUS: (496, 424, 304, 56)
    display.drawRect(496, 424, 304, 56, GxEPD_BLACK);
    
    // FREE: (496, 232, 304, 192)
    display.drawRect(496, 232, 304, 192, GxEPD_BLACK);
}

void display_draw_region_borders() {
    // 绘制所有区域的边框
    // 使用全屏刷新来绘制边框（只在setup时调用一次）
    display.setFullWindow();
    display.firstPage();
    do {
        display_draw_borders_in_callback();
    } while (display.nextPage());
}

