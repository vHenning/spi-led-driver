#ifndef CAR_LIGHT_H
#define CAR_LIGHT_H

#include <stddef.h>

#include "filters/RC.h"
#include "colors/ColorConverter.h"

class CarLight
{
public:
    CarLight(const double stepTime, const int ledCount, const ColorConverter::rgbcct lightColor);

    void step();

    void turnOn();
    void turnOff();
    bool isOn() const;

    void turnOnBrake();
    void turnOffBrake();

    void turnOnEmergencyBrake();
    void turnOffEmergencyBrake();

    void turnOnLeft();
    void turnOnRight();
    void turnOnHazard();
    void turnOffBlinker();

    void turnOnPolice();
    void turnOffPolice();

    void setColor(float red, float green, float blue);
    ColorConverter::rgb getColor() const;

    void setWhiteTemperature(float temperature);
    float getWhiteTemperature() const;

    void setColorBrightness(float brightness);
    /// Sets Color brightness after turn on/off animation is complete
    void setColorBrightnessAfter(float brightness);
    float getColorBrightness() const;

    void setWhiteBrightness(float brightness);
    void setWhiteBrightnessAfter(float brightness);
    float getWhiteBrightness() const;

    /// Get current colors of all LEDs (Pixels)
    /// @return Array with size of pixel count (use getPixelCount())
    ColorConverter::rgbcct* getPixels() const;

    /// Get Pixel (LED) count
    size_t getPixelCount() const;

private:
    /// Holds colors for each pixel (=LED)
    ColorConverter::rgbcct* colors;

    /// Filters for each color pixel
    RC* colorFilters;

    /// Filters for each cct pixel
    RC* whiteFilters;

    /// How much time passes inbetween step() calls [seconds]
    const double STEP_SIZE;

    /// Total number of LEDs (=pixels)
    const int LED_COUNT;

    /// The base color we display if we are on and nothing is happening
    ColorConverter::rgbcct baseColor;

    /// The general state that can be modified using the public turnOn/Off() functions
    bool on;
    bool braking;
    bool emergencyBraking;
    enum Blinker
    {
        OFF,
        LEFT,
        RIGHT,
        HAZARD
    } blinker;
    bool policeOn;

    /// Brightness of all RGB LEDs
    double colorBrightness;

    /// Brightness of all CCT LEDs
    double whiteBrightness;

    /// Use individual LED filters?
    bool useFilter;

    /// Turn individual LED filters on/off after animation complete?
    bool turnFilterOnAfterChange;
    bool turnFilterOffAfterChange;
    bool changeColorBrightnessAfter;
    bool changeWhiteBrightnessAfter;

    /// Frequency of emergency brake pulses [Hz]
    const double EMERGENCY_BRAKE_FREQUENCY = 5;

    /// Counts the steps spent in emergency brake mode
    int emergencyBrakeCounter;

    /// Position and filter for the on/off animation
    double position;
    RC positionFilter;

    /// Speed of blinker animation [% of total strip per second]
    const double BLINKER_SPEED = 0.3;

    /// Time in between blinks [seconds]
    const double BLINKER_PAUSE = 0.3;

    /// Width of one blinker [% of total strip]
    const float BLINKER_WIDTH = 0.2;

    /// [% of total strip]
    double blinkerPosition; // [% of total strip]

    /// Time blinker was already off in between blinks [seconds]
    double blinkerOffTime;

    /// When the blinker is turned off by the user, we finish the current blink before turning it off.
    /// With this we remember that we want to turn the blinker off when we are finished.
    bool turnOffBlinkerWhenDone;

    /// Counts the steps spent in police mode [-]
    unsigned int policeCounter;

    double normalColorBrightness = 0.5;
    double normalWhiteBrightness = 0.3;
    const double BRAKE_BRIGHTNESS = 1.0;

    double normalColorBrightnessAfter = 0.0;
    double normalWhiteBrightnessAfter = 0.0;
};

#endif