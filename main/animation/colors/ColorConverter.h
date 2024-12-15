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

hsv rgb2hsv(rgb in);
hsvcct rgb2hsv(rgbcct in);
rgb hsv2rgb(hsv in);
rgbcct hsv2rgb(hsvcct in);

/// Converts given color to 8 bit values saved in the least significant bits of the returned integer
/// Color order is Blue Red Green
/// @param in Color to convert to 8 bit values
/// @return Format 0xBBRRGG
uint64_t to8BitBRG(const rgb in);

} // namespace ColorConverter

#endif // COLOR_CONVERTER_H
