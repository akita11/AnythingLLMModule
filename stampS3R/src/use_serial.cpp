#include "use_serial.h"
#include <M5Unified.h>

#if !USE_WIFI_FOR_LLM_COMMUNICATION

initCommunicationResult init_communication() {
    // Nothing to do
    led_sayNext_initialize();
    return INIT_COMMUNICATION_SUCCESS;
}

SerialSendResult send_data(const char* data) {
    Serial.print(data);
    return SERIAL_SEND_SUCCESS;
}

SerialReceiveResult receive_data() {
    if (Serial.available()) {
        return SERIAL_RECEIVE_SUCCESS;
    } else {
        return SERIAL_RECEIVE_FAILURE;
    }
}

sendToPCResult sendToPC(const String& sending_json) {
    Serial.println(sending_json);
    return SEND_TO_PC_SUCCESS;
}

sendToPCResult sendToPCwithResponse(const String& sending_json, const bool multiple_response = false) {
    Serial.println(sending_json);
    return SEND_TO_PC_SUCCESS;
}


#endif // !USE_WIFI_FOR_LLM_COMMUNICATION
