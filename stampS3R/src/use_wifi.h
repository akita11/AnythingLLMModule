#ifndef USE_WIFI_H
#define USE_WIFI_H TRUE
#include "common.h"

initCommunicationResult init_communication();
SerialSendResult send_data(const char* data);
SerialReceiveResult receive_data();

sendToPCResult sendToPC(const String& sending_json);
sendToPCResult sendToPCwithResponse(const String& sending_json, const bool multiple_response = false);

LLM_Status llm_setup(const String& model_name);
LLM_Status llm_inference_no_streaming(const OllamaInferenceCommand& command);
LLM_Status llm_inference_streaming(const OllamaInferenceCommand& command);


#endif