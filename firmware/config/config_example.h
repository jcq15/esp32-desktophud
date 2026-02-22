#pragma once
#include <Arduino.h>

// ============================================
// 配置文件示例
// ============================================
// 复制此文件为 config.h 并填入你的实际配置
// config.h 包含敏感信息，不会被提交到 Git
// ============================================

// ============================================
// WiFi 配置 - 支持多个WiFi，按顺序尝试连接
// ============================================
// 格式：{SSID, PASSWORD}
// 可以添加多个WiFi配置，系统会从前往后依次尝试连接
struct WiFiConfig {
    const char* ssid;
    const char* password;
};

static const WiFiConfig WIFI_CONFIGS[] = {
    {"your_wifi_ssid_1", "your_wifi_password_1"},
    // 可以在这里添加更多WiFi配置，例如：
    // {"your_wifi_ssid_2", "your_wifi_password_2"},
    // {"your_wifi_ssid_3", "your_wifi_password_3"},
};

static const int WIFI_CONFIG_COUNT = sizeof(WIFI_CONFIGS) / sizeof(WIFI_CONFIGS[0]);

// ============================================
// WiFiManager配置（当所有WiFi配置都失败时，开启配置门户）
// ============================================
#define ENABLE_WIFI_MANAGER_PORTAL 1       // 是否启用WiFiManager配置门户（1=启用，0=禁用）
#define WIFI_MANAGER_AP_NAME "EINK-SETUP"  // 配置热点的名称
#define WIFI_MANAGER_AP_PASSWORD ""        // 配置热点的密码（空字符串表示无密码）
#define WIFI_MANAGER_TIMEOUT_SEC 180       // 配置门户超时时间（秒），超时后重启

// ============================================
// API 服务器配置
// ============================================
#define API_SERVER_URL "http://your-server.com/api"
#define API_UPDATE_INTERVAL_MS 300000  // 5分钟更新一次（可根据需要调整）

// API 认证（如果需要）
// #define API_TOKEN "your_api_token"
// #define API_KEY "your_api_key"

// ============================================
// SSH 配置（如果将来需要）
// ============================================
// #define SSH_HOST "your-ssh-host.com"
// #define SSH_PORT 22
// #define SSH_USER "your_username"
// #define SSH_PASSWORD "your_password"
// #define SSH_KEY_PATH "/path/to/private/key"

// ============================================
// 其他配置
// ============================================
// 可以在这里添加其他敏感配置项
// #define DATABASE_URL "your_database_url"
// #define MQTT_BROKER "your_mqtt_broker"
// #define MQTT_USERNAME "your_mqtt_username"
// #define MQTT_PASSWORD "your_mqtt_password"

