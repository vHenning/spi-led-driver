#ifndef LEDDRIVER_H
#define LEDDRIVER_H

#include <driver/rmt_tx.h>

class LEDDriver
{
public:
    LEDDriver(gpio_num_t pin, size_t leds);

    /// Sends given data (40 bit of it) to all LEDs
    /// @param color Color data to send to all LEDs. We use the 40 least significant bits. 8 bit per color.
    void set(uint64_t color);

    /// Set single Pixel/LED color at index without writing to LEDs
    void set(size_t index, uint64_t color);

    /// Sets all LEDs the given color
    void set(uint8_t red, uint8_t green, uint8_t blue, uint8_t warm, uint8_t cold);

    /// Sets single LED. Does not actually write it to the LED.
    void set(size_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t warm, uint8_t cold);

    /// Writes currently set colors to all LEDs
    void refresh();

    /// Wait (block) until rmt transmission is finished
    void wait();

private:
    static size_t encoderEncode(rmt_encoder_t* encoder, rmt_channel_handle_t tx_channel, const void* primary_data, size_t data_size, rmt_encode_state_t* ret_state);
    static esp_err_t encoderReset(rmt_encoder_t* encoder);
    static esp_err_t encoderDelete(rmt_encoder_t* encoder);

private:
    const size_t LED_COUNT;

    /// Contains raw color data for all LEDs
    uint8_t* colorBuffer;

    rmt_channel_handle_t channel;

    struct EncoderContainer
    {
        /// This is our custom encoder that is used to encode LED data.
        /// It is linked to our three private encoder functions (encoderEncode, encoderReset and encoderDelete).
        /// It uses the other two encoders in this struct when encoding.
        rmt_encoder_t parentEncoder;

        /// Reset: Ready to encode some raw data
        /// Complete: Raw data encoding finished, just need to attach the reset word
        /// Memory Full: Not used here.
        rmt_encode_state_t state;
        
        /// Converts raw color data to esp rmt data
        rmt_encoder_handle_t dataEncoder;
        
        /// Provides the reset signal (= end of transmission)
        rmt_encoder_handle_t resetEncoder;

        /// The "Reset" (= end of transmission) sequence
        rmt_symbol_word_t resetWord;
    } ledEncoder;
};

#endif // LEDDRIVER_H
