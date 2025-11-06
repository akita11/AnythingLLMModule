// true: WiFiでLLMサーバーと通信、false: シリアルでLLMサーバーと通信 (デフォルト: true)
#define USE_WIFI_FOR_LLM_COMMUNICATION true

// WiFiを使用するとき true: ステーションモード(既存のWiFiを使用)、false: APモード(これがAPになる) (デフォルト: true)
#define USE_STATION_MODE true

#include <M5Unified.h>
#include <FastLED.h>
#include <SPIFFS.h>

#if USE_WIFI_FOR_LLM_COMMUNICATION
#include <WiFi.h>
#include <HTTPClient.h>
#endif

const CRGB COLOR_ACCESSED = CRGB(0, 255, 255); // シアン
const CRGB COLOR_RUNNING = CRGB(255, 255, 128); // 黄色っぽい白
const CRGB COLOR_ERROR = CRGB(255, 0, 0); // 赤
const CRGB COLOR_OK = CRGB(0, 255, 0); // 緑

#define LED_NUM 1
#define LED_PIN 21
CRGB leds[LED_NUM];

void blinkLED(const CRGB& color, const uint8_t& times, const uint16_t& interval_ms, const bool hold = false) {
    for (uint8_t i = 0; i < times; i++) {
        leds[0] = color;
        FastLED.show();
        delay(interval_ms);
        leds[0] = CRGB::Black;
        FastLED.show();
        delay(interval_ms);
    }
    if (hold) {
        leds[0] = color;
        FastLED.show();
    }
}

#include "secrets.h"
String wifi_ssid = WIFI_SSID;
String wifi_password = WIFI_PASSWORD;
String ap_ssid = AP_SSID;
String ap_password = AP_PASSWORD;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(9600);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_NUM);
  FastLED.setBrightness(64);
  blinkLED(COLOR_RUNNING, 2, 100);

  // SPIFFSのマウント失敗 -> 初回起動 -> フォーマット、2回目以降スルー
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed. Estimated it's first boot. Formatting...");
    blinkLED(COLOR_RUNNING, 2, 100);
    if (SPIFFS.format()) {
      Serial.println("SPIFFS formatted successfully.");
      blinkLED(COLOR_OK, 3, 100);
      blinkLED(COLOR_RUNNING, 2, 100);
      if (!SPIFFS.begin(false)) {
        while (1) {
          Serial.println("SPIFFS mount failed after format.");
          blinkLED(COLOR_ERROR, 10, 1000);
        }
      }
      blinkLED(COLOR_OK, 3, 100);
      Serial.println("SPIFFS mounted successfully.");
    } else {
      while (1) {
        Serial.println("SPIFFS format failed for unknown reason.");
        blinkLED(COLOR_ERROR, 20, 500);
        delay(500);
      }
    }
  }
  else {
    Serial.println("SPIFFS mounted successfully.");
    blinkLED(COLOR_OK, 3, 100);
  }


#if USE_WIFI_FOR_LLM_COMMUNICATION
  if (USE_STATION_MODE) {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    unsigned long startTime = millis();
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
      blinkLED(COLOR_RUNNING, 3, 70);
      Serial.print(".");
      delay(500);
      if (millis() - startTime > 60000) {
        Serial.println("WiFi connection failed after 1 minute.");
        while (1) {
          blinkLED(COLOR_ERROR, 10, 1000);
        }
      }
    }
    blinkLED(COLOR_OK, 3, 100);
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_MODE_AP);
    blinkLED(COLOR_RUNNING, 2, 100);
    WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
    blinkLED(COLOR_OK, 3, 100);
    Serial.println("AP started");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP().toString());
  }
#endif
}

void loop() {
  M5.update();
}