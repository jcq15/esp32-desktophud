#pragma once
#include "widget.h"

class TimeWidget : public Widget {
private:
    String lastMinuteString = "";
    
public:
    TimeWidget() : Widget(Region::TIME) {
        updateIntervalMinutes = 0;  // 每分钟更新
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

