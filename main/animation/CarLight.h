#ifndef CAR_LIGHT_H
#define CAR_LIGHT_H

#include <stddef.h>

#include "filters/RC.h"
#include "colors/ColorConverter.h"

class CarLight
{
public:
    CarLight(const double stepTime, const int ledCount, const ColorConverter::rgb lightColor);

    void step();

    void turnOn();
    void turnOff();

    void turnOnBrake();
    void turnOffBrake();

    void turnOnEmergencyBrake();
    void turnOffEmergencyBrake();

    void left();
    void right();
    void indicatorOff();
    void hazard();

    void policeOn();
    void policeOff();

    /// Get current colors of all LEDs (Pixels)
    /// @return Array with size of pixel count (use getPixelCount())
    ColorConverter::rgb* getPixels() const;

    /// Get Pixel (LED) count
    size_t getPixelCount() const;

private:
    bool on;
    bool braking;
    bool emergencyBraking;

    ColorConverter::rgb* colors;
    RC* filters;
    double stepSize;
    int leds;

    ColorConverter::rgb color;
    double brightness;
    const double NORMAL_BRIGHTNESS = 0.3;
    const double BRAKE_BRIGHTNESS = 1.0;
    bool useFilter;
    bool turnFilterOnAfterChange;
    bool turnFilterOffAfterChange;

    int emergencyBrakeCounter;
    bool turnOffBlinker;

    unsigned int policeCounter;
    bool police;

    /**
     * percent of whole light strip
     */
    const float indicatorWidth = 0.2;

    enum Blinker
    {
        OFF,
        LEFT,
        RIGHT,
        HAZARD
    } blinker;

    /**
     * Position and filter for the on/off animation
     */
    double position;
    RC positionFilter;

    double blinkerPosition; // [% of total strip]
    double blinkerOffTime;
    const double blinkerSpeed = 0.3; // % of total strip per second
    const double blinkerPause = 0.3; // seconds
};

#endif