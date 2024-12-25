#include "CarLight.h"

#include <math.h>
#include <stddef.h>

#include <esp_log.h>

#include "colors/ColorConverter.h"
#include "colors/GammaCorrection.h"

CarLight::CarLight(const double stepTime, const int ledCount, const ColorConverter::rgbcct lightColor)
    : colors(new ColorConverter::rgbcct[ledCount])
    , colorFilters(new RC[ledCount])
    , whiteFilters(new RC[ledCount])
    , STEP_SIZE(stepTime)
    , LED_COUNT(ledCount)
    , baseColor(lightColor)
    , on(false)
    , braking(false)
    , emergencyBraking(false)
    , blinker(OFF)
    , policeOn(false)
    , colorBrightness(0.0)
    , whiteBrightness(0.0)
    , useFilter(true)
    , turnFilterOnAfterChange(false)
    , turnFilterOffAfterChange(false)
    , changeColorBrightnessAfter(false)
    , changeWhiteBrightnessAfter(false)
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
        colors[i] = ColorConverter::rgbcct(ColorConverter::rgb(0, 0, 0), 0, 0);
        colorFilters[i] = RC(STEP_SIZE, 100, 0.001);
        whiteFilters[i] = RC(STEP_SIZE, 100, 0.001);
    }

    colorBrightness = normalColorBrightness;
    whiteBrightness = normalWhiteBrightness;
}

void CarLight::step()
{
    double desiredPosition = on ? (LED_COUNT / 2.0) + 1 : 0;
    position = positionFilter.step(desiredPosition);
    if (changeColorBrightnessAfter && (position - desiredPosition) < 1)
    {
        normalColorBrightness = normalColorBrightnessAfter;
        if (!braking)
        {
            colorBrightness = normalColorBrightness;
        }
        changeColorBrightnessAfter = false;
    }
    if (changeWhiteBrightnessAfter && (position - desiredPosition) < 1)
    {
        normalWhiteBrightness = normalWhiteBrightnessAfter;
        if (!braking)
        {
            whiteBrightness = normalWhiteBrightness;
        }
        changeWhiteBrightnessAfter = false;
    }
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

    ColorConverter::hsvcct hsv = ColorConverter::rgb2hsv(baseColor);

    static const double emergencyBrakeHalfPeriod = 1.0 / (EMERGENCY_BRAKE_FREQUENCY * 2);
    if (emergencyBraking)
    {
        colorBrightness = emergencyBrakeCounter++ * STEP_SIZE > emergencyBrakeHalfPeriod ? normalColorBrightness : BRAKE_BRIGHTNESS;
        whiteBrightness = 0.0;

        if (emergencyBrakeCounter * STEP_SIZE > 2 * emergencyBrakeHalfPeriod)
        {
            emergencyBrakeCounter = 0;
        }
    }

    policeCounter++;

    for (size_t i = 0; i < LED_COUNT; ++i)
    {
        bool illuminate = i < floor(position) || i > LED_COUNT - ceil(position);
        double localColorBrightness = illuminate ? colorBrightness : 0.0;
        double localWhiteBrightness = illuminate ? whiteBrightness : 0.0;
        double colorValue = useFilter ? colorFilters[i].step(localColorBrightness) : localColorBrightness;
        double whiteValue = useFilter ? whiteFilters[i].step(localWhiteBrightness) : localWhiteBrightness;
        hsv.color.v = colorValue;
        hsv.whiteValue = whiteValue;
        if (turnFilterOnAfterChange)
        {
            useFilter = true;
            turnFilterOnAfterChange = false;
        }
        if (abs(whiteValue - localWhiteBrightness) < std::numeric_limits<double>::epsilon() && abs(colorValue - localColorBrightness) < std::numeric_limits<double>::epsilon() && turnFilterOffAfterChange)
        {
            useFilter = false;
            turnFilterOffAfterChange = false;
        }

        ColorConverter::rgbcct rgb = ColorConverter::hsv2rgb(hsv);

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
                rgb.color.r = 0.0;
                rgb.color.g = 0.0;
                rgb.color.b = 1.0;
                rgb.cw = 0;
                rgb.ww = 0;
            }
        }

        int rightStart = round((BLINKER_WIDTH - blinkerPosition) * LED_COUNT);
        int rightEnd   = round(BLINKER_WIDTH * LED_COUNT);
        int leftStart  = round((1.0 - BLINKER_WIDTH) * LED_COUNT);
        int leftEnd    = round(((1.0 - BLINKER_WIDTH) + blinkerPosition) * LED_COUNT);

        if (((blinker == RIGHT || blinker == HAZARD) && i >= rightStart && i < rightEnd)
        ||  ((blinker == LEFT  || blinker == HAZARD) && i >= leftStart  && i < leftEnd))
        {
            rgb.color.r = 1.0;
            rgb.color.g = 1.0;
            rgb.color.b = 0.0;
            rgb.cw = 0;
            rgb.ww = 0;
        }

        static const double max = 0xFF;
        uint8_t red = gamma8[static_cast<uint8_t>(rgb.color.r * max)];
        uint8_t green = gamma8[static_cast<uint8_t>(rgb.color.g * max)];
        uint8_t blue = gamma8[static_cast<uint8_t>(rgb.color.b * max)];
        uint8_t warm = gamma8[static_cast<uint8_t>(rgb.ww * max)];
        uint8_t cold = gamma8[static_cast<uint8_t>(rgb.cw * max)];

        colors[i].color.r = red / max;
        colors[i].color.g = green / max;
        colors[i].color.b = blue / max;
        colors[i].ww = warm / max;
        colors[i].cw = cold / max;
    }
}

ColorConverter::rgbcct* CarLight::getPixels() const
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

bool CarLight::isOn() const
{
    return on;
}

void CarLight::turnOnBrake()
{
    braking = true;
    colorBrightness = BRAKE_BRIGHTNESS;
    whiteBrightness = 0;
    useFilter = false;
}

void CarLight::turnOffBrake()
{
    braking = false;
    colorBrightness = normalColorBrightness;
    whiteBrightness = normalWhiteBrightness;
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

void CarLight::setColor(float red, float green, float blue)
{
    baseColor.color = ColorConverter::rgb(red, green, blue);
}

ColorConverter::rgb CarLight::getColor() const
{
    return baseColor.color;
}

void CarLight::setWhiteTemperature(float temperature)
{
    ColorConverter::hsvcct helperTemperature;
    helperTemperature.whiteTemp = temperature;
    helperTemperature.whiteValue = 1;
    ColorConverter::rgbcct helperValues = ColorConverter::hsv2rgb(helperTemperature);
    baseColor.cw = helperValues.cw;
    baseColor.ww = helperValues.ww;
}

float CarLight::getWhiteTemperature() const
{
    ColorConverter::hsvcct helper = ColorConverter::rgb2hsv(baseColor);
    return helper.whiteTemp;
}

void CarLight::setColorBrightness(float brightness)
{
    normalColorBrightness = brightness;
    if (!braking)
    {
        colorBrightness = brightness;
    }
}

float CarLight::getColorBrightness() const
{
    return normalColorBrightness;
}

void CarLight::setColorBrightnessAfter(float brightness)
{
    normalColorBrightnessAfter = brightness;
    changeColorBrightnessAfter = true;
}

void CarLight::setWhiteBrightness(float brightness)
{
    normalWhiteBrightness = brightness;
    if (!braking)
    {
        whiteBrightness = brightness;
    }
}

void CarLight::setWhiteBrightnessAfter(float brightness)
{
    normalWhiteBrightnessAfter = brightness;
    changeWhiteBrightnessAfter = true;
}

float CarLight::getWhiteBrightness() const
{
    return normalWhiteBrightness;
}
