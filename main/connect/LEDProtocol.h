#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <inttypes.h>
#include <cstring>

#include "../animation/CarLight.h"

/**
 * Parses LED control messages
 */
class LEDProtocol
{
public:
	LEDProtocol(CarLight** light, size_t count);

	/**
	 * Parse a message buffer and execute its content
	 * @param buffer - Buffer containing the control message
	 * @param size - Size of the buffer
	 */
	void parse(const uint8_t* buffer, const size_t &size);

protected:

	/**
	 * All Control messages
	 */

	struct LEDMessage
	{
		LEDMessage(const uint32_t &ID, const uint8_t* buffer) : id(ID), message(&buffer[sizeof(uint8_t)])
		{
			memcpy(&channel, buffer, sizeof(uint8_t));
		}

		uint32_t id;
		uint8_t channel;

		const uint8_t* message;
	};

	struct ColorMessage : LEDMessage
	{
		ColorMessage(const uint8_t* buffer);

		uint16_t red;
		uint16_t green;
		uint16_t blue;
	};

	struct DimMessage : LEDMessage
	{
		DimMessage(const uint8_t* buffer);

		double dim;
	};

	struct WhiteDimMessage : LEDMessage
	{
		WhiteDimMessage(const uint8_t* buffer);

		double dim;
	};

	struct ValueMessage : LEDMessage
	{
		ValueMessage(const uint8_t* buffer);

		uint16_t red;
		uint16_t green;
		uint16_t blue;
		bool raw;
	};

	struct WhiteTemperatureMessage : LEDMessage
	{
		WhiteTemperatureMessage(const uint8_t* buffer);

		double temperature;
	};

	struct WhiteMaxBrightnessMessage: LEDMessage
	{
		WhiteMaxBrightnessMessage(const uint8_t* buffer);

		bool maxBrightness;
	};

	struct FilterMessage : LEDMessage
	{
		FilterMessage(const uint8_t* buffer);

		bool useFilter;
	};

	struct SetFilterValuesMessage : LEDMessage
	{
		SetFilterValuesMessage(const uint8_t* buffer);

		double capacitance;
		double resistance;
	};

	struct SetFilterValuesBufferMessage : SetFilterValuesMessage
	{
		SetFilterValuesBufferMessage(const uint8_t* buffer);

		double x1;
		double y1;
	};

	struct TurnOnOffMessage : LEDMessage
	{
		TurnOnOffMessage(const uint8_t* buffer);

		bool on;
	};

	/**
	 * The following methods execute the specific control messages
	 */
	void executeMessage(const ColorMessage &message);
	void executeMessage(const DimMessage &message);
	void executeMessage(const WhiteDimMessage &message);
	void executeMessage(const ValueMessage &message);
	void executeMessage(const WhiteTemperatureMessage &message);
	void executeMessage(const WhiteMaxBrightnessMessage &message);
	void executeMessage(const FilterMessage &message);
	void executeMessage(const SetFilterValuesMessage &message);
	void executeMessage(const SetFilterValuesBufferMessage &message);
	void executeMessage(const TurnOnOffMessage &message);

	CarLight** lightDriver;

	const size_t driverCount;
};

#endif