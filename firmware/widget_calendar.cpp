#include "widget_calendar.h"
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;

bool CalendarWidget::syncFromHub(const DataHub& hub) {
    String currentDate = hub.calendar.date + " " + hub.calendar.weekday;
    String currentLunar = hub.calendar.lunar;
    if (hub.calendar.extra.length() > 0) {
        currentLunar += " " + hub.calendar.extra;
    }
    
    if (currentDate != lastDateString || currentLunar != lastLunarString) {
        lastDateString = currentDate;
        lastLunarString = currentLunar;
        isDirty = true;
        return true;
    }
    return false;
}

void CalendarWidget::render(const Rect& area) {
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    int y = area.y + 30;
    
    // 日期和星期
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(area.x + 10, y);
    display.print(lastDateString);
    
    // 农历
    y += 40;
    display.setFont(&FreeMono9pt7b);
    display.setCursor(area.x + 10, y);
    display.print(lastLunarString);
}

