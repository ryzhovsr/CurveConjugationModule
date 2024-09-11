#include <Common/RPLM.Base.Framework.String.h>
#include <RGPSession.h>

namespace Sample
{
	namespace Utils
	{
		// Считывает контрольные точки из файла
		RGK::Vector<RGK::Math::Vector3D> readControlPointsFromFile(const RPLM::Base::Framework::String& filePath);

		RPLM::Math::Geometry2D::Geometry::DoubleArray readKnotsFromFile(const RPLM::Base::Framework::String& filePath);

        // Запись контрольных точек кривой в файл
        void writeControlPointsInFile(const RPLM::Base::Framework::String& fileName, const RGK::Vector<RGK::Math::Vector3D>& controlPoints);
	}
}
