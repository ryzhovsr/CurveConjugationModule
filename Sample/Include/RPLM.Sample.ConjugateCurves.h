#pragma once

#include <Geometry/Curves/NURBSCurve.h>

namespace Sample
{
	namespace ConjugationMethods
	{
		/// <summary>���������� ������ � ��������������� ������������� � ������ � � �������� �����������</summary>
		/// <param name="iCurve"></param>
		/// <param name="iOrderFixFirstDeriv">������� ������ �����������</param>
		/// <param name="iOrderFixLastDeriv">������� �������� �����������</param>
		/// <returns>���������� ������</returns>
		RGK::NURBSCurve conjugateCurve(const RGK::NURBSCurve& iCurve, int iOrderFixFirstDeriv, int iOrderFixLastDeriv);
	}
}
