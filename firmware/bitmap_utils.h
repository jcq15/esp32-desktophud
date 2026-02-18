#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

// Base64解码并绘制位图到指定区域
// base64Data: base64编码的位图数据字符串
// display: 显示对象引用
// x, y: 绘制位置
// w, h: 位图尺寸（从region获取，不需要从JSON读取）
// 返回: 是否成功
bool drawBitmapFromBase64(const String& base64Data, 
                          GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                          int x, int y, int w, int h);

