#pragma once
#include "widget.h"

class SentenceWidget : public Widget {
private:
    uint32_t lastVersion = 0;
    
public:
    SentenceWidget() : Widget(Region::SENTENCE) {
        updateIntervalMinutes = 30;  // 每30分钟更新
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

