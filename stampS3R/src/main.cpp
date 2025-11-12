// true: WiFiでLLMサーバーと通信、false: シリアルでLLMサーバーと通信 (デフォルト: true)
#define USE_WIFI_FOR_LLM_COMMUNICATION true

// WiFiを使用するとき true: ステーションモード(既存のWiFiを使用)、false: APモード(これがAPになる) (デフォルト: true)
#define USE_STATION_MODE true

#include <M5Unified.h>
#include <ArduinoJson.h>
#include "common.h"

#if USE_WIFI_FOR_LLM_COMMUNICATION
#include "use_wifi.h"
#else
#include "use_serial.h"
#endif

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(9600);
  // Serial2.begin(115200, SERIAL_8N1, RX, TX) の順序
  Serial2.begin(115200, SERIAL_8N1, 7, 5);

  initLED();
  led_sayStart_initialize();
  // initSPIFFS();
  init_communication();
  led_saySuccess_initialize();

  Serial.println("[JSON] JSON reader initialized");
  resetJsonBuffer();
}

void loop()
{
  M5.update();

  // JSONドキュメントを動的に確保（必要に応じてサイズ調整）
  StaticJsonDocument<JSON_BUFFER_SIZE> doc;

  if (readJsonMessage(doc))
  {
    // JSONのパース成功
    Serial.println("[JSON] Parsed successfully:");

    Serial.println("[JSON] Sending handshake: Hello");
    Serial2.println("Hello");

    // JSONを整形して出力
    serializeJsonPretty(doc, Serial);
    Serial.println();

    // if (doc.containsKey("data"))
    // {
    //   Serial.println("[JSON] Data field found");
    // }

    //     static const char* _cmd_ping =
    //     "{\"request_id\":\"sys_ping\",\"work_id\":\"sys\",\"action\":\"ping\",\"object\":\"None\",\"data\":\"None\"}";
    // static const char* _cmd_reset =
    //     "{\"request_id\":\"sys_reset\",\"work_id\":\"sys\",\"action\":\"reset\",\"object\":\"None\",\"data\":\"None\"}";
    // static const char* _cmd_reboot =
    //     "{\"request_id\":\"sys_reboot\",\"work_id\":\"sys\",\"action\":\"reboot\",\"object\":\"None\",\"data\":\"None\"}";
    // static const char* _cmd_version =
    //     "{\"request_id\":\"sys_version\",\"work_id\":\"sys\",\"action\":\"version\",\"object\":\"None\",\"data\":\"None\"}";

    // [JSON] Parsed successfully:
    // {
    //   "request_id": "sys_version",
    //   "work_id": "sys",
    //   "action": "version",
    //   "object": "None",
    //   "data": "None"
    // }
    // [JSON] Data field found
    // [JSON] Parsed successfully:
    // {
    //   "request_id": "sys_ping",
    //   "work_id": "sys",
    //   "action": "ping",
    //   "object": "None",
    //   "data": "None"
    // }

    // {
    //   "request_id": "sys_ping",  // 送信したリクエストIDと一致すること
    //   "work_id": "sys",
    //   "object": "None",
    //   "error": {
    //     "code": 0,              // これが0ならOK、負数ならエラー
    //     "message": ""
    //   }
    // }

    // 返事用のJSONの元の構造体 をひとつ作る
    ResponseMsg_t response_msg = {};

    if (doc.containsKey("work_id") && doc.containsKey("action"))
    {
      if (doc["work_id"] == "sys")
      {
        response_msg.request_id = doc["request_id"].as<String>();
        response_msg.work_id = "sys";
        if (doc["action"] == "ping")
        {
          Serial.println("[JSON] System ping");
          response_msg.object = "None";
          response_msg.error.code = 0;
          response_msg.error.message = "";
          sendToM5(response_msg);
        }
        else if (doc["action"] == "reset")
        {
          Serial.println("[JSON] System reset");
          response_msg.object = "None";
          response_msg.error.code = 0;
          response_msg.error.message = "";
          response_msg.request_id = "sys_reset";
          response_msg.work_id = "sys";
          sendToM5(response_msg);
          delay(15);
          response_msg.request_id = "0";
          response_msg.work_id = "sys";
          response_msg.object = "None";
          response_msg.error.code = 0;
          response_msg.error.message = "";
          sendToM5(response_msg);
        }
        else if (doc["action"] == "reboot")
        {
          Serial.println("[JSON] System reboot");
        }
        else if (doc["action"] == "version")
        {
          Serial.println("[JSON] System version");
        }
      }
      else if (doc["work_id"] == "llm")
      {
        response_msg.request_id = doc["request_id"].as<String>();
        if (doc["action"] == "setup")
        {
          Serial.println("[JSON] LLM setup");
          // dataフィールドからモデル名を取得
          String model_name = "";
          if (doc["data"].is<JsonObject>())
          {
            JsonObject data_obj = doc["data"];
            if (data_obj["model"].is<String>())
            {
              model_name = data_obj["model"].as<String>();
            }
          }
          else if (doc["data"].is<String>())
          {
            model_name = doc["data"].as<String>();
          }

          if (model_name.length() == 0)
          {
            Serial.println("[JSON] Model name not specified in data field");
            response_msg.error.code = 1;
            response_msg.error.message = "Model name not specified";
            sendToM5(response_msg);
          }
          else
          {
            LLM_Status llm_status = llm_setup(model_name);
            Serial.print("[JSON] LLM setup status: ");
            Serial.println(llm_status);
            if (llm_status != LLM_OLLAMA_OK)
            {
              response_msg.error.code = (llm_status == LLM_OLLAMA_NOT_FOUND) ? 2 : 1;
              response_msg.error.message = (llm_status == LLM_OLLAMA_NOT_FOUND) ? "Model not found" : "LLM setup failed";
              sendToM5(response_msg);
            }
            else
            {
              // work_idを生成（例: "llm_12345"）
              String generated_work_id = "llm_" + String(millis() % 100000);
              response_msg.work_id = generated_work_id;
              current_work_id = generated_work_id;
              response_msg.object = "llm.setup";
              response_msg.error.code = 0;
              response_msg.error.message = "";
              response_msg.request_id = "llm_setup";
              Serial.print("[JSON] LLM setup response: ");
              Serial.print("request_id=");
              Serial.print(response_msg.request_id);
              Serial.print(", work_id=");
              Serial.print(response_msg.work_id);
              Serial.print(", object=");
              Serial.print(response_msg.object);
              Serial.print(", error.code=");
              Serial.print(response_msg.error.code);
              Serial.println();

              sendToM5(response_msg);

              // PCに送る（適当に受信したJSONをそのまま送る）
              String json_str;
              serializeJson(doc, json_str);
              sendToPC(json_str);
            }
          }
        }
        else if (doc["action"] == "inference")
        {
 
        }
      }
      else
      {
        if (doc["action"] == "inference" && doc["request_id"] == "llm_inference")
        {
          Serial.println("[JSON] LLM inference");
          if (doc["object"] == "llm.utf-8.stream")
          {
            Serial.println("[JSON] Using streaming inference");
            OllamaInferenceCommand command;
            command.model = using_model_name;
            command.prompt = doc["data"]["delta"].as<String>();

            LLM_Status llm_status = llm_inference_streaming(command);
            Serial.print("[JSON] LLM inference status: ");
            Serial.println(llm_status);
            if (llm_status != LLM_OLLAMA_OK)
            {
              response_msg.error.code = 1;
              response_msg.error.message = "LLM inference failed";
              sendToM5(response_msg);
            }
          }
          else
          {
            Serial.println("[JSON] Using non-streaming inference");
          }
        }
      }
    }
    // LEDで成功を表示
    blinkLED(COLOR_OK, 1, 50);
  }

  delay(10);
}
