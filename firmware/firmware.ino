#include <Arduino.h>
#include "hal/display_hal.h"
#include "core/scheduler.h"
#include "hal/wifi_config.h"
#include "config/config.h"
#include <time.h>

// Arduino IDE 有时不会自动编译子目录中的 .cpp 文件
// 这里显式包含以确保它们被编译（使用条件编译避免重复定义）
#ifndef ARDUINO_CPP_INCLUDED
#define ARDUINO_CPP_INCLUDED
#include "hal/display_hal.cpp"
#include "hal/wifi_config.cpp"
#include "core/data_hub.cpp"
#include "core/region.cpp"
#include "core/scheduler.cpp"
#include "ui/bitmap_utils.cpp"
#include "ui/ui_pages.cpp"
#include "ui/seven_segment.cpp"
#include "widgets/widget_utils.cpp"
#include "widgets/widget_sentence.cpp"
#include "widgets/widget_time.cpp"
#include "widgets/widget_calendar.cpp"
#include "widgets/widget_weather.cpp"
#include "widgets/widget_note.cpp"
#include "widgets/widget_status.cpp"
#include "widgets/widget_free.cpp"
#endif

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
    
    // 初始化调度器
    scheduler.begin();

    // 调试信息
    Serial.printf("Heap: %u\n", ESP.getFreeHeap());
    Serial.printf("PSRAM: %u\n", ESP.getFreePsram());
    
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
            int count = scheduler.getWidgetCount();
            ws[count - 1]->render(ws[count - 1]->rect);
        },
        statusRect.x, statusRect.y, statusRect.w, statusRect.h
    );
    
    if (wifi_connect(&scheduler)) {
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
            // 重新绘制边框（因为widget的fillRect会覆盖边框）
            display_draw_borders_in_callback();
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
    // 检查串口调试命令
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();  // 去除首尾空白字符
        
        if (command == "refresh" || command == "r" || command == "R") {
            Serial.println("[Debug] Full refresh command received");
            scheduler.forceFullRefresh();
        } else if (command.length() > 0) {
            Serial.print("[Debug] Unknown command: ");
            Serial.println(command);
            Serial.println("[Debug] Available commands: refresh, r, R");
        }
    }
    
    // 检查WiFi连接状态
    if (!wifi_is_connected()) {
        scheduler.updateWifiStatus(false);
        
        // 尝试重连
        if (wifi_connect(&scheduler)) {
            scheduler.updateWifiStatus(true);
            initNTP();  // 重新同步时间
        }
    } else {
        scheduler.updateWifiStatus(true);
    }
    
    // 调度器主循环
    scheduler.loop();
}
