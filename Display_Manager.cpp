#include "Display_Manager.h"
#include <ArduinoJson.h>

// ==== 内部状態 ====
static Adafruit_NeoMatrix matrix(
  DISP_W, DISP_H, DISP_LED_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  DISP_PIXEL_TYPE
);

static unsigned long s_until_ms = 0;
static int s_scrollX = 0;
static uint8_t s_brightness = 20;

// ==== 共通初期化 ====
void Display_Init(uint8_t global_brightness) {
  s_brightness = global_brightness;
  matrix.begin();
  matrix.setBrightness(s_brightness);
  matrix.setTextWrap(false);
  matrix.fillScreen(0);
  matrix.show();
  s_scrollX = matrix.width();
  s_until_ms = 0;
}

void Display_SetBrightness(uint8_t b) {
  s_brightness = b;
  matrix.setBrightness(b);
}

void Display_Clear() {
  matrix.fillScreen(0);
  matrix.show();
  s_until_ms = 0;
}

// ==== TEXT ====
static int getTextWidth(const char* text) {
  if (!text) return 0;
  return strlen(text) * 6;  // 1文字6px想定
}

void Display_ShowText(const char* text) {
  if (!text) return;
  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.setTextColor(matrix.Color(255, 255, 255));
  matrix.print(text);
  matrix.show();
}

void Display_FlowText(const char* text) {
  if (!text) return;
  int textWidth = getTextWidth(text);
  matrix.setBrightness(s_brightness);
  matrix.fillScreen(0);
  matrix.setCursor(s_scrollX, 0);
  matrix.print(text);
  if (--s_scrollX < -textWidth) s_scrollX = matrix.width();
  matrix.show();
}

void Display_PlayTextOnce(const char* text, uint16_t frame_delay_ms) {
  if (!text) return;
  int textWidth = getTextWidth(text);
  s_scrollX = matrix.width();
  while (s_scrollX >= -textWidth) {
    matrix.fillScreen(0);
    matrix.setCursor(s_scrollX, 0);
    matrix.print(text);
    matrix.show();
    s_scrollX--;
    delay(frame_delay_ms);
  }
}

// ==== IMAGE ====
// static void drawRGBArrayRotCCW(const uint8_t* rgb, size_t n) {
//   if (!rgb) return;
//   if (n < (size_t)(DISP_W * DISP_H * 3)) return;

//   matrix.fillScreen(0);

//   for (int sy = 0; sy < DISP_H; ++sy) {
//     for (int sx = 0; sx < DISP_W; ++sx) {
//       // ZIGZAG 反転は不要（プログレッシブ配線なので削除）
//       size_t i = (size_t)(sy * DISP_W + sx) * 3;

//       // 90度反時計回りに回転
//       int dx = sy;
//       int dy = DISP_W - 1 - sx;

//       matrix.drawPixel(dx, dy, matrix.Color(rgb[i + 1], rgb[i], rgb[i + 2]));
//     }
//   }

//   matrix.show();
// }

static void drawRGBArrayRotCCW(const uint8_t* rgb, size_t n) {
  if (!rgb) return;
  if (n < (size_t)(DISP_W * DISP_H * 3)) return;

  matrix.fillScreen(0);

  for (int sy = 0; sy < DISP_H; ++sy) {
    for (int sx = 0; sx < DISP_W; ++sx) {
      // ZIGZAG 反転は不要（プログレッシブ配線なので削除）
      size_t i = (size_t)(sy * DISP_W + sx) * 3;

      // 90度反時計回りに回転
      // int dx = sy;
      // int dy = DISP_W - 1 - sx;

      // 🔸 180度回転（上下左右を反転）
      int dx = DISP_W - 1 - sx;
      int dy = DISP_H - 1 - sy;

      matrix.drawPixel(dx, dy, matrix.Color(rgb[i + 1], rgb[i], rgb[i + 2]));
    }
  }

  matrix.show();
}




bool Display_ShowRGBRotCCW(const uint8_t* rgb, size_t n, unsigned long display_ms) {
  if (!rgb) return false;
  if (n < (size_t)(DISP_W * DISP_H * 3)) return false;
  drawRGBArrayRotCCW(rgb, n);
  s_until_ms = millis() + display_ms;
  return true;
}

// ==== JSON ====
bool Display_ShowFromJson(const uint8_t* buf, size_t len, unsigned long display_ms) {
  if (!buf || len == 0) return false;

  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, buf, len)) {
    Serial.println("❌ JSON parse error");
    return false;
  }

  if (doc.containsKey("brightness")) {
    Display_SetBrightness(constrain(doc["brightness"].as<int>(), 0, 255));
  }

  String flag = doc["flag"] | "text";
  if (flag == "text") {
    const char* text = doc["text"] | "";
    Display_ShowText(text);
    return true;
  }

  if (doc["rgb"].is<JsonArray>()) {
    JsonArray arr = doc["rgb"].as<JsonArray>();
    size_t need = DISP_W * DISP_H * 3;
    if (arr.size() < need) return false;
    static uint8_t rgbBuf[DISP_W * DISP_H * 3];
    for (size_t i = 0; i < need; ++i) rgbBuf[i] = arr[i];
    Display_ShowRGBRotCCW(rgbBuf, need, display_ms);
    return true;
  }

  return false;
}

// ==== 状態管理 ====
bool Display_IsActive() {
  return (s_until_ms != 0) && (millis() < s_until_ms);
}

bool Display_EndIfExpired() {
  if (s_until_ms && millis() >= s_until_ms) {
    Display_Clear();
    return true;
  }
  return false;
}

void Display_BlockFor(unsigned long ms) {
  if (ms == 0) return;
  s_until_ms = millis() + ms;
}
