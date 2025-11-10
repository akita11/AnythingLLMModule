#include "use_wifi.h"
#include <M5Unified.h>

#if USE_WIFI_FOR_LLM_COMMUNICATION

#include "secrets.h"
String wifi_ssid = WIFI_SSID;
String wifi_password = WIFI_PASSWORD;
String ap_ssid = AP_SSID;
String ap_password = AP_PASSWORD;
String host_ollama_url = String("http://") + String(HOST_IP) + ":" + String(HOST_OLLAMA_PORT);
#include "WiFi.h"
#include "HTTPClient.h"
#include <ArduinoJson.h>


initCommunicationResult init_communication() {
    led_sayNext_initialize();
    if (USE_STATION_MODE) {
        WiFi.mode(WIFI_MODE_STA);
        WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
        unsigned long startTime = millis();
        Serial.print("Connecting to WiFi...");
        while (WiFi.status() != WL_CONNECTED) {
        blinkLED(COLOR_RUNNING, 1, 300);
        Serial.print(".");
        delay(700);
        if (millis() - startTime > 60000) {
            while (1) {
                Serial.println("WiFi connection failed after 1 minute.");
                led_sayError_initialize();
            }
        }
        }
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        WiFi.mode(WIFI_MODE_AP);
        bool ap_success = WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
        if (!ap_success) {
            while (1) {
                Serial.println("AP start failed.");
                led_sayError_initialize();
            }
        }
        Serial.println("AP started");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP().toString());
    }
    return INIT_COMMUNICATION_SUCCESS;
}


sendToPCResult sendToPC(const String& sending_json) {
    HTTPClient http;
    http.begin(host_ollama_url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    http.setConnectTimeout(3000);
    int httpCode = http.POST(sending_json);
    http.end();
    return httpCode == 200 ? SEND_TO_PC_SUCCESS : SEND_TO_PC_FAILURE;
}

LLM_Status llm_setup(const String& model_name) {
    // /api/version にGETしてみる
    HTTPClient http;
    http.begin(host_ollama_url + "/api/version");
    http.setTimeout(5000);
    http.setConnectTimeout(3000);
    int httpCode = http.GET();

    Serial.print("[JSON] LLM setup version status: ");
    Serial.println(httpCode);

    http.end();
    // /api/tags をGET
    http.begin(host_ollama_url + "/api/tags");
    http.setTimeout(5000);
    http.setConnectTimeout(3000);
    httpCode = http.GET();
    Serial.print("[JSON] LLM setup list status: ");
    Serial.println(httpCode);
    
    if (httpCode != 200) {
        http.end();
        return LLM_OLLAMA_NOT_OK;
    }
    
    // responseを読み取る
    String response = http.getString();
    // Serial.print("[JSON] LLM setup list response: ");
    // Serial.println(response);

    http.end();
    
    // JSONをパースしてモデル名をチェック
    if (model_name.length() == 0) {
        Serial.println("[JSON] Model name is empty");
        return LLM_OLLAMA_NOT_OK;
    }
    
    // ArduinoJsonでパース
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.print("[JSON] Parse error: ");
        Serial.println(error.c_str());
        // 確保したメモリを解放
        doc.clear();
        http.end();
        return LLM_OLLAMA_NOT_OK;
    }
    
    // models配列をチェック
    if (!doc.containsKey("models") || !doc["models"].is<JsonArray>()) {
        Serial.println("[JSON] No models array found");
        return LLM_OLLAMA_NOT_OK;
    }
    
    JsonArray models = doc["models"];
    bool model_found = false;
    
    for (JsonObject model : models) {
        // name または model フィールドをチェック
        if (model.containsKey("name") && model["name"].as<String>() == model_name) {
            model_found = true;
            Serial.print("[JSON] Model found: ");
            Serial.println(model_name);
            break;
        }
        if (model.containsKey("model") && model["model"].as<String>() == model_name) {
            model_found = true;
            Serial.print("[JSON] Model found: ");
            Serial.println(model_name);
            break;
        }
    }
    
    if (model_found) {
        Serial.print("[JSON] Model found: ");
        Serial.println(model_name);
        doc.clear();
        http.end();
        using_model_name = model_name;
        return LLM_OLLAMA_OK;
    } else {
        Serial.print("[JSON] Model not found: ");
        Serial.println(model_name);
        doc.clear();
        http.end();
        return LLM_OLLAMA_NOT_FOUND;
    }
}

LLM_Status llm_inference_no_streaming(const OllamaInferenceCommand& command) {
    return LLM_OLLAMA_OK;
}

LLM_Status llm_inference_streaming(const OllamaInferenceCommand& command) {
    HTTPClient http;
    String url = host_ollama_url + "/api/generate";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);  // 10秒タイムアウト
    http.setConnectTimeout(5000);  // 5秒接続タイムアウト
    
    // リクエストJSONを作成
    StaticJsonDocument<512> requestDoc;
    requestDoc["model"] = command.model;
    requestDoc["prompt"] = command.prompt;
    requestDoc["stream"] = true;
    // curl http://localhost:11434/api/generate -d '{
    //     "model": "gemma3",
    //     "prompt": "Why is the sky blue?"
    //   }'
    
    String requestJson;
    serializeJson(requestDoc, requestJson);
    
    Serial.print("[JSON] LLM inference streaming request: ");
    Serial.println(requestJson);
    Serial.print("[JSON] Request URL: ");
    Serial.println(url);
    
    int httpCode = http.POST(requestJson);
    
    if (httpCode != 200) {
        Serial.print("[JSON] LLM inference streaming HTTP error: ");
        Serial.println(httpCode);
        Serial.print("[JSON] Error string: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        return LLM_OLLAMA_NOT_OK;
    }
    
    // ストリーミングレスポンスを読み取る
    WiFiClient* stream = http.getStreamPtr();
    if (!stream) {
        Serial.println("[JSON] Failed to get stream");
        http.end();
        return LLM_OLLAMA_NOT_OK;
    }
    
    // 改行区切りのJSONを読み取るバッファ
    String lineBuffer = "";
    lineBuffer.reserve(2048);  // メモリを事前確保
    bool done = false;
    unsigned long lastDataTime = millis();  // 最後にデータを受信した時刻
    const unsigned long IDLE_TIMEOUT = 30000;  // 30秒アイドルタイムアウト（データが来ない時間）
    const size_t MAX_LINE_BUFFER = 4096;  // 最大4KB
    
    while (!done) {
        // アイドルタイムアウトチェック
        if (millis() - lastDataTime > IDLE_TIMEOUT) {
            Serial.println("[JSON] Stream idle timeout (no data for 30s)");
            http.end();
            return LLM_OLLAMA_NOT_OK;
        }
        
        // ストリームからデータを読み取る
        bool dataReceived = false;
        while (stream->available()) {
            dataReceived = true;
            char c = stream->read();
            
            if (c == '\n' || c == '\r') {
                // 改行が来たらJSONをパース
                if (lineBuffer.length() > 0) {
                    // JSONドキュメントのサイズを動的に決定（行バッファの2倍程度）
                    size_t docSize = lineBuffer.length() * 2;
                    if (docSize < 2048) docSize = 2048;
                    if (docSize > 8192) docSize = 8192;  // 最大8KB
                    
                    DynamicJsonDocument responseDoc(docSize);
                    DeserializationError error = deserializeJson(responseDoc, lineBuffer);
                    
                    if (!error) {
                        // responseフィールドを取得してUARTに送信
                        if (responseDoc["response"].is<String>()) {
                            String response_text = responseDoc["response"].as<String>();
                            Serial.print("[JSON] Stream chunk: ");
                            Serial.println(response_text);
                            ResponseMsg_t_error error_val;
                            error_val.code = 0;
                            error_val.message = "";
                            ResponseMsg_t_inference_data inference_data_val;
                            inference_data_val.delta = response_text;
                            inference_data_val.index = 0;
                            inference_data_val.finish = false;
                            ResponseMsg_t response_msg;
                            response_msg.request_id = "llm_inference";
                            String work_id = current_work_id.length() > 0 ? current_work_id : ("llm_" + String(millis() % 100000));
                            response_msg.work_id = work_id;
                            if (current_work_id.length() == 0) {
                                current_work_id = work_id;
                            }
                            response_msg.object = "llm.utf-8.stream";
                            response_msg.error = error_val;
                            response_msg.inference_data = inference_data_val;
                            sendToM5(response_msg);

                        }
                        
                        // doneフィールドをチェック
                        if (responseDoc["done"].is<bool>() && responseDoc["done"].as<bool>()) {
                            done = true;
                            Serial.println("[JSON] Stream done");
                            // struct ResponseMsg_t_inference_data {
                            //     String delta;
                            //     uint16_t index;
                            //     bool finish;
                            // };
                            
                            // struct ResponseMsg_t {
                            //     String request_id;
                            //     String work_id;
                            //     String object;
                            //     ResponseMsg_t_error error;
                            //     ResponseMsg_t_inference_data inference_data;
                            //   };
                            ResponseMsg_t_error error_val;
                            error_val.code = 0;
                            error_val.message = "";
                            ResponseMsg_t_inference_data inference_data_val;
                            inference_data_val.delta = "";
                            inference_data_val.index = 0;
                            inference_data_val.finish = true;
                            ResponseMsg_t response_msg;
                            response_msg.request_id = "llm_inference";
                            String work_id = current_work_id.length() > 0 ? current_work_id : ("llm_" + String(millis() % 100000));
                            response_msg.work_id = work_id;
                            if (current_work_id.length() == 0) {
                                current_work_id = work_id;
                            }
                            response_msg.object = "llm.utf-8.stream";
                            response_msg.error = error_val;
                            response_msg.inference_data = inference_data_val;
                            sendToM5(response_msg);
                            break;
                        }
                    } else {
                        Serial.print("[JSON] Parse error: ");
                        Serial.println(error.c_str());
                        Serial.print("[JSON] Line buffer (first 200 chars): ");
                        String preview = lineBuffer.substring(0, 200);
                        Serial.println(preview);
                    }
                    
                    lineBuffer = "";
                    lineBuffer.reserve(2048);  // リセット後もメモリ確保
                }
            } else {
                lineBuffer += c;
                
                // バッファオーバーフロー対策：最初の方を削除して続行
                if (lineBuffer.length() > MAX_LINE_BUFFER) {
                    Serial.println("[JSON] Line buffer overflow, truncating...");
                    // 後半の2048バイトを残す（最新のデータを優先）
                    String truncated = lineBuffer.substring(lineBuffer.length() - 2048);
                    lineBuffer = truncated;
                }
            }
        }
        
        // データを受信した場合はタイムアウト時刻を更新
        if (dataReceived) {
            lastDataTime = millis();
        }
        
        // 少し待機（CPU負荷軽減）
        delay(10);
    }
    
    http.end();
    return LLM_OLLAMA_OK;
}


#endif // USE_WIFI_FOR_LLM_COMMUNICATION
