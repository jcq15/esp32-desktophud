#include "widget_calendar.h"
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>
#include "bitmap_utils.h"

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;
extern DataHub dataHub;

bool CalendarWidget::syncFromHub(const DataHub& hub) {
    if (hub.calendar.version != lastVersion) {
        lastVersion = hub.calendar.version;
        isDirty = true;
        return true;
    }
    return false;
}

void CalendarWidget::render(const Rect& area) {
    // 清空区域
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    
    // 如果有位图数据，绘制位图
    if (dataHub.calendar.bitmapBuffer.length() > 0) {
        drawBitmapFromBase64(dataHub.calendar.bitmapBuffer, display, area.x, area.y, area.w, area.h);
    }
}

