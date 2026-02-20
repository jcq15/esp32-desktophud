#include "seven_segment.h"

// 七段数码管的段定义（a-g对应7个段）
// 每个数字用7位表示，从高位到低位：a, b, c, d, e, f, g
const uint8_t SEGMENT_PATTERNS[10] = {
    0b1111110,  // 0: a,b,c,d,e,f
    0b0110000,  // 1: b,c
    0b1101101,  // 2: a,b,d,e,g
    0b1111001,  // 3: a,b,c,d,g
    0b0110011,  // 4: b,c,f,g
    0b1011011,  // 5: a,c,d,f,g
    0b1011111,  // 6: a,c,d,e,f,g
    0b1110000,  // 7: a,b,c
    0b1111111,  // 8: a,b,c,d,e,f,g
    0b1111011   // 9: a,b,c,d,f,g
};

// 绘制单个段（水平或垂直）
void drawSegment(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                 int x1, int y1, int x2, int y2, int lineWidth) {
    // 使用 fillRect 绘制粗线
    if (x1 == x2) {
        // 垂直线
        int rectX = x1 - lineWidth / 2;
        int rectY = min(y1, y2);
        int rectW = lineWidth;
        int rectH = abs(y2 - y1) + 1;
        display.fillRect(rectX, rectY, rectW, rectH, GxEPD_BLACK);
    } else {
        // 水平线
        int rectX = min(x1, x2);
        int rectY = y1 - lineWidth / 2;
        int rectW = abs(x2 - x1) + 1;
        int rectH = lineWidth;
        display.fillRect(rectX, rectY, rectW, rectH, GxEPD_BLACK);
    }
}

// 绘制单个数字
void drawSevenSegmentDigit(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                          int x, int y, int width, int height, int digit, int lineWidth) {
    if (digit < 0 || digit > 9) {
        return;
    }
    
    uint8_t pattern = SEGMENT_PATTERNS[digit];
    
    // 计算段的坐标
    int padding = lineWidth / 2 + 2;
    int segLength = width - padding * 2;  // 水平段的长度
    int segHeight = (height - padding * 3) / 2;  // 垂直段的高度
    
    int x1 = x + padding;
    int x2 = x + width - padding;
    int y1 = y + padding;
    int y2 = y + padding + segHeight;
    int y3 = y + padding * 2 + segHeight;
    int y4 = y + height - padding;
    
    // 段 a (上横)
    if (pattern & 0b1000000) {
        drawSegment(display, x1, y1, x2, y1, lineWidth);
    }
    
    // 段 b (右上竖)
    if (pattern & 0b0100000) {
        drawSegment(display, x2, y1, x2, y2, lineWidth);
    }
    
    // 段 c (右下竖)
    if (pattern & 0b0010000) {
        drawSegment(display, x2, y3, x2, y4, lineWidth);
    }
    
    // 段 d (下横)
    if (pattern & 0b0001000) {
        drawSegment(display, x1, y4, x2, y4, lineWidth);
    }
    
    // 段 e (左下竖)
    if (pattern & 0b0000100) {
        drawSegment(display, x1, y3, x1, y4, lineWidth);
    }
    
    // 段 f (左上竖)
    if (pattern & 0b0000010) {
        drawSegment(display, x1, y1, x1, y2, lineWidth);
    }
    
    // 段 g (中横)
    if (pattern & 0b0000001) {
        drawSegment(display, x1, y3, x2, y3, lineWidth);
    }
}

// 绘制冒号
void drawSevenSegmentColon(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                          int x, int y, int digitHeight) {
    // 冒号应该比数字小很多，使用数字高度的1/4作为参考
    int colonHeight = digitHeight / 4;
    int dotSize = colonHeight / 3;  // 圆点更小
    int centerY = y + digitHeight / 2;
    int dotY1 = centerY - colonHeight / 2;
    int dotY2 = centerY + colonHeight / 2;
    int centerX = x + 10;  // 固定宽度，不需要和数字一样宽
    
    // 上点
    display.fillCircle(centerX, dotY1, dotSize, GxEPD_BLACK);
    // 下点
    display.fillCircle(centerX, dotY2, dotSize, GxEPD_BLACK);
}

// 绘制时间 HH:MM
void drawSevenSegmentTime(GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                         int x, int y, int width, int height, const String& timeStr) {
    if (timeStr.length() < 5) {
        return;
    }
    
    // 解析时间字符串 "HH:MM"
    int h1 = timeStr.charAt(0) - '0';
    int h2 = timeStr.charAt(1) - '0';
    int m1 = timeStr.charAt(3) - '0';
    int m2 = timeStr.charAt(4) - '0';
    
    // 缩小整体尺寸，增加边距（使用80%的区域）
    int marginX = width * 0.1;  // 左右各10%边距
    int marginY = height * 0.15;  // 上下各15%边距
    int availableWidth = width - marginX * 2;
    int availableHeight = height - marginY * 2;
    
    // 计算每个数字的尺寸（缩小）
    int digitWidth = (availableWidth - 60) / 4;  // 4个数字，留出间距和冒号空间
    int digitHeight = availableHeight;  // 使用可用高度
    int lineWidth = 8;  // 稍微减小线宽
    
    int spacing = 12;  // 数字之间的间距
    int colonWidth = 20;  // 冒号宽度（固定）
    
    // 计算起始位置（居中）
    int totalWidth = digitWidth * 4 + colonWidth + spacing * 3;
    int startX = x + marginX + (availableWidth - totalWidth) / 2;
    int startY = y + marginY;
    
    // 绘制 H1
    drawSevenSegmentDigit(display, startX, startY, digitWidth, digitHeight, h1, lineWidth);
    startX += digitWidth + spacing;
    
    // 绘制 H2
    drawSevenSegmentDigit(display, startX, startY, digitWidth, digitHeight, h2, lineWidth);
    startX += digitWidth + spacing;
    
    // 绘制冒号（使用更小的尺寸）
    drawSevenSegmentColon(display, startX, startY, digitHeight);
    startX += colonWidth + spacing;
    
    // 绘制 M1
    drawSevenSegmentDigit(display, startX, startY, digitWidth, digitHeight, m1, lineWidth);
    startX += digitWidth + spacing;
    
    // 绘制 M2
    drawSevenSegmentDigit(display, startX, startY, digitWidth, digitHeight, m2, lineWidth);
}

