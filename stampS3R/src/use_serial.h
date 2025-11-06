#ifndef USE_SERIAL_H
#define USE_SERIAL_H TRUE
#include "common.h"

initCommunicationResult init_communication();
SerialSendResult send_data(const char* data);
SerialReceiveResult receive_data();

#endif
