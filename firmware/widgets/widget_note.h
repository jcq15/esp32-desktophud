#pragma once
#include "widget.h"

class NoteWidget : public Widget {
private:
    uint32_t lastVersion = 0;
    
public:
    NoteWidget() : Widget(Region::NOTE) {
        updateIntervalMinutes = 5;  // 每5分钟更新
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

