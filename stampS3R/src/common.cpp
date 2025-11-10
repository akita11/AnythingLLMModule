#include "common.h"
#include <M5Unified.h>

const CRGB COLOR_ACCESSED = CRGB(0, 255, 255); // シアン
const CRGB COLOR_RUNNING = CRGB(255, 255, 128); // 黄色っぽい白
const CRGB COLOR_ERROR = CRGB(255, 0, 0); // 赤
const CRGB COLOR_OK = CRGB(0, 255, 0); // 緑

CRGB leds[LED_NUM];

String using_model_name = "";
String current_work_id = "";

void blinkLED(const CRGB& color, const uint8_t& times, const uint16_t& interval_ms, const bool hold) {
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

void initLED() {
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_NUM);
    FastLED.setBrightness(LED_BRIGHTNESS);
}

void led_sayStart_initialize() {
    blinkLED(COLOR_RUNNING, 3, 100);
}

void led_sayNext_initialize() {
    blinkLED(COLOR_RUNNING, 2, 100);
}

void led_sayError_initialize() {
    blinkLED(COLOR_ERROR, 5, 1000);
}

void led_saySuccess_initialize() {
    blinkLED(COLOR_OK, 3, 100);
}


initSPIFFSResult initSPIFFS() {
    led_sayNext_initialize();
    // SPIFFSのマウント失敗 -> 初回起動 -> フォーマット、2回目以降スルー
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed. Estimated it's first boot. Formatting...");
    led_sayNext_initialize();
    if (SPIFFS.format()) {
      Serial.println("SPIFFS formatted successfully.");
      if (!SPIFFS.begin(false)) {
        while (1) {
          Serial.println("SPIFFS mount failed after format.");
          led_sayError_initialize();
        }
      }
      led_sayNext_initialize();
      Serial.println("SPIFFS mounted successfully.");
    } else {
      while (1) {
        Serial.println("SPIFFS format failed for unknown reason.");
        led_sayError_initialize();
      }
    }
  }
  else {
    Serial.println("SPIFFS mounted successfully.");
  }
}

void sendToM5(const ResponseMsg_t& response_msg) {
    String response_json = "{\"request_id\":\"" + response_msg.request_id + "\",\"work_id\":\"" + response_msg.work_id + "\",\"object\":\"" + response_msg.object + "\"";
    
    // inference_dataが空でない場合はdataフィールドを追加
    if (response_msg.inference_data.delta.length() > 0 || response_msg.inference_data.finish) {
        // deltaフィールドのエスケープ処理（JSONの特殊文字をエスケープ）
        String delta_escaped = response_msg.inference_data.delta;
        delta_escaped.replace("\\", "\\\\");
        delta_escaped.replace("\"", "\\\"");
        delta_escaped.replace("\n", "\\n");
        delta_escaped.replace("\r", "\\r");
        delta_escaped.replace("\t", "\\t");
        
        response_json += ",\"data\":{\"delta\":\"" + delta_escaped + "\",\"index\":" + String(response_msg.inference_data.index) + ",\"finish\":" + (response_msg.inference_data.finish ? "true" : "false") + "}";
    }
    
    response_json += ",\"error\":{\"code\":" + String(response_msg.error.code) + ",\"message\":\"" + response_msg.error.message + "\"}}";
    Serial2.println(response_json);
    Serial.print("[JSON] Sent to M5: ");
    Serial.println(response_json);
}
   