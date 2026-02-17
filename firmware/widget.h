#pragma once
#include <Arduino.h>
#include <time.h>
#include "region.h"
#include "data_hub.h"

// 渲染模式
enum class RenderMode {
    PARTIAL,  // 局部刷新
    FULL,     // 全屏刷新
    MERGED    // 合并刷新（多个区域一起）
};

// Widget基类
class Widget {
public:
    Region region;
    Rect rect;
    RenderMode renderMode = RenderMode::PARTIAL;
    int updateIntervalMinutes = 0;  // 更新间隔（分钟），0表示每分钟，5表示每5分钟，1440表示每天等
    time_t nextUpdateTime = 0;  // 下次更新时间（Unix时间戳）
    bool isDirty = false;
    
    Widget(Region r) : region(r) {
        rect = getRegionRect(r);
        rect.alignTo16();  // 对齐到16的倍数
    }
    
    virtual ~Widget() {}
    
    // 从DataHub同步数据，返回是否dirty
    virtual bool syncFromHub(const DataHub& hub) = 0;
    
    // 渲染到指定区域
    virtual void render(const Rect& area) = 0;
    
    // 检查是否需要更新
    bool shouldUpdate() {
        if (nextUpdateTime == 0) {
            // 首次调用，初始化下次更新时间
            scheduleNextUpdate();
            return false;
        }
        
        time_t now;
        time(&now);
        return now >= nextUpdateTime;
    }
    
    // 标记为已更新，并安排下次更新时间（对齐到整点）
    void markUpdated() {
        scheduleNextUpdate();
        isDirty = false;
    }
    
private:
    // 安排下次更新时间（对齐到整点）
    void scheduleNextUpdate() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return;  // 时间未同步，无法安排
        }
        
        if (updateIntervalMinutes == 0) {
            // 每分钟更新：对齐到下一分钟的0秒
            timeinfo.tm_sec = 0;
            timeinfo.tm_min += 1;
        } else if (updateIntervalMinutes < 60) {
            // 每N分钟更新：对齐到下一个N分钟整数倍的0秒
            int nextMinute = ((timeinfo.tm_min / updateIntervalMinutes) + 1) * updateIntervalMinutes;
            if (nextMinute >= 60) {
                timeinfo.tm_hour += 1;
                timeinfo.tm_min = 0;
            } else {
                timeinfo.tm_min = nextMinute;
            }
            timeinfo.tm_sec = 0;
        } else if (updateIntervalMinutes == 1440) {
            // 每天更新：对齐到明天的0点0分0秒
            timeinfo.tm_sec = 0;
            timeinfo.tm_min = 0;
            timeinfo.tm_hour = 0;
            timeinfo.tm_mday += 1;
        } else {
            // 其他间隔：对齐到下一个间隔的0秒
            int hours = updateIntervalMinutes / 60;
            int minutes = updateIntervalMinutes % 60;
            timeinfo.tm_sec = 0;
            timeinfo.tm_min += minutes;
            if (timeinfo.tm_min >= 60) {
                timeinfo.tm_hour += 1;
                timeinfo.tm_min -= 60;
            }
            timeinfo.tm_hour += hours;
            if (timeinfo.tm_hour >= 24) {
                timeinfo.tm_mday += 1;
                timeinfo.tm_hour -= 24;
            }
        }
        
        nextUpdateTime = mktime(&timeinfo);
    }
};

