#pragma once
#include "widget.h"

class CalendarWidget : public Widget {
private:
    String lastDateString = "";
    String lastLunarString = "";
    
public:
    CalendarWidget() : Widget(Region::CALENDAR) {
        updateIntervalMinutes = 1440;  // 每天更新（1440分钟 = 24小时）
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

