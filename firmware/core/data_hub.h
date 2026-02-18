#pragma once
#include <Arduino.h>

// 数据版本号
struct DataVersions {
    uint32_t sentence_ver = 0;  // 句子版本
    uint32_t wx_ver = 0;    // 天气版本
    uint32_t cal_ver = 0;   // 日历版本
    uint32_t note_ver = 0;  // 笔记版本
    uint32_t global_ver = 0; // 全局版本（可选）
};

// 天气数据
struct WeatherData {
    String bitmapBuffer = "";  // base64编码的位图数据
    uint32_t version = 0;
};

// 日历数据
struct CalendarData {
    String bitmapBuffer = "";  // base64编码的位图数据
    uint32_t version = 0;
};

// 笔记数据
struct NoteData {
    String bitmapBuffer = "";  // base64编码的位图数据
    uint32_t version = 0;
};

// 句子数据
struct SentenceData {
    String bitmapBuffer = "";  // base64编码的位图数据
    uint32_t version = 0;
};

// 数据中心：统一管理所有数据
class DataHub {
public:
    DataVersions versions;
    SentenceData sentence;
    WeatherData weather;
    CalendarData calendar;
    NoteData notes;
    
    // 本地数据
    String localTimeString = "";
    bool wifiConnected = false;
    String lastSyncTime = "";
    String errorMessage = "";
    String statusMessage = "";  // 启动状态信息（正在连WiFi、正在校准时间等）
    
    // 下次获取时间
    time_t nextFetchTime = 0;  // 下次获取时间（Unix时间戳）
    int fetchIntervalMinutes = 1;  // 获取间隔（分钟） // 调试阶段设置1分钟
    
    // 重试机制
    int fetchRetryCount = 0;  // 当前重试次数
    static const int MAX_FETCH_RETRIES = 3;  // 最大重试次数
    unsigned long lastFetchAttemptTime = 0;  // 上次尝试获取的时间（millis）
    static const unsigned long FETCH_RETRY_DELAY_MS = 10000;  // 重试延迟（10秒）
    
    // 从服务器JSON更新数据
    bool updateFromJson(const String& json);
    
    // 检查是否需要获取数据
    bool shouldFetch() {
        if (nextFetchTime == 0) {
            // 首次调用，初始化下次获取时间
            scheduleNextFetch();
            return false;
        }
        
        time_t now;
        time(&now);
        return now >= nextFetchTime;
    }
    
    // 标记已获取，并安排下次获取时间（对齐到整点）
    void markFetched() {
        scheduleNextFetch();
    }
    
private:
    // 安排下次获取时间（对齐到整点）
    void scheduleNextFetch() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return;  // 时间未同步，无法安排
        }
        
        // 对齐到下一个5分钟的整数倍的0秒
        int nextMinute = ((timeinfo.tm_min / fetchIntervalMinutes) + 1) * fetchIntervalMinutes;
        if (nextMinute >= 60) {
            timeinfo.tm_hour += 1;
            timeinfo.tm_min = 0;
        } else {
            timeinfo.tm_min = nextMinute;
        }
        timeinfo.tm_sec = 0;
        
        nextFetchTime = mktime(&timeinfo);
    }
};

