#include "LEDDriver.h"

#include <cstring>
#include <esp_system.h>

LEDDriver::LEDDriver(gpio_num_t pin, size_t leds)
    : LED_COUNT(leds)
    , colorBuffer(NULL)
{
    const int CHANNELS = 5; // Red, Green, Blue, Cold white, Warm white
    const int CHANNEL_BYTES = 1;

    colorBuffer = new uint8_t[LED_COUNT * CHANNELS * CHANNEL_BYTES];

    const int RESOLUTION_HZ = 20000000; // 20 MHz
    const int RESOLUTION_NS = 50; // [ns] (=1/20 MHZ)

    // Configure RMT channel
    rmt_tx_channel_config_t config;
    config.clk_src = RMT_CLK_SRC_APB;
    config.gpio_num = pin;
    config.intr_priority = 0;
    config.mem_block_symbols = 128;
    config.resolution_hz = RESOLUTION_HZ;
    config.trans_queue_depth = 4;
    config.flags.with_dma = 0;
    config.flags.invert_out = 0;

    rmt_new_tx_channel(&config, &channel);
    rmt_enable(channel);

    // Configure data encoder (tell rmt how to send a 1 and a 0)
    rmt_bytes_encoder_config_t dataEncoderConfig;
    dataEncoderConfig.bit0.level0 = 1;
    dataEncoderConfig.bit0.duration0 = 300 / RESOLUTION_NS;
    dataEncoderConfig.bit0.level1 = 0;
    dataEncoderConfig.bit0.duration1 = 950 / RESOLUTION_NS;
    dataEncoderConfig.bit1.level0 = 1;
    dataEncoderConfig.bit1.duration0 = 950 / RESOLUTION_NS;
    dataEncoderConfig.bit1.level1 = 0;
    dataEncoderConfig.bit1.duration1 = 300 / RESOLUTION_NS;
    dataEncoderConfig.flags.msb_first = 1;
    rmt_new_bytes_encoder(&dataEncoderConfig, &ledEncoder.dataEncoder);

    // Configure reset encoder (it just sends the "end of transmission" word after we sent all LED data)
    rmt_copy_encoder_config_t copyEncoderConfig;
    rmt_new_copy_encoder(&copyEncoderConfig, &ledEncoder.resetEncoder);

    // The reset word (="End of transmission") is just 300 µs of low level
    const int RESET_DURATION = 300000; // [ns] = 300 µs
    ledEncoder.resetWord.duration0 = (RESET_DURATION / 2) / RESOLUTION_NS;
    ledEncoder.resetWord.duration1 = ledEncoder.resetWord.duration0;
    ledEncoder.resetWord.level0 = 0;
    ledEncoder.resetWord.level1 = 0;

    // We also act as an encoder. We are the first instance that the rmt library calls. We then manage the two other encoders accordingly.
    ledEncoder.parentEncoder.encode = &LEDDriver::encoderEncode;
    ledEncoder.parentEncoder.reset = &LEDDriver::encoderReset;
    ledEncoder.parentEncoder.del = &LEDDriver::encoderDelete;

    ledEncoder.state = RMT_ENCODING_RESET;
}

void LEDDriver::set(size_t index, uint64_t color)
{
    const int BYTES_PER_LED = 5;
    memcpy(&colorBuffer[index * BYTES_PER_LED], &color, BYTES_PER_LED);
}

void LEDDriver::set(uint64_t color)
{
    const int BYTES_PER_LED = 5;
    for (size_t i = 0; i < LED_COUNT; ++i)
    {
        memcpy(&colorBuffer[i * BYTES_PER_LED], &color, BYTES_PER_LED);
    }
    rmt_transmit_config_t transmitConfig;
    transmitConfig.loop_count = 0;
    transmitConfig.flags.eot_level = 0;
    rmt_transmit(channel, &ledEncoder.parentEncoder, colorBuffer, BYTES_PER_LED * LED_COUNT, &transmitConfig);
}

void LEDDriver::set(uint8_t red, uint8_t green, uint8_t blue, uint8_t warm, uint8_t cold)
{
    const int BYTES_PER_LED = 5;
    for (size_t i = 0; i < LED_COUNT; ++i)
    {
        memcpy(&colorBuffer[i * BYTES_PER_LED], &green, sizeof(uint8_t));
        memcpy(&colorBuffer[i * BYTES_PER_LED + 1], &red, sizeof(uint8_t));
        memcpy(&colorBuffer[i * BYTES_PER_LED + 2], &blue, sizeof(uint8_t));
        memcpy(&colorBuffer[i * BYTES_PER_LED + 3], &warm, sizeof(uint8_t));
        memcpy(&colorBuffer[i * BYTES_PER_LED + 4], &cold, sizeof(uint8_t));
    }
    rmt_transmit_config_t transmitConfig;
    transmitConfig.loop_count = 0;
    transmitConfig.flags.eot_level = 0;
    rmt_transmit(channel, &ledEncoder.parentEncoder, colorBuffer, BYTES_PER_LED * LED_COUNT, &transmitConfig);
}

void LEDDriver::set(size_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t warm, uint8_t cold)
{
    const int BYTES_PER_LED = 5;
    if (index < LED_COUNT)
    {
        memcpy(&colorBuffer[index * BYTES_PER_LED], &green, sizeof(uint8_t));
        memcpy(&colorBuffer[index * BYTES_PER_LED + 1], &red, sizeof(uint8_t));
        memcpy(&colorBuffer[index * BYTES_PER_LED + 2], &blue, sizeof(uint8_t));
        memcpy(&colorBuffer[index * BYTES_PER_LED + 3], &warm, sizeof(uint8_t));
        memcpy(&colorBuffer[index * BYTES_PER_LED + 4], &cold, sizeof(uint8_t));
    }
}

void LEDDriver::refresh()
{
    const int BYTES_PER_LED = 5;
    rmt_transmit_config_t transmitConfig;
    transmitConfig.loop_count = 0;
    transmitConfig.flags.eot_level = 0;
    rmt_transmit(channel, &ledEncoder.parentEncoder, colorBuffer, BYTES_PER_LED * LED_COUNT, &transmitConfig);
}

void LEDDriver::wait()
{
    rmt_tx_wait_all_done(channel, -1);
}

size_t IRAM_ATTR LEDDriver::encoderEncode(rmt_encoder_t* encoder, rmt_channel_handle_t tx_channel, const void* primary_data, size_t data_size, rmt_encode_state_t* ret_state)
{
    EncoderContainer* instance = __containerof(encoder, EncoderContainer, parentEncoder);
    rmt_encode_state_t resultState = RMT_ENCODING_RESET;
    size_t encoded = 0;

    if (instance->state == RMT_ENCODING_RESET)
    {
        // Encode raw data
        encoded += instance->dataEncoder->encode(instance->dataEncoder, tx_channel, primary_data, data_size, &resultState);

        if (resultState & RMT_ENCODING_COMPLETE)
        {
            // Encoded all raw data. Proceed with reset word.
            instance->state = RMT_ENCODING_COMPLETE;
        }
        if (resultState & RMT_ENCODING_MEM_FULL)
        {
            // Memory full. Return. This function will be called again when memory is available.
            *ret_state = RMT_ENCODING_MEM_FULL;
            return encoded;
        }
    }

    if (instance->state == RMT_ENCODING_COMPLETE)
    {
        // All raw data was encoded, append reset word
        encoded += instance->resetEncoder->encode(instance->resetEncoder, tx_channel, &instance->resetWord, sizeof(instance->resetWord), &resultState);

        if (resultState & RMT_ENCODING_COMPLETE)
        {
            // We are done with this batch.
            instance->state = RMT_ENCODING_RESET;
        }
    }
    *ret_state = resultState;
    return encoded;
}

esp_err_t LEDDriver::encoderReset(rmt_encoder_t* encoder)
{
    EncoderContainer* container = __containerof(encoder, EncoderContainer, parentEncoder);
    esp_err_t error = ESP_OK;
    error |= rmt_encoder_reset(container->dataEncoder);
    error |= rmt_encoder_reset(container->resetEncoder);
    container->state = RMT_ENCODING_RESET;
    return error;
}

esp_err_t LEDDriver::encoderDelete(rmt_encoder_t* encoder)
{
    return ESP_OK;
}
