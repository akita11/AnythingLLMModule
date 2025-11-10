#ifndef USE_SERIAL_H
#define USE_SERIAL_H TRUE
#include "common.h"

initCommunicationResult init_communication();
SerialSendResult send_data(const char* data);
SerialReceiveResult receive_data();

sendToPCResult sendToPC(const String& sending_json);
sendToPCResult sendToPCwithResponse(const String& sending_json, const bool multiple_response = false);


#endif
