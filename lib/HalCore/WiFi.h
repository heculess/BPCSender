#ifndef WiFi_h
#define WiFi_h

#include <stdint.h>

#include "WiFiType.h"
#include "WiFiSTA.h"

class WiFiClass : public WiFiSTAClass
{
public:
    WiFiClass()
    {
    }

    using WiFiSTAClass::SSID;
    using WiFiSTAClass::RSSI;
    using WiFiSTAClass::BSSID;
    using WiFiSTAClass::BSSIDstr;

    std::string softAPmacAddress(void);

public:  
    friend class WiFiClient;
};

extern WiFiClass WiFi;

#endif
