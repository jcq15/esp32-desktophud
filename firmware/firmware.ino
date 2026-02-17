#include <Arduino.h>
#include "display_hal.h"
#include "scheduler.h"
#include "wifi_config.h"
#include "config/config.h"
#include <time.h>

static Scheduler scheduler;

// 初始化NTP时间同步
void initNTP() {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // UTC+8 (中国时区)
    Serial.println("等待NTP时间同步...");
    
    struct tm timeinfo;
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (getLocalTime(&timeinfo)) {
        Serial.println();
        Serial.println("NTP时间同步成功!");
    } else {
        Serial.println();
        Serial.println("NTP时间同步失败");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);

    // 初始化显示
    display_begin();

    // 初始全屏刷新
    display_full_refresh([]() {
        // 空白屏幕
    });
    
    // 初始化调度器
    scheduler.begin();
    
    // 显示"正在初始化..."状态
    extern DataHub dataHub;
    dataHub.statusMessage = "Initializing...";
    scheduler.updateStatusMessage("Initializing...");
    
    // 显示状态（首次刷新）
    display_full_refresh([]() {
        extern Scheduler scheduler;
        Widget** widgets = scheduler.getWidgets();
        int count = scheduler.getWidgetCount();
        // 只渲染状态Widget（最后一个）
        widgets[count - 1]->render(widgets[count - 1]->rect);
    });
    
    // 连接WiFi
    dataHub.statusMessage = "Connecting WiFi...";
    scheduler.updateStatusMessage("Connecting WiFi...");
    Widget** widgets = scheduler.getWidgets();
    int widgetCount = scheduler.getWidgetCount();
    Rect statusRect = widgets[widgetCount - 1]->rect;
    display_partial_refresh_area(
        []() {
            extern Scheduler scheduler;
            Widget** ws = scheduler.getWidgets();
            ws[4]->render(ws[4]->rect);
        },
        statusRect.x, statusRect.y, statusRect.w, statusRect.h
    );
    
    if (wifi_connect()) {
        scheduler.updateWifiStatus(true);
        
        // 初始化NTP时间同步
        dataHub.statusMessage = "Syncing time...";
        scheduler.updateStatusMessage("Syncing time...");
        display_partial_refresh_area(
            []() {
                extern Scheduler scheduler;
                Widget** ws = scheduler.getWidgets();
                int count = scheduler.getWidgetCount();
                ws[count - 1]->render(ws[count - 1]->rect);
            },
            statusRect.x, statusRect.y, statusRect.w, statusRect.h
        );
        
        initNTP();
        scheduler.updateLocalTime();
        
        // 首次从服务器获取数据
        dataHub.statusMessage = "Fetching data...";
        scheduler.updateStatusMessage("Fetching data...");
        display_partial_refresh_area(
            []() {
                extern Scheduler scheduler;
                Widget** ws = scheduler.getWidgets();
                int count = scheduler.getWidgetCount();
                ws[count - 1]->render(ws[count - 1]->rect);
            },
            statusRect.x, statusRect.y, statusRect.w, statusRect.h
        );
        
        scheduler.performInitialFetch();
        
        // 清除状态信息
        dataHub.statusMessage = "";
        scheduler.updateStatusMessage("");
        
        // 首次全屏刷新显示所有Widget
        display_full_refresh([]() {
            extern Scheduler scheduler;
            Widget** widgets = scheduler.getWidgets();
            int count = scheduler.getWidgetCount();
            for (int i = 0; i < count; i++) {
                widgets[i]->render(widgets[i]->rect);
            }
        });
    } else {
        scheduler.updateWifiStatus(false);
        dataHub.statusMessage = "WiFi failed";
        scheduler.updateStatusMessage("WiFi failed");
        display_partial_refresh_area(
            []() {
                extern Scheduler scheduler;
                Widget** ws = scheduler.getWidgets();
                int count = scheduler.getWidgetCount();
                ws[count - 1]->render(ws[count - 1]->rect);
            },
            statusRect.x, statusRect.y, statusRect.w, statusRect.h
        );
    }
}

void loop() {
    // 检查WiFi连接状态
    if (!wifi_is_connected()) {
        scheduler.updateWifiStatus(false);
        
        // 尝试重连
        if (wifi_connect()) {
            scheduler.updateWifiStatus(true);
            initNTP();  // 重新同步时间
        }
    } else {
        scheduler.updateWifiStatus(true);
    }
    
    // 调度器主循环
    scheduler.loop();
}
