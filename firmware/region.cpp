#include "region.h"

Rect getRegionRect(Region region) {
    // 横向布局：800x480
    // 根据实际需求调整各区域位置和大小
    switch (region) {
        case Region::TIME:
            // 时间区域：右上角
            return Rect(600, 0, 200, 80);
            
        case Region::CALENDAR:
            // 日历区域：左上角
            return Rect(0, 0, 300, 120);
            
        case Region::WEATHER:
            // 天气区域：左上角下方
            return Rect(0, 120, 300, 150);
            
        case Region::NOTE:
            // 笔记区域：中间
            return Rect(300, 0, 300, 300);
            
        case Region::STATUS:
            // 状态区域：底部
            return Rect(0, 420, 800, 60);
            
        default:
            return Rect(0, 0, 0, 0);
    }
}

