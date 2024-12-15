#include "CarLight.h"

#include <math.h>
#include <stddef.h>

#include <esp_log.h>

#include "colors/ColorConverter.h"
#include "colors/GammaCorrection.h"

CarLight::CarLight(const double stepTime, const int ledCount, const ColorConverter::rgb lightColor)
    : colors(new ColorConverter::rgb[ledCount])
    , filters(new RC[ledCount])
    , STEP_SIZE(stepTime)
    , LED_COUNT(ledCount)
    , baseColor(lightColor)
    , on(false)
    , braking(false)
    , emergencyBraking(false)
    , blinker(OFF)
    , policeOn(false)
    , brightness(0.0)
    , useFilter(true)
    , turnFilterOnAfterChange(false)
    , turnFilterOffAfterChange(false)
    , emergencyBrakeCounter(0.0)
    , position(0)
    , positionFilter(stepTime, 200, 0.001)
    , blinkerPosition(0)
    , blinkerOffTime(0)
    , turnOffBlinkerWhenDone(false)
    , policeCounter(0)
{
    for (size_t i = 0; i < ledCount; ++i)
    {
        colors[i] = ColorConverter::rgb(0, 0, 0);
        filters[i] = RC(STEP_SIZE, 100, 0.001);
    }

    brightness = NORMAL_BRIGHTNESS;
}

void CarLight::step()
{
    position = positionFilter.step(on ? (LED_COUNT / 2.0) + 1 : 0);
    if (blinkerOffTime > BLINKER_PAUSE)
    {
        blinkerPosition += BLINKER_SPEED * STEP_SIZE;
    }
    else
    {
        blinkerOffTime += STEP_SIZE;
    }
    if (blinkerPosition > BLINKER_WIDTH)
    {
        if (turnOffBlinkerWhenDone)
        {
            blinker = OFF;
            turnOffBlinkerWhenDone = false;
        }
        blinkerPosition = 0;
        blinkerOffTime = 0;
    }

    ColorConverter::hsv hsv = ColorConverter::rgb2hsv(baseColor);

    static const double emergencyBrakeHalfPeriod = 1.0 / (EMERGENCY_BRAKE_FREQUENCY * 2);
    if (emergencyBraking)
    {
        brightness = emergencyBrakeCounter++ * STEP_SIZE > emergencyBrakeHalfPeriod ? NORMAL_BRIGHTNESS : BRAKE_BRIGHTNESS;

        if (emergencyBrakeCounter * STEP_SIZE > 2 * emergencyBrakeHalfPeriod)
        {
            emergencyBrakeCounter = 0;
        }
    }

    policeCounter++;

    for (size_t i = 0; i < LED_COUNT; ++i)
    {
        bool illuminate = i < floor(position) || i > LED_COUNT - ceil(position);
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

        if (policeOn)
        {
            int millis = policeCounter * STEP_SIZE * 1000;

            static bool on = false;
            static bool left = false;

            if (i == 0)
            {
                if (millis % 100 < STEP_SIZE)
                {
                    on = !on;
                }
                if (millis % 400 < STEP_SIZE)
                {
                    left = !left;
                }
            }

            static const int rightStart = round(0.3 * LED_COUNT);
            static const int rightEnd = round(0.5 * LED_COUNT);
            static const int leftStart = round(0.5 * LED_COUNT);
            static const int leftEnd = round(0.7 * LED_COUNT);

            if ((left && on && (i >= leftStart && i < leftEnd)) || (!left && on && (i >= rightStart && i < rightEnd)))
            {
                rgb.r = 0.0;
                rgb.g = 0.0;
                rgb.b = 1.0;
            }
        }

        int rightStart = round((BLINKER_WIDTH - blinkerPosition) * LED_COUNT);
        int rightEnd   = round(BLINKER_WIDTH * LED_COUNT);
        int leftStart  = round((1.0 - BLINKER_WIDTH) * LED_COUNT);
        int leftEnd    = round(((1.0 - BLINKER_WIDTH) + blinkerPosition) * LED_COUNT);

        if (((blinker == RIGHT || blinker == HAZARD) && i >= rightStart && i < rightEnd)
        ||  ((blinker == LEFT  || blinker == HAZARD) && i >= leftStart  && i < leftEnd))
        {
            rgb.r = 1.0;
            rgb.g = 1.0;
            rgb.b = 0.0;
        }

        static const double max = 0xFF;
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
    return LED_COUNT;
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

void CarLight::turnOnLeft()
{
    if (blinker == OFF)
    {
        blinkerPosition = 0;
    }
    blinker = LEFT;
    turnOffBlinkerWhenDone = false;
}

void CarLight::turnOnRight()
{
    if (blinker == OFF)
    {
        blinkerPosition = 0;
    }
    blinker = RIGHT;
    turnOffBlinkerWhenDone = false;
}

void CarLight::turnOnHazard()
{
    if (blinker == OFF)
    {
        blinkerPosition = 0;
    }
    blinker = HAZARD;
    turnOffBlinkerWhenDone = false;
}

void CarLight::turnOffBlinker()
{
    turnOffBlinkerWhenDone = true;
}

void CarLight::turnOnPolice()
{
    policeOn = true;
}

void CarLight::turnOffPolice()
{
    policeOn = false;
}