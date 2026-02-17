#pragma once
#include "widget.h"

class StatusWidget : public Widget {
private:
    bool lastWifiState = false;
    String lastSyncTime = "";
    String lastStatusMessage = "";
    
public:
    StatusWidget() : Widget(Region::STATUS) {
        updateIntervalMinutes = 0;  // 每分钟更新
        renderMode = RenderMode::PARTIAL;
    }
    
    bool syncFromHub(const DataHub& hub) override;
    void render(const Rect& area) override;
};

