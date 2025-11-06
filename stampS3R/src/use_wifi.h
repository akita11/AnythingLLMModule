#ifndef USE_WIFI_H
#define USE_WIFI_H TRUE
#include "common.h"

initCommunicationResult init_communication();
SerialSendResult send_data(const char* data);
SerialReceiveResult receive_data();

#endif