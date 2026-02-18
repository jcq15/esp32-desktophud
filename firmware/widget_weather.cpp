#include "widget_weather.h"
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>
#include "bitmap_utils.h"

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;
extern DataHub dataHub;

bool WeatherWidget::syncFromHub(const DataHub& hub) {
    if (hub.weather.version != lastVersion) {
        lastVersion = hub.weather.version;
        isDirty = true;
        return true;
    }
    return false;
}

void WeatherWidget::render(const Rect& area) {
    // 清空区域
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    
    // 如果有位图数据，绘制位图
    if (dataHub.weather.bitmapBuffer.length() > 0) {
        drawBitmapFromBase64(dataHub.weather.bitmapBuffer, display, area.x, area.y, area.w, area.h);
    }
}

