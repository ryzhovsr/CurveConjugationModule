#pragma once

#include <Geometry/Curves/NURBSCurve.h>

namespace Sample
{
	namespace ConjugationMethods
	{
		/// <summary>Сопряжение кривой с дополнительными ограничениями в первой и в конечной производной</summary>
		/// <param name="iCurve"></param>
		/// <param name="iOrderFixFirstDeriv">Порядок первой производной</param>
		/// <param name="iOrderFixLastDeriv">Порядок конечной производной</param>
		/// <returns>Сопряжённая кривая</returns>
		RGK::NURBSCurve conjugateCurve(const RGK::NURBSCurve& iCurve, bool fixBeginningCurve, bool fixEndCurve);
	}
}
