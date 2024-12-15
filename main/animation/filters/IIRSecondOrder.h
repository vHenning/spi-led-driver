#ifndef IIR_SECOND_ORDER
#define IIR_SECOND_ORDER

class IIRSecondOrder
{
public:
	IIRSecondOrder(const double &c0, const double &c1, const double &c2, const double &d0, const double &d1, const double &sampleTime);

	void setInitialValues(const double &input, const double &output);

	double step(const double &input);

private:
	double sampleTime;

	double c0;
	double c1;
	double c2;

	double d0;
	double d1;

	double lastInput;
	double lastOutput;

	double lastLastInput;
	double lastLastOutput;
};

#endif
