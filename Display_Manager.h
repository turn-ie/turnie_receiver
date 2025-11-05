#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>

// ==== 基本設定 ====
#ifndef DISP_W
#define DISP_W 8
#endif

#ifndef DISP_H
#define DISP_H 8
#endif

#ifndef DISP_LED_PIN
#define DISP_LED_PIN 14
#endif

#ifndef DISP_PIXEL_TYPE
#define DISP_PIXEL_TYPE (NEO_GRB + NEO_KHZ800)
#endif

// ==== 共通インターフェース ====
void Display_Init(uint8_t global_brightness = 20);
void Display_SetBrightness(uint8_t b);
void Display_Clear();

// ---- TEXT ----
void Display_ShowText(const char* text);
void Display_FlowText(const char* text);     // 非ブロッキング（loopで連続呼び出し）
void Display_PlayTextOnce(const char* text, uint16_t frame_delay_ms);

// ---- IMAGE ----
bool Display_ShowRGBRotCCW(const uint8_t* rgb, size_t n, unsigned long display_ms);

// ---- JSON ----
bool Display_ShowFromJson(const uint8_t* buf, size_t len, unsigned long display_ms);

// ---- 状態管理 ----
bool Display_IsActive();
bool Display_EndIfExpired();
void Display_BlockFor(unsigned long ms);
