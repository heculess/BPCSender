#include "BPCCode.h"
#include "lwip/apps/sntp.h"
#include "driver/gpio.h"
#include <string.h>
#include "esp_log.h"

static const char* TAG = "BPC_CODER";

static bool _bpc_frame_busy = false;

BPCCode::FrameLocker::FrameLocker()
{
    _bpc_frame_busy = true;
}

BPCCode::FrameLocker::~FrameLocker()
{
    _bpc_frame_busy = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


BPCCode::BPCFrame::BPCFrame()
{
    memset(_frame_data, 0, sizeof(_frame_data));
}

void BPCCode::BPCFrame::init_from(tm &timeinfo)
{
    fill_binary_data(&_frame_data[BPCS_SEC],timeinfo.tm_sec/20, 2);
    _frame_data[BPCS_P3] = timeinfo.tm_hour < 12 ? 0:1;
    fill_binary_data(&_frame_data[BPCS_HOUR] ,timeinfo.tm_hour%12, 4);
    fill_binary_data(&_frame_data[BPCS_MIN] ,timeinfo.tm_min, 6);

    uint8_t dayOfWeek = timeinfo.tm_wday;
    if(dayOfWeek == 0)
        dayOfWeek = 7;
    fill_binary_data(&_frame_data[BPCS_WEEK] ,dayOfWeek, 4);
    _frame_data[BPCS_P3+1] = even_check(BPCS_SEC, BPCS_P3);
    fill_binary_data(&_frame_data[BPCS_DAY] ,timeinfo.tm_mday, 6);
    fill_binary_data(&_frame_data[BPCS_MONTH] , timeinfo.tm_mon + 1, 4);
 
    std::string str_year = std::to_string(timeinfo.tm_year);
    uint8_t n_year = atoi(str_year.substr(str_year.length()-2).c_str());
    fill_binary_data(&_frame_data[BPCS_YEAR] , n_year, 6);

    _frame_data[BPCS_P4+1] = even_check(BPCS_DAY,BPCS_P4);
}

void BPCCode::BPCFrame::fill_binary_data(uint8_t *data_array, uint8_t byte_data, uint8_t data_length)
{
    uint8_t data_mask = 0;
    switch (data_length)
    {
    case 2:
        data_mask = 0x2;
        break;
    case 4:
        data_mask = 0x8;
        break;
    case 6:
        data_mask = 0x20;
        break;
    case 8:
        data_mask = 0x80;
        break;
    default:
        break;
    }

    for(int i = 0; i < data_length; i++)
    {
        if(byte_data & data_mask)
            data_array[i] = 1;
        else
            data_array[i] = 0;

        byte_data = byte_data << 1;
    }
}

uint8_t BPCCode::BPCFrame::even_check(uint8_t n_start, uint8_t n_end)
{
    uint8_t bit_check = 0;
    for(int i = n_start; i < n_end; i++)
	{
		if(_frame_data[i])
			bit_check++;
	}
    return bit_check%2;
}

uint8_t BPCCode::BPCFrame::to_quaternary(uint8_t n_start, bool single_byte)
{
    if(single_byte)
        return _frame_data[n_start];

    return _frame_data[n_start]*2 + _frame_data[n_start+1];
}

std::vector<uint8_t> BPCCode::BPCFrame::to_byte_array()
{
    std::vector<uint8_t> data_arry;
    for(int i= 0; i < BPC_FRAME_SIZE ; i++){
        data_arry.push_back(to_quaternary(i));
        i++;
    }
    return data_arry;
}

std::string BPCCode::BPCFrame::to_string()
{
    std::string dats_string;

    for(int i= 0; i < BPC_FRAME_SIZE ; i++){
        dats_string += std::to_string(to_quaternary(i));
        i++;
    }

    return dats_string;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPCCode::BPCCode(uint8_t pin_signal):
_pin_signal(pin_signal)
{
    gpio_pad_select_gpio(_pin_signal);
    gpio_set_direction((gpio_num_t)_pin_signal, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)_pin_signal, 0);
}

BPCCode::~BPCCode()
{

}

void BPCCode::initialize_sntp()
{
    setenv("TZ", "CST-8", 1);
    tzset();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char *)("ntp1.aliyun.com"));
    sntp_init();
}

void BPCCode::send_from_tm(tm &timeinfo)
{
    if(_bpc_frame_busy)
        return;

    FrameLocker locker;

    BPCFrame data_frame;
    data_frame.init_from(timeinfo);

    ESP_LOGI(TAG, "current time : %d-%02d-%02d %02d:%02d:%02d week : %d ", timeinfo.tm_year+1900, 
        timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min,
        timeinfo.tm_sec,timeinfo.tm_wday);
    ESP_LOGI(TAG, "send data string : %s ", data_frame.to_string().c_str());

    std::vector<uint8_t> data_bytes = data_frame.to_byte_array();

    int nSize = data_bytes.size();

    uint pulse_duration = 0;

    for (size_t i = 0; i < nSize; i++)
    {
        uint8_t data = data_bytes[i];
        switch(data)
        {
            case 0:
                pulse_duration = 100;
                break;
            case 1:
                pulse_duration = 200;
                break;
            case 2:
                pulse_duration = 300;
                break;
            case 3:
                pulse_duration = 400;
                break;
        }

        gpio_set_level((gpio_num_t)_pin_signal, 1);
        vTaskDelay(pulse_duration/ portTICK_PERIOD_MS);
        gpio_set_level((gpio_num_t)_pin_signal, 0);
        vTaskDelay((1000-pulse_duration)/portTICK_PERIOD_MS);
    }
}

bool BPCCode::is_frame_busy()
{
    return _bpc_frame_busy;
}

void BPCCode::test_print()
{
    struct tm timeinfo;
    timeinfo.tm_year = 2004-1900;
    timeinfo.tm_mon = 2;
    timeinfo.tm_mday = 9;
    timeinfo.tm_wday = 2;
    timeinfo.tm_hour = 9;
    timeinfo.tm_min = 15;
    timeinfo.tm_sec = 1; //0021033021021030101

    //00 00 1001 001111 0010 01 001001 0011 000100 01

    BPCFrame data_frame;
    data_frame.init_from(timeinfo);
    ESP_LOGD(TAG, "data string : %s ", data_frame.to_string().c_str());


    timeinfo.tm_year = 2008-1900;
    timeinfo.tm_mon = 0;
    timeinfo.tm_mday = 19;
    timeinfo.tm_wday = 6;
    timeinfo.tm_hour = 18;
    timeinfo.tm_min = 14;
    timeinfo.tm_sec = 22; //1012032122103010201

    //01 00 0110 010110 0110 10 010011 0001 001000 01

    data_frame.init_from(timeinfo);
    ESP_LOGD(TAG, "data string : %s ", data_frame.to_string().c_str());
}