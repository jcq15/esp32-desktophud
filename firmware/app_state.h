#pragma once
#include <Arduino.h>
#include "api_client.h"

struct AppState {
    // API数据
    ApiResponse apiData;
    
    // WiFi连接状态
    bool wifiConnected = false;
    
    // 最后更新时间
    unsigned long lastUpdateTime = 0;
    
    // 显示状态信息
    String statusMessage = "初始化中...";
};
