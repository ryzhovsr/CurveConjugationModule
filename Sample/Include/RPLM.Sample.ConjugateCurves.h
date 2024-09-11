#pragma once

#include <Geometry/Curves/NURBSCurve.h>

namespace ConjugateMethods
{
	// ���������� ������ - ���������� ������ �������������
	RGK::NURBSCurve conjugateCurve(const RGK::NURBSCurve& curve, int orderFixFirstDeriv, int orderFixLastDeriv);
}
