#include "IIRSecondOrder.h"

IIRSecondOrder::IIRSecondOrder(const double &c0, const double &c1, const double &c2, const double &d0, const double &d1, const double &sampleTime) :
	sampleTime(sampleTime),
	c0(c0),
	c1(c1),
	c2(c2),
	d0(d0),
	d1(d1),
	lastInput(0),
	lastOutput(0),
	lastLastInput(0),
	lastLastOutput(0)
{}

void IIRSecondOrder::setInitialValues(const double &input, const double &output)
{
	lastInput = input;
	lastLastInput = input;

	lastOutput = output;
	lastLastOutput = output;
}

double IIRSecondOrder::step(const double &input)
{
	double output = c2 * input + c1 * lastInput - d1 * lastOutput + c0 * lastLastInput - d0 * lastLastOutput;

	lastLastInput = lastInput;
	lastInput = input;

	lastLastOutput = lastOutput;
	lastOutput = output;

	return output;
}
