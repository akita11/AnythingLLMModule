#ifndef COMMON_H
#define COMMON_H

#include "config.h"
#include <ArduinoJson.h>
#include <FastLED.h>
#include <SPIFFS.h>

enum initCommunicationResult
{
    INIT_COMMUNICATION_SUCCESS = 0,
    INIT_COMMUNICATION_FAILURE = 1
};

enum SerialSendResult
{
    SERIAL_SEND_SUCCESS = 0,
    SERIAL_SEND_FAILURE = 1
};

enum SerialReceiveResult
{
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
void blinkLED(const CRGB &color, const uint8_t &times, const uint16_t &interval_ms, const bool hold = false);
void led_sayStart_initialize();
void led_sayNext_initialize();
void led_sayError_initialize();
void led_saySuccess_initialize();

enum initSPIFFSResult
{
    INIT_SPIFFS_SUCCESS = 0,
    INIT_SPIFFS_FAILURE = 1
};

initSPIFFSResult initSPIFFS();

// 通信初期化関数
initCommunicationResult init_communication();

// 返答用のJSONの元になる構造体
struct ResponseMsg_t_error
{
    uint16_t code;
    String message;
};

struct ResponseMsg_t_inference_data
{
    String delta;
    uint16_t index;
    bool finish;
};

struct ResponseMsg_t
{
    String request_id;
    String work_id;
    String object;
    ResponseMsg_t_error error;
    ResponseMsg_t_inference_data inference_data;
};

void sendToM5(const ResponseMsg_t &response_msg);

enum sendToPCResult
{
    SEND_TO_PC_SUCCESS = 0,
    SEND_TO_PC_FAILURE = 1
};

sendToPCResult sendToPC(const String &sending_json);

enum LLM_Status
{
    LLM_OLLAMA_OK = 0,
    LLM_OLLAMA_NOT_OK = 1,
    LLM_OLLAMA_NOT_FOUND = 2
};

extern String using_model_name;
extern String current_work_id;

struct OllamaInferenceCommand
{
    String model;
    String prompt;
};

constexpr size_t JSON_BUFFER_SIZE = 2048;
constexpr unsigned long JSON_TIMEOUT_MS = 1000;
constexpr unsigned long PARSE_ERROR_WAIT_MS = 50;

void resetJsonBuffer();
bool readJsonMessage(JsonDocument &doc);

#endif // COMMON_H