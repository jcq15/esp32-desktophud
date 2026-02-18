#pragma once
#include <Arduino.h>

// 屏幕尺寸（横向）
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// 区域定义
enum class Region {
    SENTENCE,  // 句子（最上面一横条）
    TIME,      // 时间（高频，分钟级）
    CALENDAR, // 日期/星期/农历（低频，日级）
    WEATHER,  // 天气（低频，30-60分钟）
    NOTE,     // 自定义内容（低频/手动/服务器驱动）
    STATUS,   // WiFi/最后同步时间/错误提示（中频）
    FREE      // 自由区域
};

// 矩形区域
struct Rect {
    int x, y, w, h;
    
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
};

// 获取区域的矩形定义
Rect getRegionRect(Region region);

