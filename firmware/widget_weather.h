#pragma once
#include "widget.h"

class WeatherWidget : public Widget {
private:
    uint32_t lastVersion = 0;
    
public:
    WeatherWidget() : Widget(Region::WEATHER) {
        updateIntervalMinutes = 30;  // 每30分钟更新
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

