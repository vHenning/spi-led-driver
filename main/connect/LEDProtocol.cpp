#include "LEDProtocol.h"

#include <math.h>

#include <esp_log.h>

LEDProtocol::LEDProtocol(CarLight* light)
	: lightDriver(light)
{}

void LEDProtocol::parse(const uint8_t* buffer, const size_t &size)
{
	if (size < 4)
	{
		ESP_LOGI("LED_Protocol", "Message was invalid (Shorter than 4)");
		return;
	}

	uint32_t id = 0;
	memcpy(&id, buffer, sizeof(uint32_t));

	switch (id)
	{
		case 0x100:
		{
			executeMessage(ColorMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x101:
		{
			executeMessage(DimMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x102:
		{
			executeMessage(ValueMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x103:
		{
			executeMessage(FilterMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x104:
		{
			executeMessage(SetFilterValuesMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x105:
		{
			executeMessage(SetFilterValuesBufferMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x106:
		{
			executeMessage(WhiteDimMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x107:
		{
			executeMessage(WhiteTemperatureMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x108:
		{
			executeMessage(TurnOnOffMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		case 0x109:
		{
			executeMessage(WhiteMaxBrightnessMessage(&buffer[sizeof(uint32_t)]));
			break;
		}
		default:
		{
			break;
		}
	}
}

void LEDProtocol::executeMessage(const ColorMessage &message)
{
	float red = static_cast<float>(message.red) / 0xFFFF;
	float green = static_cast<float>(message.green) / 0xFFFF;
	float blue = static_cast<float>(message.blue) / 0xFFFF;

	switch (message.channel)
	{
	case 0:
		ESP_LOGI("LEDProtocol", "Color Message RGB %.02f %.02f %.02f", red, green, blue);
		lightDriver->setColor(red, green, blue);
		break;
	case 1:
		break;
	default:
		break;
	}
}

LEDProtocol::ColorMessage::ColorMessage(const uint8_t* buffer) : LEDMessage(0x100, buffer)
{
	const size_t colorSize = sizeof(uint16_t);

	memcpy(&red,   &message[0 * colorSize], colorSize);
	memcpy(&green, &message[1 * colorSize], colorSize);
	memcpy(&blue,  &message[2 * colorSize], colorSize);
}

void LEDProtocol::executeMessage(const DimMessage &message)
{
	switch (message.channel)
	{
	case 0:
		ESP_LOGI("LEDProtocol", "Set Dim %f", message.dim);
		lightDriver->setColorBrightness(message.dim);
		break;
	case 1:
		break;
	default:
		break;
	}
}

LEDProtocol::DimMessage::DimMessage(const uint8_t* buffer) : LEDMessage(0x101, buffer)
{
	memcpy(&dim, message, sizeof(double));
}

void LEDProtocol::executeMessage(const ValueMessage &message)
{
	// Not implemented
}

LEDProtocol::ValueMessage::ValueMessage(const uint8_t* buffer) : LEDMessage(0x102, buffer)
{
	const size_t colorSize = sizeof(uint16_t);

	memcpy(&red,   &message[0 * colorSize], colorSize);
	memcpy(&green, &message[1 * colorSize], colorSize);
	memcpy(&blue,  &message[2 * colorSize], colorSize);

	uint8_t rawChar = 0;

	memcpy(&rawChar, &message[3 * colorSize], sizeof(uint8_t));
	raw = 0x01 & rawChar;
}

void LEDProtocol::executeMessage(const WhiteTemperatureMessage &message)
{
	switch (message.channel)
	{
	case 0:
		ESP_LOGI("LEDProtocol", "White Temperature %f", message.temperature);
		lightDriver->setWhiteTemperature(message.temperature);
		break;
	case 1:
		break;
	default:
		break;
	}
}

LEDProtocol::WhiteTemperatureMessage::WhiteTemperatureMessage(const uint8_t* buffer) : LEDMessage(0x107, buffer)
{
	memcpy(&temperature, message, sizeof(double));
}

void LEDProtocol::executeMessage(const WhiteDimMessage &message)
{
	switch (message.channel)
	{
	case 0:
		ESP_LOGI("LEDProtocol", "Dim White %f", message.dim);
		lightDriver->setWhiteBrightness(message.dim);
		break;
	case 1:
		break;
	default:
		break;
	}
}

LEDProtocol::WhiteMaxBrightnessMessage::WhiteMaxBrightnessMessage(const uint8_t* buffer) : LEDMessage(0x109, buffer)
{
	maxBrightness = *message == 1;
}

void LEDProtocol::executeMessage(const WhiteMaxBrightnessMessage &message)
{
	switch (message.channel)
	{
		case 0:
			ESP_LOGI("LEDProtocol", "Max White Brightness %s", message.maxBrightness ? "true" : "false");
			ColorConverter::setMaxWhiteBrightness(message.maxBrightness);
			break;
		default:
			break;
	}
}

LEDProtocol::WhiteDimMessage::WhiteDimMessage(const uint8_t* buffer) : LEDMessage(0x106, buffer)
{
	memcpy(&dim, message, sizeof(double));
}

void LEDProtocol::executeMessage(const FilterMessage &message)
{
	switch (message.channel)
	{
	case 0:
		// if (message.useFilter && !led0.usesFilter())
		// {
		// 	led0.useFilter(stepSize);
		// }
		// else if (!message.useFilter)
		// {
		// 	led0.stopUsingFilter();
		// }
		break;
	case 1:
		// if (message.useFilter && !led1.usesFilter())
		// {
		// 	led1.useFilter(stepSize);
		// }
		// else if (!message.useFilter)
		// {
		// 	led1.stopUsingFilter();
		// }
		break;
	default:
		break;
	}
}

LEDProtocol::FilterMessage::FilterMessage(const uint8_t* buffer) : LEDMessage(0x103, buffer)
{
	uint8_t filterChar = 0;
	memcpy(&filterChar, message, sizeof(uint8_t));

	useFilter = 0x01 & filterChar;
}

void LEDProtocol::executeMessage(const SetFilterValuesMessage &message)
{
	switch (message.channel)
	{
	case 0:
		lightDriver->setFilterValues(message.capacitance, message.resistance);
		break;
	case 1:
		break;
	default:
		break;
	}
}

LEDProtocol::SetFilterValuesMessage::SetFilterValuesMessage(const uint8_t* buffer) : LEDMessage(0x104, buffer)
{
	size_t valueSize = sizeof(double);
	memcpy(&capacitance, message, valueSize);
	memcpy(&resistance, &message[valueSize], valueSize);
}

void LEDProtocol::executeMessage(const SetFilterValuesBufferMessage &message)
{
	executeMessage(static_cast<SetFilterValuesMessage>(message));
	switch(message.channel)
	{
	case 0:
		lightDriver->setInitialFilterValues(message.x1, message.y1);
		break;
	case 1:
		break;
	default:
		break;
	}
}

LEDProtocol::SetFilterValuesBufferMessage::SetFilterValuesBufferMessage(const uint8_t* buffer) : SetFilterValuesMessage(buffer)
{
	id = 0x105;

	size_t valueSize = sizeof(double);
	memcpy(&x1, &message[16], valueSize);
	memcpy(&y1, &message[24], valueSize);
}

void LEDProtocol::executeMessage(const TurnOnOffMessage &message)
{
	switch (message.channel)
	{
	case 0:
		ESP_LOGI("LEDProtocol", "Turn %s message", message.on ? "on" : "off");
		message.on ? lightDriver->turnOn() : lightDriver->turnOff();
	case 1:
		break;
	default:
		break;
	}
}

LEDProtocol::TurnOnOffMessage::TurnOnOffMessage(const uint8_t* buffer) : LEDMessage(0x108, buffer)
{
	on = false;
	if (message[0] == 1)
	{
		on = true;
	}
}
