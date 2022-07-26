
#include "BPCCode.h"
#include <esp_system.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h> 
#include "freertos/task.h"
#include "esp_log.h"
#include "WiFi.h"

BPCCode *_bpc_coder = NULL;

static const char* TAG = "BPCSender";

static uint8_t check_snpt_server = 0;;

extern "C"
{
    void app_main(void);
}

void loop()
{
    if(!_bpc_coder)
        return;

    struct tm info;
    time_t now;
    time(&now);
    localtime_r(&now, &info);

    if(info.tm_sec == 1 || info.tm_sec == 21 || info.tm_sec == 41){

        if(info.tm_year == 70){
            ESP_LOGE(TAG, "uninitialized date : %d", info.tm_year+1900);
            if(check_snpt_server > 5)
                esp_restart();
            check_snpt_server ++ ;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            return;
        }
        check_snpt_server = 0;
        _bpc_coder->send_from_tm(info);
    }
    
    //bpc_coder.test_print();  
}

void main_loop(void *pvParameters)
{
    _bpc_coder = new BPCCode(26);
    while (true)
    {
        loop();
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}


void app_main() 
{
    ESP_ERROR_CHECK(nvs_flash_init());

    uint8_t mac[6];
    char macStr[13] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    std::string str_mac("BPCCoder-");
    str_mac.append(macStr);

    WiFi.init(str_mac.c_str());

    BPCCode::initialize_sntp();

    WiFi.begin("heculess", "lc3.1415926");
    int max_try = 3; 
    for (int i = 0; i < max_try; i++)
    {
        int status = WiFi.status();
        if (status == WL_CONNECTED)
            break;
        else
            vTaskDelay(1000/ portTICK_PERIOD_MS);
    }

    vTaskDelay(200/ portTICK_PERIOD_MS);

    xTaskCreatePinnedToCore(main_loop, "main_loop", 4096, NULL,
                            configMAX_PRIORITIES - 1, NULL, 0);
}