#pragma once
#include <Arduino.h>
#include "config/config.h"  // 从统一配置文件读取配置

// WiFi连接函数
bool wifi_connect();
bool wifi_is_connected();

