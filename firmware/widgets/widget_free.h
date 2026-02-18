#pragma once
#include "widget.h"

class FreeWidget : public Widget {
public:
    FreeWidget() : Widget(Region::FREE) {
        updateIntervalMinutes = 0;  // 不自动更新
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

