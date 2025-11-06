// true: WiFiでLLMサーバーと通信、false: シリアルでLLMサーバーと通信 (デフォルト: true)
#define USE_WIFI_FOR_LLM_COMMUNICATION true

// WiFiを使用するとき true: ステーションモード(既存のWiFiを使用)、false: APモード(これがAPになる) (デフォルト: true)
#define USE_STATION_MODE true

#include <M5Unified.h>
#include "common.h"

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(9600);

  initLED();
  led_sayStart_initialize();
  initSPIFFS();
  init_communication();
}

void loop() {
  M5.update();
}