#pragma once

#include <Geometry/Curves/NURBSCurve.h>

namespace ConjugateMethods
{
	// Сопряжение кривой - выполнение полной непрерывности
	RGK::NURBSCurve conjugateCurve(const RGK::NURBSCurve& curve, int orderFixFirstDeriv, int orderFixLastDeriv);
}
