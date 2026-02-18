#include "ui_pages.h"
#include <Adafruit_GFX.h>

// 居中绘制文本
void drawCenteredText(const GFXfont* font, 
                      const String& text,
                      int x, int y, int w, int h) {
    display.setFont(font);
    display.setTextColor(GxEPD_BLACK);

    int16_t x1, y1;
    uint16_t tw, th;
    display.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);

    int cx = x + (w - tw) / 2;
    int cy = y + (h + th) / 2;

    display.setCursor(cx, cy);
    display.print(text);
}
