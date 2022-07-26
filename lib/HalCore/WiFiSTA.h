#ifndef ESP32WIFISTA_H_
#define ESP32WIFISTA_H_


#include "WiFiType.h"
#include "IPAddress.h"
#include <esp_event.h>

class WiFiSTAClass
{

public:
    WiFiSTAClass();

    void init(const char * hostname);

    wl_status_t begin(const char* ssid, const char *passphrase = NULL, bool connect = true);
    wl_status_t begin();

    bool reconnect();
    bool disconnect(bool wifioff = false, bool eraseap = false);

    bool isConnected();

    bool setAutoReconnect(bool autoReconnect);
    bool getAutoReconnect();

    bool hasWifiConfig();

    // STA network info
    IPAddress localIP();

    uint8_t * macAddress(uint8_t* mac);
    std::string macAddress();

    IPAddress subnetMask();
    IPAddress gatewayIP();
    IPAddress dnsIP(uint8_t dns_no = 0);

    IPAddress broadcastIP();
    IPAddress networkID();
    uint8_t subnetCIDR();
    
    bool enableIpV6();

    const char * getHostname();
    bool setHostname(const char * hostname);
    bool hostname(const std::string& aHostname) { return setHostname(aHostname.c_str()); }

    // STA WiFi info
    static wl_status_t status();
    std::string SSID() const;
    std::string psk() const;

    uint8_t * BSSID();
    std::string BSSIDstr();

    int8_t RSSI();

    static void _network_event_cb(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    
    static std::string _hostname;
protected:
    static bool _autoReconnect;
    static esp_netif_t * _netif;

private:
    void set_status(wl_status_t status);
    

    IPAddress calculateNetworkID(IPAddress ip, IPAddress subnet);
    IPAddress calculateBroadcast(IPAddress ip, IPAddress subnet);
    uint8_t calculateSubnetCIDR(IPAddress subnetMask);

};


#endif /* ESP32WIFISTA_H_ */
