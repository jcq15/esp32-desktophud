#include "widget_time.h"
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>
#include "ui_pages.h"

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
    //display.setTextColor(GxEPD_BLACK);
    //display.setFont(&FreeMonoBold24pt7b);
    drawCenteredText(&FreeMonoBold24pt7b, lastMinuteString, area.x, area.y, area.w, area.h);
}

