#pragma once

#include <string>
#include <vector>
#include "stdint.h"

#define BPC_FRAME_SIZE 38



class BPCCode
{
public:
    BPCCode(uint8_t pin_signal);
    virtual ~BPCCode();

    static void initialize_sntp();

    void test_print();

    void send_from_tm(tm &timeinfo);

    bool is_frame_busy();

protected:
    class FrameLocker
    {
    public:
        FrameLocker();
        virtual ~FrameLocker();
    };

    class BPCFrame
    {
    public:
        enum BPC_SECTION
        {
            BPCS_SEC    = 0,
            BPCS_P2     = 2,
            BPCS_HOUR   = 4,
            BPCS_MIN    = 8,
            BPCS_WEEK   = 14,
            BPCS_P3     = 18,
            BPCS_DAY    = 20,
            BPCS_MONTH  = 26,
            BPCS_YEAR   = 30,
            BPCS_P4     = 36,
        };

        BPCFrame();

        void init_from(tm &timeinfo);

        std::vector<uint8_t> to_byte_array();

        std::string to_string();

    private:
        void fill_binary_data(uint8_t *data_array, uint8_t byte_data, uint8_t data_length);
        uint8_t even_check(uint8_t n_start, uint8_t n_end);

        uint8_t to_quaternary(uint8_t n_start, bool single_byte = false);

        uint8_t _frame_data[BPC_FRAME_SIZE]; 
    };

private:
    const uint8_t _pin_signal;
};