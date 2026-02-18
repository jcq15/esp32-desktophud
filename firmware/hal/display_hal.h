#pragma once
#include <Arduino.h>

// 初始化屏幕
void display_begin();

// 全屏刷新：传入一个绘制回调（首次使用，会有闪烁）
void display_full_refresh(void (*drawFn)());

// 局部刷新：传入一个绘制回调（无闪烁，适合频繁更新）
void display_partial_refresh(void (*drawFn)());

// 局部刷新指定区域：传入绘制回调和区域坐标
void display_partial_refresh_area(void (*drawFn)(), int x, int y, int w, int h);

// 绘制所有区域的边框（固定，不刷新）
void display_draw_region_borders();

// 在绘制回调中绘制边框（用于全屏刷新时）
void display_draw_borders_in_callback();

