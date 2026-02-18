#pragma once
#include <Arduino.h>
#include "../core/region.h"
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

// 通用的位图widget渲染函数
// 清空区域，绘制位图（如果有），然后绘制边框
void render_bitmap_widget(const Rect& area, const String& bitmapBuffer);

// 通用的版本号同步函数
bool sync_version_from_hub(uint32_t& lastVersion, uint32_t currentVersion, bool& isDirty);

// 通用的边框绘制函数
void draw_widget_border(const Rect& area);

