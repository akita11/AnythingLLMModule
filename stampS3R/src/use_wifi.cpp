#include "use_wifi.h"
#include <M5Unified.h>

#if USE_WIFI_FOR_LLM_COMMUNICATION

#include "secrets.h"
String wifi_ssid = WIFI_SSID;
String wifi_password = WIFI_PASSWORD;
String ap_ssid = AP_SSID;
String ap_password = AP_PASSWORD;
#include "WiFi.h"
#include "HTTPClient.h"


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

#endif // USE_WIFI_FOR_LLM_COMMUNICATION
