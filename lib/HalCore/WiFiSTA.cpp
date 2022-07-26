#include "WiFi.h"
#include "WiFiSTA.h"

extern "C" {
#include <string.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <lwip/ip_addr.h>
#include "lwip/err.h"
#include "lwip/dns.h"
#include <esp_netif.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
}

static const char *TAG = "WiFiSTAClass";

void WiFiSTAClass::_network_event_cb(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    
    WiFiSTAClass * _sta_class = (WiFiSTAClass *)arg;
    if( event_base == WIFI_EVENT){
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                _sta_class->set_status(WL_DISCONNECTED);
                break;
            case WIFI_EVENT_STA_STOP:
                _sta_class->set_status(WL_NO_SHIELD);
                break;
            case WIFI_EVENT_STA_CONNECTED:
                _sta_class->set_status(WL_IDLE_STATUS);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t*) event_data;
                    uint8_t reason = disconnected->reason;
                    if(reason == WIFI_REASON_NO_AP_FOUND) {
                        _sta_class->set_status(WL_NO_SSID_AVAIL);
                    } else if(reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_ASSOC_FAIL) {
                        _sta_class->set_status(WL_CONNECT_FAILED);
                    } else if(reason == WIFI_REASON_BEACON_TIMEOUT || reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
                        _sta_class->set_status(WL_CONNECTION_LOST);
                    } else if(reason == WIFI_REASON_AUTH_EXPIRE) {

                    } else {
                        _sta_class->set_status(WL_DISCONNECTED);
                    }

                    if(((reason == WIFI_REASON_AUTH_EXPIRE) ||
                        (reason >= WIFI_REASON_BEACON_TIMEOUT && reason != WIFI_REASON_AUTH_FAIL)) &&
                        _sta_class->getAutoReconnect())
                    {
                        _sta_class->disconnect();
                        _sta_class->begin();
                    }
                }
                break;
        }
    }
    if(event_base == IP_EVENT){
        switch (event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                _sta_class->set_status(WL_CONNECTED);
                break;
            case IP_EVENT_STA_LOST_IP:
                _sta_class->set_status(WL_IDLE_STATUS);
                break;
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- STA function -----------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

bool WiFiSTAClass::_autoReconnect = true;
esp_netif_t * WiFiSTAClass::_netif = NULL;
std::string WiFiSTAClass::_hostname = "esp32-wifi";

static wl_status_t _sta_status = WL_NO_SHIELD;

WiFiSTAClass::WiFiSTAClass()
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_network_event_cb, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &_network_event_cb, this));
}

void WiFiSTAClass::set_status(wl_status_t status)
{
    ESP_LOGI(TAG, "set_status : %d \r\n", status);
    _sta_status = status;
    esp_netif_set_hostname(_netif, _hostname.c_str());
}

wl_status_t WiFiSTAClass::status()
{
    return _sta_status;
}

wl_status_t WiFiSTAClass::begin(const char* ssid, const char *passphrase, bool connect)
{
    if(!ssid || *ssid == 0x00 || strlen(ssid) > 32) {
        return WL_CONNECT_FAILED;
    }

    if(passphrase && strlen(passphrase) > 64) {
        return WL_CONNECT_FAILED;
    }

    wifi_config_t conf;
    memset(&conf, 0, sizeof(wifi_config_t));
    strcpy(reinterpret_cast<char*>(conf.sta.ssid), ssid);

    if(passphrase) {
        if (strlen(passphrase) == 64){ // it's not a passphrase, is the PSK
            memcpy(reinterpret_cast<char*>(conf.sta.password), passphrase, 64);
        } else {
            strcpy(reinterpret_cast<char*>(conf.sta.password), passphrase);
        }
    }

    if(status() == WL_CONNECTED){
        return WL_CONNECTED;
    } else {
        esp_wifi_set_config(WIFI_IF_STA, &conf);
    }

    if(connect && esp_wifi_connect()) {
        return WL_CONNECT_FAILED;
    }

    return status();
}

bool WiFiSTAClass::hasWifiConfig()
{  
    wifi_config_t current_conf;
    if(esp_wifi_get_config(WIFI_IF_STA, &current_conf) != ESP_OK)  {
        return false;
    }
    ESP_LOGI(TAG, "WiFi ssid : %s password : %s\r\n", reinterpret_cast<char *>(current_conf.sta.ssid), 
        reinterpret_cast<char *>(current_conf.sta.password));
    if(strlen(reinterpret_cast<char *>(current_conf.sta.ssid)) == 0)
        return false;

    if(esp_wifi_set_config(WIFI_IF_STA, &current_conf) != ESP_OK){
        return false;
    }

    return true;
}

wl_status_t WiFiSTAClass::begin()
{
    wifi_config_t current_conf;
    if(esp_wifi_get_config(WIFI_IF_STA, &current_conf) != ESP_OK || esp_wifi_set_config(WIFI_IF_STA, &current_conf) != ESP_OK) {
        return WL_CONNECT_FAILED;
    }

    return begin((const char*)current_conf.sta.ssid,(const char*)current_conf.sta.password,true);
}

bool WiFiSTAClass::reconnect()
{
    if(esp_wifi_disconnect() == ESP_OK) {
        return esp_wifi_connect() == ESP_OK;
    }
    return false;
}

bool WiFiSTAClass::disconnect(bool wifioff, bool eraseap)
{
    wifi_config_t conf;

    if(eraseap){
        memset(&conf, 0, sizeof(wifi_config_t));
        esp_wifi_set_config(WIFI_IF_STA, &conf);
    }
    if(esp_wifi_disconnect()){
        return false;
    }
    return true;
}

bool WiFiSTAClass::isConnected()
{
    return (status() == WL_CONNECTED);
}


bool WiFiSTAClass::setAutoReconnect(bool autoReconnect)
{
    _autoReconnect = autoReconnect;
    return true;
}

bool WiFiSTAClass::getAutoReconnect()
{
    return _autoReconnect;
}

void WiFiSTAClass::init(const char * hostname)
{
    if(_netif)
        return;

    if(hostname)
        _hostname = hostname;

    ESP_ERROR_CHECK(esp_netif_init());
    _netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    wifi_mode_t  wifi_mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));
    if(WIFI_MODE_STA != wifi_mode)
        esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wifi_stop();
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_netif_set_hostname(_netif, _hostname.c_str());
}

IPAddress WiFiSTAClass::localIP()
{
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    return IPAddress(ip.ip.addr);
}

uint8_t* WiFiSTAClass::macAddress(uint8_t* mac)
{
    esp_wifi_get_mac(WIFI_IF_STA, mac);	
    return mac;
}

std::string WiFiSTAClass::macAddress(void)
{
    uint8_t mac[6];
    char macStr[18] = { 0 };
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(macStr);
}

IPAddress WiFiSTAClass::subnetMask()
{
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    return IPAddress(ip.netmask.addr);
}

IPAddress WiFiSTAClass::gatewayIP()
{
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    return IPAddress(ip.gw.addr);
}

IPAddress WiFiSTAClass::dnsIP(uint8_t dns_no)
{
    const ip_addr_t* dns_ip = dns_getserver(dns_no);
    return IPAddress(dns_ip->u_addr.ip4.addr);
}

IPAddress WiFiSTAClass::broadcastIP()
{
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    
    return calculateBroadcast(IPAddress(ip.gw.addr), IPAddress(ip.netmask.addr));
}

IPAddress WiFiSTAClass::networkID()
{
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    return calculateNetworkID(IPAddress(ip.gw.addr), IPAddress(ip.netmask.addr));
}

uint8_t WiFiSTAClass::subnetCIDR()
{
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    return calculateSubnetCIDR(IPAddress(ip.netmask.addr));
}

std::string WiFiSTAClass::SSID() const
{
    wifi_ap_record_t info;
    if(!esp_wifi_sta_get_ap_info(&info)) {
        return std::string(reinterpret_cast<char*>(info.ssid));
    }
    return std::string();
}

std::string WiFiSTAClass::psk() const
{
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    return std::string(reinterpret_cast<char*>(conf.sta.password));
}

uint8_t* WiFiSTAClass::BSSID(void)
{
    static uint8_t bssid[6];
    wifi_ap_record_t info;
    if(!esp_wifi_sta_get_ap_info(&info)) {
        memcpy(bssid, info.bssid, 6);
        return reinterpret_cast<uint8_t*>(bssid);
    }
    return NULL;
}

std::string WiFiSTAClass::BSSIDstr(void)
{
    uint8_t* bssid = BSSID();
    if(!bssid){
        return std::string();
    }
    char mac[18] = { 0 };
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return std::string(mac);
}

int8_t WiFiSTAClass::RSSI(void)
{
    wifi_ap_record_t info;
    if(!esp_wifi_sta_get_ap_info(&info)) {
        return info.rssi;
    }
    return 0;
}

const char * WiFiSTAClass::getHostname()
{
    const char * hostname = NULL;
    if(tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &hostname)){
        return NULL;
    }
    return hostname;
}

bool WiFiSTAClass::setHostname(const char * hostname)
{
    _hostname = hostname;
    return tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname) == 0;
}

bool WiFiSTAClass::enableIpV6()
{
    return tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA) == 0;
}

IPAddress WiFiSTAClass::calculateNetworkID(IPAddress ip, IPAddress subnet) {
	IPAddress networkID;

	for (size_t i = 0; i < 4; i++)
		networkID[i] = subnet[i] & ip[i];

	return networkID;
}

IPAddress WiFiSTAClass::calculateBroadcast(IPAddress ip, IPAddress subnet) {
    IPAddress broadcastIp;
    
    for (int i = 0; i < 4; i++)
        broadcastIp[i] = ~subnet[i] | ip[i];

    return broadcastIp;
}

uint8_t WiFiSTAClass::calculateSubnetCIDR(IPAddress subnetMask) {
	uint8_t CIDR = 0;

	for (uint8_t i = 0; i < 4; i++) {
		if (subnetMask[i] == 0x80)  // 128
			CIDR += 1;
		else if (subnetMask[i] == 0xC0)  // 192
			CIDR += 2;
		else if (subnetMask[i] == 0xE0)  // 224
			CIDR += 3;
		else if (subnetMask[i] == 0xF0)  // 242
			CIDR += 4;
		else if (subnetMask[i] == 0xF8)  // 248
			CIDR += 5;
		else if (subnetMask[i] == 0xFC)  // 252
			CIDR += 6;
		else if (subnetMask[i] == 0xFE)  // 254
			CIDR += 7;
		else if (subnetMask[i] == 0xFF)  // 255
			CIDR += 8;
	}

	return CIDR;
}