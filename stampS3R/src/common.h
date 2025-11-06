#ifndef COMMON_H
#define COMMON_H

#include "config.h"
#include <FastLED.h>
#include <SPIFFS.h>

enum initCommunicationResult {
    INIT_COMMUNICATION_SUCCESS = 0,
    INIT_COMMUNICATION_FAILURE = 1        
};

enum SerialSendResult {
    SERIAL_SEND_SUCCESS = 0,
    SERIAL_SEND_FAILURE = 1
};

enum SerialReceiveResult {
    SERIAL_RECEIVE_SUCCESS = 0,
    SERIAL_RECEIVE_FAILURE = 1
};

// LED設定
#define LED_NUM 1
#define LED_PIN 21

// LED色定数
extern const CRGB COLOR_ACCESSED;
extern const CRGB COLOR_RUNNING;
extern const CRGB COLOR_ERROR;
extern const CRGB COLOR_OK;

// LED明るさ
#define LED_BRIGHTNESS 64

// LED配列
extern CRGB leds[LED_NUM];

// LED制御関数
void initLED();
void blinkLED(const CRGB& color, const uint8_t& times, const uint16_t& interval_ms, const bool hold = false);
void led_sayStart_initialize();
void led_sayNext_initialize();
void led_sayError_initialize();
void led_saySuccess_initialize();


enum initSPIFFSResult {
    INIT_SPIFFS_SUCCESS = 0,
    INIT_SPIFFS_FAILURE = 1
};

initSPIFFSResult initSPIFFS();

// 通信初期化関数
initCommunicationResult init_communication();

#endif // COMMON_H