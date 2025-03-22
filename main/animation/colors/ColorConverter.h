#ifndef COLOR_CONVERTER_H
#define COLOR_CONVERTER_H

#include <inttypes.h>

namespace ColorConverter
{

static const double WARM_TEMPERATURE = 3000; // Kelvin
static const double COLD_TEMPERATURE = 6500; // Kelvin

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

typedef struct {
    rgb color;
    double ww;
    double cw;
} rgbcct;

typedef struct {
    hsv color;
    double whiteTemp;
    double whiteValue;
} hsvcct;

/// If false, the CCT brightness will be in set according to the brightness of one white channel. Pro: Brightness does not change when changing color temperature. Contra: We only get half of the brightness that both white leds could make.
/// If true, the CCT brightness will be set to the maximum brightness that can be produced with that color temperature. Pro: Maximum white brightness. Contra: Brightness changes when changing color temperature (brightest in the middle: both white LEDs at 100%, darkest at the edges: only one white LED at 100%, the other at 0%)
void setMaxWhiteBrightness(bool max);

hsv rgb2hsv(rgb in);
hsvcct rgb2hsv(rgbcct in);
rgb hsv2rgb(hsv in);
rgbcct hsv2rgb(hsvcct in);

/// Converts given color to 8 bit values saved in the least significant bits of the returned integer
/// Color order is Blue Red Green
/// @param in Color to convert to 8 bit values
/// @return Format 0xBBRRGG
uint64_t to8BitBRG(const rgb in);

/// Converts given color to 8 bit values saved in the least significant bits of the returned integer
/// Color order is cold warm blue red green
/// @param in Color to convert to 8 bit values
/// @return Format 0xCWWWBBRRGG
uint64_t to8BitWWBRG(const rgbcct in);

} // namespace ColorConverter

#endif // COLOR_CONVERTER_H
