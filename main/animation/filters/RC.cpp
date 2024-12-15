#include "RC.h"

RC::RC(const double& stepTime, const double& resistance, const double& capacitance)
    : IIRSecondOrder(stepTime / (stepTime + 2 * resistance * capacitance),
    stepTime / (stepTime + 2 * resistance * capacitance),
    0,
    (stepTime - 2 * resistance * capacitance) / (stepTime + 2 * resistance * capacitance),
    0,
    stepTime)
{}
