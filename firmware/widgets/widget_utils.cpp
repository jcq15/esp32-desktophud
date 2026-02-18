#include "widget_utils.h"
#include "../ui/bitmap_utils.h"

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;

// 通用的位图widget渲染函数
void render_bitmap_widget(const Rect& area, const String& bitmapBuffer) {
    // 清空区域
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    
    // 如果有位图数据，绘制位图
    if (bitmapBuffer.length() > 0) {
        drawBitmapFromBase64(bitmapBuffer, display, area.x, area.y, area.w, area.h);
    }
    
    // 绘制边框
    draw_widget_border(area);
}

// 通用的版本号同步函数
bool sync_version_from_hub(uint32_t& lastVersion, uint32_t currentVersion, bool& isDirty) {
    if (currentVersion != lastVersion) {
        lastVersion = currentVersion;
        isDirty = true;
        return true;
    }
    return false;
}

// 通用的边框绘制函数
void draw_widget_border(const Rect& area) {
    display.drawRect(area.x, area.y, area.w, area.h, GxEPD_BLACK);
}

