
#include "WiFi.h"
#include <esp_wifi.h>

std::string WiFiClass::softAPmacAddress(void)
{
    uint8_t mac[6];
    char macStr[18] = { 0 };

    esp_wifi_get_mac(WIFI_IF_AP, mac);

    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(macStr);
}

WiFiClass WiFi;
