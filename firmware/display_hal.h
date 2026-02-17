#pragma once
#include <Arduino.h>

// 初始化屏幕
void display_begin();

// 全屏刷新：传入一个绘制回调
void display_full_refresh(void (*drawFn)());
