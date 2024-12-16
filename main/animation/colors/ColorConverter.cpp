#include "ColorConverter.h"

#include <math.h>

namespace ColorConverter {

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}

hsvcct rgb2hsv(rgbcct in)
{
    hsvcct ret;
    ret.color = rgb2hsv(in.color);

    // We always want the cold and warm values to add up to the value.
    // This ensures that we keep the same brightness when changing temperature.
    ret.whiteValue = std::min(in.cw + in.ww, 1.0);

    double coldAmount = COLD_TEMPERATURE * in.cw;
    double warmAmount = WARM_TEMPERATURE * in.ww;

    ret.whiteTemp = (coldAmount + warmAmount) / (in.cw + in.ww);

    return ret;
}

rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;
}

rgbcct hsv2rgb(hsvcct in)
{
    rgbcct ret;
    ret.color = hsv2rgb(in.color);

    double temperatureRange = COLD_TEMPERATURE - WARM_TEMPERATURE;

    ret.cw = (in.whiteTemp - WARM_TEMPERATURE) / temperatureRange;
    ret.ww = (COLD_TEMPERATURE - in.whiteTemp) / temperatureRange;

    ret.cw *= in.whiteValue;
    ret.ww *= in.whiteValue;

    return ret;
}

uint64_t to8BitBRG(const rgb in)
{
    int64_t retValue = 0;
    int64_t maxValue = 0xFF;

    int64_t blue = maxValue * in.b;
    retValue = retValue | (blue << 16);
    int64_t red = maxValue * in.r;
    retValue = retValue | (red << 8);
    int64_t green = maxValue * in.g;
    retValue = retValue | green;

    return retValue;
}

uint64_t to8BitWWBRG(const rgbcct in)
{
    int64_t retValue = to8BitBRG(in.color);
    int64_t maxValue = 0xFF;

    int64_t cold = maxValue * in.cw;
    retValue = retValue | (cold << 32);
    int64_t warm = maxValue * in.ww;
    retValue = retValue | (warm << 24);

    return retValue;
}

} // namespace ColorConverter
