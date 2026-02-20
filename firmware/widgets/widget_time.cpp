#include "widget_time.h"
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>
#include "../ui/seven_segment.h"
#include "widget_utils.h"

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;

bool TimeWidget::syncFromHub(const DataHub& hub) {
    // 提取分钟部分（HH:MM）
    String currentMinute = hub.localTimeString;
    if (currentMinute.length() >= 5) {
        currentMinute = currentMinute.substring(0, 5);  // 取前5个字符 "HH:MM"
    }
    if (currentMinute != lastMinuteString) {
        lastMinuteString = currentMinute;
        isDirty = true;
        return true;
    }
    return false;
}

void TimeWidget::render(const Rect& area) {
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    
    // 使用七段数码管显示时间
    if (lastMinuteString.length() >= 5) {
        drawSevenSegmentTime(display, area.x, area.y, area.w, area.h, lastMinuteString);
    }
    
    draw_widget_border(area);
}

