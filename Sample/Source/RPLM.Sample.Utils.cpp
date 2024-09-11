#include "RPLM.Sample.Utils.h"
#include <fstream>

namespace Sample
{
	RGK::Vector<RGK::Math::Vector3D> Utils::readControlPointsFromFile(const RPLM::Base::Framework::String& filePath)
	{
		std::string line;
		std::ifstream in(filePath);
		RGK::Vector<RGK::Math::Vector3D> controlPoints;
		// Счётчик контрольных точек
		int controlPointsCounter = 0;
		int pointCoordCounter = 0;

		if (in.is_open())
		{
			RGK::Vector<double> temp;
			double number = 0;

			while (in >> number)
			{
				// Если все три координаты заполнены
				if (pointCoordCounter == 3)
				{
					controlPoints.push_back(RGK::Math::Vector3D(temp[0], temp[1], temp[2]));
					temp.clear();
					pointCoordCounter = 0;
				}

				temp.push_back(number);
				++pointCoordCounter;
			}

			controlPoints.push_back(RGK::Math::Vector3D(temp[0], temp[1], temp[2]));
		}

		in.close();
		return controlPoints;
	}

	RPLM::Math::Geometry2D::Geometry::DoubleArray Sample::Utils::readKnotsFromFile(const RPLM::Base::Framework::String& filePath)
	{
		RPLM::Math::Geometry2D::Geometry::DoubleArray arr;
		
		std::string line;
		std::ifstream in(filePath);

		if (in.is_open())
		{
			double number = 0;

			while (in >> number)
			{
				arr.push_back(number);
			}
		}

		in.close();
		return arr;
	}

	void Utils::writeControlPointsInFile(const RPLM::Base::Framework::String& fileName, const RGK::Vector<RGK::Math::Vector3D>& controlPoints)
	{
		std::ofstream outFile(fileName);

		if (outFile.is_open())
		{
			for (const auto& curvePoints : controlPoints)
			{
				outFile << curvePoints.GetX() << " " << curvePoints.GetY() << " " << curvePoints.GetZ() << '\n';
			}

			outFile.close();
		}
	}
}
