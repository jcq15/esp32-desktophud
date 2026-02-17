#include <Arduino.h>
#include "display_hal.h"
#include "ui_pages.h"
#include "app_state.h"
#include "wifi_config.h"
#include "api_client.h"
#include "config/config.h"  // 统一配置文件

static AppState state;
static AppState* g_state = nullptr;

static void drawCb() {
    renderHome(*g_state);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    display_begin();

    g_state = &state;
    
    // 初始显示
    state.statusMessage = "正在连接WiFi...";
    display_full_refresh(drawCb);
    
    // 连接WiFi
    if (wifi_connect()) {
        state.wifiConnected = true;
        state.statusMessage = "WiFi已连接";
        
        // 立即获取一次API数据
        if (api_fetch_data(state.apiData)) {
            state.statusMessage = "数据获取成功";
            state.lastUpdateTime = millis();
        } else {
            state.statusMessage = "数据获取失败";
        }
    } else {
        state.wifiConnected = false;
        state.statusMessage = "WiFi连接失败";
    }
    
    // 刷新显示
    display_full_refresh(drawCb);
}

void loop() {
    unsigned long currentTime = millis();
    
    // 检查WiFi连接状态
    if (!wifi_is_connected()) {
        state.wifiConnected = false;
        state.statusMessage = "WiFi已断开，正在重连...";
        display_full_refresh(drawCb);
        
        if (wifi_connect()) {
            state.wifiConnected = true;
            state.statusMessage = "WiFi重连成功";
        }
    } else {
        state.wifiConnected = true;
    }
    
    // 定时更新API数据
    if (state.wifiConnected && 
        (currentTime - state.lastUpdateTime >= API_UPDATE_INTERVAL_MS || 
         state.lastUpdateTime == 0)) {
        
        state.statusMessage = "正在获取数据...";
        display_full_refresh(drawCb);
        
        if (api_fetch_data(state.apiData)) {
            state.statusMessage = "数据已更新";
            state.lastUpdateTime = currentTime;
        } else {
            state.statusMessage = "数据获取失败";
        }
        
        // 更新后刷新显示
        display_full_refresh(drawCb);
    }
    
    delay(1000);
}
