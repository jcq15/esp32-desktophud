#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>
#include <Adafruit_GFX.h>

// 引用 display_hal.cpp 里那个全局 display 对象
extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;

// 居中绘制文本
// font: 字体指针（如 &FreeMonoBold12pt7b）
// text: 要绘制的文本
// x, y, w, h: 文本居中的区域（矩形）
void drawCenteredText(const GFXfont* font, 
                      const String& text,
                      int x, int y, int w, int h);

