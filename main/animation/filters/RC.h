#ifndef RC_H
#define RC_H

#include "IIRSecondOrder.h"

class RC : public IIRSecondOrder
{
public:
    RC(const double& stepTime = 0.01, const double& resistance = 100, const double& capacitance = 0.00001);
};

#endif
