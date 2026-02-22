#pragma once
#include <Arduino.h>
#include "../config/config.h"  // 从统一配置文件读取配置

// 前向声明
class Scheduler;

// WiFi连接函数
// statusCallback: 用于更新状态消息的回调函数（可选）
bool wifi_connect(Scheduler* scheduler = nullptr);
bool wifi_is_connected();

