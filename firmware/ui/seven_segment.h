#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

// 七段数码管绘制函数
// 绘制单个数字
// display: 显示对象
// x, y: 左上角坐标
// width, height: 数字的宽度和高度
// digit: 要显示的数字 (0-9)
// lineWidth: 段的线宽
void drawSevenSegmentDigit(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                           int x, int y, int width, int height, int digit, int lineWidth = 8);

// 绘制冒号（用于时间分隔符）
void drawSevenSegmentColon(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                          int x, int y, int size);

// 绘制时间 HH:MM
void drawSevenSegmentTime(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                         int x, int y, int width, int height, const String& timeStr);

