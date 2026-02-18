#include "region.h"

Rect getRegionRect(Region region) {
    // 横向布局：800x480
    // 根据实际需求调整各区域位置和大小
    switch (region) {
        case Region::SENTENCE:
            // 句子区域（最上面一横条）
            return Rect(0, 0, 800, 56);
            
        case Region::TIME:
            // 时间区域
            return Rect(0, 176, 496, 176);
            
        case Region::CALENDAR:
            // 日历区域
            return Rect(0, 56, 496, 120);
            
        case Region::WEATHER:
            // 天气区域
            return Rect(496, 56, 304, 176);
            
        case Region::NOTE:
            // 笔记区域
            return Rect(0, 352, 496, 128);
            
        case Region::STATUS:
            // 状态区域
            return Rect(496, 424, 304, 56);
        
        case Region::FREE:
            // 自由区域
            return Rect(496, 56+176, 304, 480-56-176-56);
            
        default:
            return Rect(0, 0, 0, 0);
    }
}

