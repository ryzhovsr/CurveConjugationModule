#include <Common/RPLM.Base.Framework.String.h>
#include <RGPSession.h>

namespace Sample
{
	namespace Utils
	{
		/// <summary>Считывает контрольные точки из файла </summary>
		/// <param name="iFilePath">Путь к файлу</param>
		/// <returns>Контрольные точки</returns>
		RGK::Vector<RGK::Math::Vector3D> readControlPointsFromFile(const RPLM::Base::Framework::String& iFilePath);

		/// <summary>Считывает узлы из файла</summary>
		/// <param name="iFilePath">Путь к файлу</param>
		/// <returns>Вектор узлов</returns>
		RPLM::Math::Geometry2D::Geometry::DoubleArray readKnotsFromFile(const RPLM::Base::Framework::String& iFilePath);

        /// <summary> </summary>
        /// <param name="iFilePath">Путь к файлу</param>
        /// <param name="iControlPoints">Контрольные точки кривой</param>
        void writeControlPointsInFile(const RPLM::Base::Framework::String& iFilePath, const RGK::Vector<RGK::Math::Vector3D>& iControlPoints);
	}
}
