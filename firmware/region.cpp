#include "region.h"

Rect getRegionRect(Region region) {
    // 横向布局：800x480
    // 根据实际需求调整各区域位置和大小
    switch (region) {
        case Region::TIME:
            // 时间区域
            return Rect(0, 120, 500, 180);
            
        case Region::CALENDAR:
            // 日历区域
            return Rect(0, 0, 500, 120);
            
        case Region::WEATHER:
            // 天气区域
            return Rect(500, 0, 300, 120);
            
        case Region::NOTE:
            // 笔记区域
            return Rect(0, 300, 500, 180);
            
        case Region::STATUS:
            // 状态区域
            return Rect(500, 420, 300, 60);
            
        default:
            return Rect(0, 0, 0, 0);
    }
}

