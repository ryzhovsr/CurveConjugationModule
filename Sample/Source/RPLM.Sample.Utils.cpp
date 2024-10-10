#include "RPLM.Sample.Utils.h"
#include <fstream>

namespace Sample
{
	RGK::Vector<RGK::Math::Vector3D> Utils::readControlPointsFromFile(const RPLM::Base::Framework::String& iFilePath)
	{
		std::string line;
		std::ifstream in(iFilePath);
		RGK::Vector<RGK::Math::Vector3D> controlPoints;

		// Счётчик координат
		int coordinateCounter = 0;

		if (in.is_open())
		{
			RGK::Vector<double> temp;
			double number = 0;

			while (in >> number)
			{
				// Если все три координаты получены
				if (coordinateCounter == 3)
				{
					controlPoints.push_back(RGK::Math::Vector3D(temp[0], temp[1], temp[2]));
					temp.clear();
					coordinateCounter = 0;
				}

				temp.push_back(number);
				++coordinateCounter;
			}

			// Добавляем последнюю координату
			if (coordinateCounter == 3)
			{
				controlPoints.push_back(RGK::Math::Vector3D(temp[0], temp[1], temp[2]));
			}
		}

		in.close();
		return controlPoints;
	}

	RPLM::Math::Geometry2D::Geometry::DoubleArray Sample::Utils::readKnotsFromFile(const RPLM::Base::Framework::String& iFilePath)
	{
		RPLM::Math::Geometry2D::Geometry::DoubleArray knots;
		
		std::string line;
		std::ifstream in(iFilePath);

		if (in.is_open())
		{
			double number = 0;

			while (in >> number)
			{
				knots.push_back(number);
			}
		}

		in.close();

		return knots;
	}

	void Utils::writeControlPointsInFile(const RPLM::Base::Framework::String& iFilePath, const RGK::Vector<RGK::Math::Vector3D>& iControlPoints)
	{
		std::ofstream outFile(iFilePath);

		if (outFile.is_open())
		{
			for (const auto& curvePoints : iControlPoints)
			{
				outFile << curvePoints.GetX() << " " << curvePoints.GetY() << " " << curvePoints.GetZ() << '\n';
			}

			outFile.close();
		}
	}
}
