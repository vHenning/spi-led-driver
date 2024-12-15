#include "CarLight.h"

#include <math.h>
#include <stddef.h>

#include <esp_log.h>

#include "colors/ColorConverter.h"
#include "colors/GammaCorrection.h"

CarLight::CarLight(const double stepTime, const int ledCount, const ColorConverter::rgb lightColor)
    : on(false)
    , braking(false)
    , emergencyBraking(false)
    , colors(new ColorConverter::rgb[ledCount])
    , filters(new RC[ledCount])
    , stepSize(stepTime)
    , leds(ledCount)
    , color(lightColor)
    , brightness(0.0)
    , useFilter(true)
    , turnFilterOnAfterChange(false)
    , turnFilterOffAfterChange(false)
    , emergencyBrakeCounter(0.0)
    , indicatorCounter(0.0)
    , indicatorOn(false)
    , turnOffBlinker(false)
    , policeCounter(0)
    , police(false)
    , blinker(OFF)
    , position(0)
    , positionFilter(stepTime, 200, 0.001)
    , blinkerPosition(0)
    , blinkerOffTime(0)
    , blinkerSpeed((40.0/144.0) * ledCount)
{
    for (size_t i = 0; i < ledCount; ++i)
    {
        colors[i] = ColorConverter::rgb(0, 0, 0);
        filters[i] = RC(stepSize, 100, 0.001);
    }

    brightness = NORMAL_BRIGHTNESS;
}

void CarLight::step()
{
    position = positionFilter.step(on ? (leds / 2.0) + 1 : 0);
    if (blinkerOffTime > blinkerPause)
    {
        blinkerPosition += blinkerSpeed * stepSize;
    }
    else
    {
        blinkerOffTime += stepSize;
    }
    if (blinkerPosition > leds * indicatorWidth)
    {
        blinkerPosition = 0;
        blinkerOffTime = 0;
    }
    
    ColorConverter::rgb preRgb;
    preRgb.r = color.r;
    preRgb.g = color.g;
    preRgb.b = color.b;
    double max = (double) 0xFF;

    ColorConverter::hsv hsv = ColorConverter::rgb2hsv(preRgb);

    const double emergencyBrakeFrequency = 5; // Hz
    const double emergencyBrakeHalfPeriod = 1.0 / (emergencyBrakeFrequency * 2);
    if (emergencyBraking)
    {
        brightness = emergencyBrakeCounter++ * stepSize > emergencyBrakeHalfPeriod ? NORMAL_BRIGHTNESS : BRAKE_BRIGHTNESS;

        if (emergencyBrakeCounter * stepSize > 2 * emergencyBrakeHalfPeriod)
        {
            emergencyBrakeCounter = 0;
        }
    }

    const float indicatorTime = 0.5;
    if (indicatorCounter++ * stepSize >= indicatorTime)
    {
        if (turnOffBlinker)
        {
            blinker = OFF;
        }
        indicatorOn = !indicatorOn;
        indicatorCounter = 0;
    }

    policeCounter++;

    for (size_t i = 0; i < leds; ++i)
    {
        bool illuminate = i < floor(position) || i > leds - ceil(position);
        double localBrightness = illuminate ? brightness : 0.0;
        hsv.v = useFilter ? filters[i].step(localBrightness) : localBrightness;
        if (turnFilterOnAfterChange)
        {
            useFilter = true;
            turnFilterOnAfterChange = false;
        }
        if (abs(hsv.v - localBrightness) < std::numeric_limits<double>::epsilon() && turnFilterOffAfterChange)
        {
            useFilter = false;
            turnFilterOffAfterChange = false;
        }

        ColorConverter::rgb rgb = ColorConverter::hsv2rgb(hsv);

        if (police)
        {
            int millis = policeCounter * stepSize * 1000;

            static bool on = false;
            static bool left = false;

            if (i == 0)
            {
                if (millis % 100 < stepSize)
                {
                    on = !on;
                }
                if (millis % 400 < stepSize)
                {
                    left = !left;
                }
            }

            if ((left && on && (i > 80 && i < 100)) || (!left && on && (i < 54 && i > 34)))
            {
                rgb.r = 0.0;
                rgb.g = 0.0;
                rgb.b = 1.0;
            }
        }

        if (((blinker == RIGHT || blinker == HAZARD) && i < (leds * indicatorWidth) -1 && i > (leds * indicatorWidth - blinkerPosition) -1)
        ||  (((blinker == LEFT || blinker == HAZARD) && i > leds - (leds * indicatorWidth)) && i < leds - (leds * indicatorWidth) + blinkerPosition))
        {
            rgb.r = 1.0;
            rgb.g = 1.0;
            rgb.b = 0.0;
        }

        uint8_t red = gamma8[static_cast<uint8_t>(rgb.r * max)];
        uint8_t green = gamma8[static_cast<uint8_t>(rgb.g * max)];
        uint8_t blue = gamma8[static_cast<uint8_t>(rgb.b * max)];

        colors[i].r = red / max;
        colors[i].g = green / max;
        colors[i].b = blue / max;
    }
}

ColorConverter::rgb* CarLight::getPixels() const
{
    return colors;
}

size_t CarLight::getPixelCount() const
{
    return leds;
}

void CarLight::turnOn()
{
    on = true;
}

void CarLight::turnOff()
{
    on = false;
}

void CarLight::turnOnBrake()
{
    braking = true;
    brightness = BRAKE_BRIGHTNESS;
    useFilter = false;
}

void CarLight::turnOffBrake()
{
    braking = false;
    brightness = NORMAL_BRIGHTNESS;
    if (!emergencyBraking)
    {
        turnFilterOnAfterChange = true;
    }
}

void CarLight::turnOnEmergencyBrake()
{
    emergencyBraking = true;
    useFilter = false;
    emergencyBrakeCounter = 0;
}

void CarLight::turnOffEmergencyBrake()
{
    emergencyBraking = false;
    if (!braking)
    {
        turnFilterOnAfterChange = true;
    }
}

void CarLight::left()
{
    if (blinker == OFF)
    {
        indicatorCounter = 0;
        blinkerPosition = 0;
    }
    blinker = LEFT;
    turnOffBlinker = false;
}

void CarLight::right()
{
    if (blinker == OFF)
    {
        indicatorCounter = 0;
        blinkerPosition = 0;
    }
    blinker = RIGHT;
    turnOffBlinker = false;
}

void CarLight::indicatorOff()
{
    turnOffBlinker = true;
}

void CarLight::hazard()
{
    if (blinker == OFF)
    {
        indicatorCounter = 0;
        blinkerPosition = 0;
    }
    blinker = HAZARD;
    turnOffBlinker = false;
}

void CarLight::policeOn()
{
    police = true;
}

void CarLight::policeOff()
{
    police = false;
}