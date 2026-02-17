#include "widget_status.h"
#include <Fonts/FreeMono9pt7b.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;
extern DataHub dataHub;

bool StatusWidget::syncFromHub(const DataHub& hub) {
    bool changed = false;
    if (hub.wifiConnected != lastWifiState) {
        lastWifiState = hub.wifiConnected;
        changed = true;
    }
    if (hub.lastSyncTime != lastSyncTime) {
        lastSyncTime = hub.lastSyncTime;
        changed = true;
    }
    if (hub.errorMessage.length() > 0) {
        changed = true;
    }
    if (hub.statusMessage != lastStatusMessage) {
        lastStatusMessage = hub.statusMessage;
        changed = true;
    }
    
    if (changed) {
        isDirty = true;
        return true;
    }
    return false;
}

void StatusWidget::render(const Rect& area) {
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMono9pt7b);
    
    int x = area.x + 10;
    int y = area.y + 30;
    
    // 状态信息（启动时显示）
    if (dataHub.statusMessage.length() > 0) {
        display.setCursor(x, y);
        display.print(dataHub.statusMessage);
        y += 25;
    }
    
    // WiFi状态
    display.setCursor(x, y);
    if (dataHub.wifiConnected) {
        display.print("WiFi: Connected");
    } else {
        display.print("WiFi: Disconnected");
    }
    
    // 最后同步时间
    if (dataHub.lastSyncTime.length() > 0) {
        x += 150;
        display.setCursor(x, y);
        display.print("Sync: ");
        display.print(dataHub.lastSyncTime);
    }
    
    // 错误信息
    if (dataHub.errorMessage.length() > 0) {
        x = area.x + 10;
        y += 25;
        display.setCursor(x, y);
        display.print("Error: ");
        display.print(dataHub.errorMessage);
    }
}

