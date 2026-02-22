#pragma once
#include "widget.h"

class FreeWidget : public Widget {
private:
    uint32_t lastVersion = 0;  // 上次同步的版本号
    
public:
    FreeWidget() : Widget(Region::FREE) {
        updateIntervalMinutes = 0;  // 不自动更新
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

