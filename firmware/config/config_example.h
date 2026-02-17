#pragma once
#include <Arduino.h>

// ============================================
// 配置文件示例
// ============================================
// 复制此文件为 config.h 并填入你的实际配置
// config.h 包含敏感信息，不会被提交到 Git
// ============================================

// ============================================
// WiFi 配置
// ============================================
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

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

