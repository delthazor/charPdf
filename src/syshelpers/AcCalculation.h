#pragma once

#include "syshelpers/Config.h"

namespace AcCalculation
{

int ArmorStatContribution(const Config& armorAc, int statMod);

int CalcAc(const Config& characterConfig);

}
