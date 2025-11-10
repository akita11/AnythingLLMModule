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

// JSON読み取り用の設定
#define JSON_BUFFER_SIZE 2048
#define JSON_TIMEOUT_MS 1000   // 1秒でタイムアウト
#define PARSE_ERROR_WAIT_MS 50 // パースエラー後の待機時間（続きが来るか待つ）

// JSON読み取り用のバッファ
char jsonBuffer[JSON_BUFFER_SIZE];
size_t jsonBufferIndex = 0;
unsigned long lastCharTime = 0;
static int openBraces = 0;               // 括弧のバランスを追跡
static bool inString = false;            // 文字列内かどうか
static bool escaped = false;             // エスケープ文字の次の文字かどうか
static unsigned long parseErrorTime = 0; // パースエラーが発生した時刻（0ならエラーなし）

// JSONバッファをリセット
void resetJsonBuffer()
{
  jsonBufferIndex = 0;
  jsonBuffer[0] = '\0';
  lastCharTime = 0;
  openBraces = 0;
  inString = false;
  escaped = false;
  parseErrorTime = 0;
}

// 完全なJSONメッセージを読み取る（括弧のバランスで終端判定）
bool readJsonMessage(JsonDocument &doc)
{
  // タイムアウトチェック
  if (jsonBufferIndex > 0 && millis() - lastCharTime > JSON_TIMEOUT_MS)
  {
    Serial.println("[JSON] Timeout, resetting buffer");
    resetJsonBuffer();
  }

  // パースエラー後の待機チェック（50ms待って続きが来なかったら破棄）
  if (parseErrorTime > 0)
  {
    if (millis() - parseErrorTime > PARSE_ERROR_WAIT_MS)
    {
      // 続きが来なかったので破棄
      Serial.println("[JSON] Parse error timeout, resetting buffer");
      resetJsonBuffer();
    }
    else
    {
      // まだ待機中、続きが来る可能性がある
      // 新しいデータが来たらエラー時刻をリセット（続きが来たと判断）
      if (Serial2.available())
      {
        parseErrorTime = 0; // 続きが来たのでエラー時刻をリセット
      }
    }
  }

  // シリアルからデータを読み取る
  while (Serial2.available())
  {
    char c = (char)Serial2.read();
    lastCharTime = millis();

    // 新しいデータが来たらパースエラー時刻をリセット（続きが来たと判断）
    if (parseErrorTime > 0)
    {
      parseErrorTime = 0;
    }

    // 改行が来たらリセット（改行で区切られている想定）
    if (c == '\n' || c == '\r')
    {
      resetJsonBuffer();
      continue;
    }

    // バッファオーバーフロー対策
    if (jsonBufferIndex >= JSON_BUFFER_SIZE - 1)
    {
      Serial.println("[JSON] Buffer overflow, resetting");
      resetJsonBuffer();
      continue;
    }

    // エスケープ文字の処理
    if (escaped)
    {
      escaped = false;
      jsonBuffer[jsonBufferIndex++] = c;
      continue;
    }

    if (c == '\\')
    {
      escaped = true;
      jsonBuffer[jsonBufferIndex++] = c;
      continue;
    }

    // 文字列の開始/終了を判定
    if (c == '"')
    {
      inString = !inString;
      jsonBuffer[jsonBufferIndex++] = c;
      continue;
    }

    // 文字をバッファに追加
    jsonBuffer[jsonBufferIndex++] = c;

    // 文字列外でのみ括弧のバランスをチェック
    if (!inString)
    {
      if (c == '{')
      {
        openBraces++;
      }
      else if (c == '}')
      {
        openBraces--;

        // 括弧のバランスが取れたら終端と判定
        if (openBraces == 0)
        {
          jsonBuffer[jsonBufferIndex] = '\0';

          // 先頭・末尾の空白を削除
          char *start = jsonBuffer;
          while (*start == ' ' || *start == '\t')
            start++;
          char *end = jsonBuffer + jsonBufferIndex - 1;
          while (end > start && (*end == ' ' || *end == '\t' || *end == '\0'))
            end--;
          *(end + 1) = '\0';

          if (strlen(start) > 0)
          {
            DeserializationError error = deserializeJson(doc, start);

            if (error)
            {
              // パースエラー：50ms待って続きが来るか確認
              Serial.print("[JSON] Parse error: ");
              Serial.println(error.c_str());
              Serial.print("[JSON] Received: ");
              Serial.println(start);
              parseErrorTime = millis(); // エラー時刻を記録
              return false;              // バッファは破棄せず、続きを待つ
            }

            // パース成功：即座にバッファを破棄
            resetJsonBuffer();
            return true;
          }

          resetJsonBuffer();
        }
        else if (openBraces < 0)
        {
          // 括弧のバランスが崩れたらリセット
          resetJsonBuffer();
        }
      }
    }
  }

  return false;
}

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(9600);
  // Serial2.begin(115200, SERIAL_8N1, RX, TX) の順序
  // 受信できているのでRX=5は正しい。TXピンを確認する必要がある
  Serial2.begin(115200, SERIAL_8N1, 5, 7);

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

    Serial2.print("Hello");

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
    ResponseMsg_t response_msg;

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

              // sendToM5(response_msg);
              // JSON文字列を生成（sendToM5と同じロジック）
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
              Serial.println(response_json);
              Serial2.print(response_json);

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
    M5.Lcd.clear();
    delay(50);
    M5.Lcd.fillScreen(YELLOW);
  }

  delay(10);
}
