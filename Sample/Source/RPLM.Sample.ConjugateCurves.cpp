#include "RPLM.Sample.ConjugateCurves.h"
#include <RGPSession.h>
#include "RPLM.Sample.IMatrixOperations.h"
#include "RPLM.Math.ConstraintSolver.EquationSolver/EquationsMatrix.h"

namespace Sample
{
    // ����������� �� ��������������� ��������� ��� ���������� ������
    namespace ImplConjugateCurve
    {
        using DoubleArray = RPLM::Math::Geometry2D::Geometry::DoubleArray;

        // ��������� NURBS ������ �� ������ �����
        static RGK::Vector<RGK::NURBSCurve> splitting�urveIntoBezierCurves(const RGK::NURBSCurve& originalCurve)
        {
            // ����������� ����� ������������ ������
            RGK::Vector<RGK::Math::Vector3D> controlPointsOriginalCurve = originalCurve.GetControlPoints();
            int degree = originalCurve.GetDegree();

            // ����� ������ �����, �� ������� ����� ������ originalCurve
            size_t numberBezierCurves = controlPointsOriginalCurve.size() / degree;
            RGK::Vector<RGK::NURBSCurve> bezierCurves(numberBezierCurves);
            RGK::NURBSCurve tempBezierCurve;

            for (size_t i = 0; i != numberBezierCurves; ++i)
            {
                RGK::Vector<RGK::Math::Vector3D> tempControlPoints;

                // ��������� �� ������ ����������� ����� ������������ ������ ��� ������ ��������� ����� ������
                for (size_t j = 0; j != static_cast<size_t>(degree) + 1; ++j)
                {
                    tempControlPoints.push_back(controlPointsOriginalCurve[j + i * degree]);
                }

                // TODO! �� ����, ����� �� ������ ��� ������������� rgkContext � ����������� ��� � NURBSCurve::CreateBezier...
                RGK::Context rgkContext;
                RPLM::EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

                RGK::NURBSCurve::CreateBezier(rgkContext, tempControlPoints, degree, tempBezierCurve);
                bezierCurves[i] = tempBezierCurve;
            }

            return bezierCurves;
        }

        // ������� �����-�������� (����) ��� ��������� curveParameter
        static int findSpanForParameter(const std::vector<double>& nodalVector, int degree, double curveParameter)
        {
            size_t numKnots = nodalVector.size();

            if (curveParameter < nodalVector[degree] || curveParameter > nodalVector[numKnots - degree - 1])
            {
                throw _STR("Error! findSpanForParameter: parameter ����� �� �������� ��������!");
            }

            if (curveParameter == nodalVector[numKnots - degree - 1])
            {
                return static_cast<int>(numKnots) - degree - 2;
            }

            int low = degree;
            int high = static_cast<int>(numKnots) - degree - 1;
            int middle = (low + high) / 2;

            while ((curveParameter < nodalVector[middle]) || (curveParameter >= nodalVector[middle + 1]))
            {
                if (curveParameter < nodalVector[middle])
                {
                    high = middle;
                }
                else
                {
                    low = middle;
                }

                middle = (low + high) / 2;
            }

            return middle;
        }

        // ��������� ������� �������� �������
        static void calculateZeroBasisFuncs(RGK::Vector<RGK::Vector<double>>& basisFuncs, RGK::Vector<RGK::Vector<double>>& tempStorage, DoubleArray& nodalVector, int degree, double curveParameter)
        {
            int span = findSpanForParameter(nodalVector, degree, curveParameter);

            RGK::Vector<double> left(degree + 1), right(degree + 1);
            tempStorage[0][0] = 1;

            for (int i = 1; i < degree + 1; ++i)
            {
                left[i] = curveParameter - nodalVector[span + 1 - i];
                right[i] = nodalVector[span + i] - curveParameter;
                double saved = 0;

                for (int j = 0; j < i; ++j)
                {
                    // ������ �����������
                    tempStorage[i][j] = right[j + 1] + left[i - j];
                    double temp = tempStorage[j][i - 1] / tempStorage[i][j];
                    // ������� �����������
                    tempStorage[j][i] = saved + right[j + 1] * temp;
                    saved = left[i - j] * temp;
                }

                tempStorage[i][i] = saved;
            }

            for (int i = 0; i <= degree; ++i)
            {
                basisFuncs[0][i] = tempStorage[i][degree];
            }
        }

        // ��������� ����������� �������� �������
        static void calcDerivsBasisFuncs(RGK::Vector<RGK::Vector<double>>& basisFuncs, RGK::Vector<RGK::Vector<double>>& tempStorage, int degree)
        {
            // ������ ��� ����������� ����
            RGK::Vector<RGK::Vector<double>> rows(2, RGK::Vector<double>(degree + 1));

            for (int i = 0; i < degree + 1; ++i)
            {
                int s1 = 0, s2 = 1;
                rows[0][0] = 1;

                for (int j = 1; j <= degree; ++j)
                {
                    double d = 0;
                    double rk = i - j;
                    double pk = degree - j;

                    if (i >= j)
                    {
                        rows[s2][0] = rows[s1][0] / tempStorage[pk + 1][rk];
                        d = rows[s2][0] * tempStorage[rk][pk];
                    }

                    double j1 = 0, j2 = 0;

                    if (rk >= -1)
                    {
                        j1 = 1;
                    }
                    else
                    {
                        j1 = -rk;
                    }

                    if (i - 1 <= pk)
                    {
                        j2 = j - 1;
                    }
                    else
                    {
                        j2 = degree - i;
                    }

                    for (int j = j1; j <= j2; ++j)
                    {
                        rows[s2][j] = (rows[s1][j] - rows[s1][j - 1]) / tempStorage[pk + 1][rk + j];
                        d += rows[s2][j] * tempStorage[rk + j][pk];
                    }

                    if (i <= pk)
                    {
                        rows[s2][j] = -rows[s1][j - 1] / tempStorage[pk + 1][i];
                        d += rows[s2][j] * tempStorage[i][pk];
                    }

                    basisFuncs[j][i] = d;
                    int temp = s1;
                    s1 = s2;
                    s2 = temp;
                }
            }

            double k = degree;

            // �������� �� ������������
            for (int i = 1; i <= degree; ++i)
            {
                for (int j = 0; j < degree + 1; ++j)
                {
                    basisFuncs[i][j] *= k;
                }

                k *= degree - i;
            }
        }

        // ��������� �������� ������� � �� �����������
        static RGK::Vector<RGK::Vector<double>> calculateBasisFuncs(const RGK::NURBSCurve& curve, double curveParameter)
        {
            int degree = curve.GetDegree();

            // ��� �������� �������� ������� � �� ����������� (������� ������ - ������� �����������, ������ ������ - ������ ������. � �.�.)
            RGK::Vector<RGK::Vector<double>> basisFuncs(degree + 1, RGK::Vector<double>(degree + 1));
            // ��� �������� �������� ������� � ����� ��������
            RGK::Vector<RGK::Vector<double>> tempStorage(degree + 1, RGK::Vector<double>(degree + 1));

            // ��������� ������� �������� �������
            calculateZeroBasisFuncs(basisFuncs, tempStorage, curve.GetKnots(), degree, curveParameter);
            // ��������� ��� ����������� �������� �������
            calcDerivsBasisFuncs(basisFuncs, tempStorage, degree);

            double sum = 0;

            // ��� �������� ��������� �������� �������� ������� � �����
            for (int i = 0; i < degree + 1; ++i)
            {
                sum += basisFuncs[0][i];
            }

            // ����� �������� ������� ������ = 1
            if (sum < (1 - 1e-10) || (sum > 1 + 1e-10))
            {
                throw _STR("Error! calculateBasisFuncs: c���� �������� ������� �� ����� 1!");
            }

            return basisFuncs;
        }

        // ��������� �������� ������� ������������� ��� � ������� ����������
        static void fillUpperTriangularCoefficientMatrix(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, RGK::Vector<RGK::Vector<double>>& basisFuncs, size_t numberEpsilons, size_t numberBreakPoints)
        {
            // ���������� �������� �������
            const size_t NUMBER_BASIS_FUNCS = basisFuncs.size();

            // ������ breakPoint - ���� �������� ���������� �������� ������� � coefficientMatrix
            for (size_t breakPointsCounter = 0; breakPointsCounter != numberBreakPoints; ++breakPointsCounter)
            {
                // ������ ������ ��� ��������������� ������ ������������
                size_t reverseRow = NUMBER_BASIS_FUNCS * 2 - 1 + NUMBER_BASIS_FUNCS * breakPointsCounter;
                size_t colBasisFunc = 0;

                // ����������� �� ������ ����� �������� �������
                for (size_t row = 0 + NUMBER_BASIS_FUNCS * breakPointsCounter; row != NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointsCounter; ++row)
                {
                    // ������ �������� �������
                    size_t rowBasisFunc = 0;
                    // ���������� �������� �������� ������� (��� ����������� ���������� ������������� � ������ ������ "+" ��� "-")
                    double prevBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];

                    for (size_t col = numberEpsilons + NUMBER_BASIS_FUNCS * breakPointsCounter; col != numberEpsilons + NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointsCounter; ++col)
                    {
                        double nextBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];
                        coefficientMatrix[row][col] = nextBasisFuncVal;

                        // ���������� ���� � ��������������� ����� �������� �������
                        if (prevBasisFuncVal < 0 && nextBasisFuncVal < 0)
                        {
                            nextBasisFuncVal *= -1;
                        }
                        else if (prevBasisFuncVal >= 0 && nextBasisFuncVal > 0)
                        {
                            nextBasisFuncVal *= -1;
                        }

                        coefficientMatrix[reverseRow][col] = nextBasisFuncVal;
                        prevBasisFuncVal = nextBasisFuncVal;
                        ++rowBasisFunc;
                    }

                    --reverseRow;
                    ++colBasisFunc;
                }
            }
        }

        // ��������� �������� ������� ������������� ��� � ������� ����������
        static void fillLowerTriangularCoefficientMatrix(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, RGK::Vector<RGK::Vector<double>>& basisFuncs, size_t numberEpsilons, size_t numberBreakPoints)
        {
            // ���������� �������� �������
            const size_t NUMBER_BASIS_FUNCS = basisFuncs.size();

            // ������ breakPoint - ���� �������� ���������� �������� ������� � coefficientMatrix
            for (size_t breakPointCounter = 0; breakPointCounter != numberBreakPoints; ++breakPointCounter)
            {
                size_t rowBasisFunc = 0;

                // ����������� �� ������ ����� �������� �������
                for (size_t row = numberEpsilons + NUMBER_BASIS_FUNCS * breakPointCounter; row != numberEpsilons + NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointCounter; ++row)
                {
                    // ������ ������� ��� ��������������� �����
                    size_t reverseCol = NUMBER_BASIS_FUNCS * 2 - 1 + NUMBER_BASIS_FUNCS * breakPointCounter;
                    size_t colBasisFunc = 0;
                    // ���������� �������� �������� ������� (��� ����������� ���������� ������������� � ������ ������ "+" ��� "-")
                    double prevBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];

                    for (size_t col = 0 + NUMBER_BASIS_FUNCS * breakPointCounter; col != NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointCounter; ++col)
                    {
                        double nextBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];
                        coefficientMatrix[row][col] = nextBasisFuncVal;

                        // ���������� ���� � ��������������� ����� �������� �������
                        if (prevBasisFuncVal < 0 && nextBasisFuncVal < 0 && col != NUMBER_BASIS_FUNCS * breakPointCounter)
                        {
                            nextBasisFuncVal *= -1;
                        }
                        else if (prevBasisFuncVal >= 0 && nextBasisFuncVal > 0)
                        {
                            nextBasisFuncVal *= -1;
                        }

                        prevBasisFuncVal = nextBasisFuncVal;
                        coefficientMatrix[row][reverseCol] = nextBasisFuncVal;
                        ++colBasisFunc;
                        --reverseCol;
                    }

                    ++rowBasisFunc;
                }
            }
        }

        // ���������� ������� �������������
        static void fillCoefficientsMatrix(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, RGK::Vector<RGK::Vector<double>>& basisFuncs, size_t numberEpsilons, size_t numberBreakPoints)
        {
            // ��������� �������� ������� ���������
            for (size_t i = 0; i != numberEpsilons; ++i)
            {
                coefficientMatrix[i][i] = 2;
            }

            // ��������� ������� ������������ ��������� ��������� ��� ������� ���������� � ��� ���
            fillUpperTriangularCoefficientMatrix(coefficientMatrix, basisFuncs, numberEpsilons, numberBreakPoints);
            fillLowerTriangularCoefficientMatrix(coefficientMatrix, basisFuncs, numberEpsilons, numberBreakPoints);
        }

        // ��������� ������ � ��������� ����� ������ � �� ������ ����������� (��������� ������������ �������� � ����� � ������� �������������). �� ��������� ����������� ��� 4 �����
        static void fixPointsAtCurve(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, size_t numberEpsilons, size_t numberBasisFuncs, bool fixBeginningCurve, bool fixEndCurve)
        {
            int orderFixFirstDeriv = 1;
            int orderFixLastDeriv = 1;

            if (fixBeginningCurve)
            {
                // �������� ������ ��������� ����� ������
                while (orderFixFirstDeriv >= 0)
                {
                    for (size_t row = numberEpsilons; row != numberEpsilons + numberBasisFuncs; ++row)
                    {
                        coefficientMatrix[orderFixFirstDeriv][row] = 0;
                    }

                    --orderFixFirstDeriv;
                }
            }

            if (fixEndCurve)
            {
                for (size_t col = numberEpsilons + numberBasisFuncs; col != coefficientMatrix.size(); ++col)
                {
                    coefficientMatrix[numberEpsilons - 1][col] = 0;
                }
            }


            // �� �������� ��� 1 ������� �����������
            //int tempCounter = 1;

            //while (orderFixLastDeriv >= 0)
            //{
            //    for (size_t col = numberEpsilons; col != numberEpsilons + numberBasisFuncs; ++col)
            //    {
            //        coefficientMatrix[numberEpsilons - tempCounter][col] = 0;
            //    }

            //    ++tempCounter;
            //    --orderFixLastDeriv;
            //}
        }

        // ��������� ������� ��������� ������
        static void fillFreeMemberMatrix(RGK::Vector<RGK::Vector<double>>& freeMembersMatrix, const RGK::Vector<RGK::Math::Vector3DArray>& controlPointsBezierCurves, RGK::Vector<RGK::Vector<double>>& basisFuncs, RGK::Vector<RGK::Vector<double>>& reverseBasisFuncs, size_t numberEpsilons)
        {
            size_t indexFreeMembers = numberEpsilons;

            for (size_t row = 0; row != controlPointsBezierCurves.size() - 1; ++row)
            {
                size_t rowBasisFunc = 0;

                for (size_t col = 0; col != basisFuncs[0].size(); ++col)
                {
                    for (size_t i = 0; i != controlPointsBezierCurves[0].size(); ++i)
                    {
                        // ������� ������
                        freeMembersMatrix[indexFreeMembers][0] += controlPointsBezierCurves[row][i].GetX() * -basisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][1] += controlPointsBezierCurves[row][i].GetY() * -basisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][2] += controlPointsBezierCurves[row][i].GetZ() * -basisFuncs[rowBasisFunc][i];
                        // ��������� ������
                        freeMembersMatrix[indexFreeMembers][0] += controlPointsBezierCurves[row + 1][i].GetX() * reverseBasisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][1] += controlPointsBezierCurves[row + 1][i].GetY() * reverseBasisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][2] += controlPointsBezierCurves[row + 1][i].GetZ() * reverseBasisFuncs[rowBasisFunc][i];
                    }

                    ++rowBasisFunc;
                    ++indexFreeMembers;
                }
            }
        }

        // ��������� ����� ������ ��� ������� ���������� ������
        static RGK::Vector<RGK::Vector<double>> calculateShiftPoints(const RGK::Vector<RGK::Vector<double>>& coefficientMatrix, const RGK::Vector<RGK::Vector<double>>& freeMembersMatrix)
        {
            // ������ ��������� �� ��������� �������� ����
            auto operation = IMatrixOperations::GetMatrixOperationsClass(OperationClass::eigen);

            if (operation == nullptr)
            {
                throw "Error! conjugateCurve: operation = nullptr";
            }

            // ��������� ������������ ������� �������������
            double coefficientMatrixDet = operation->getMatrixDet(coefficientMatrix);

            if (coefficientMatrixDet == 0)
            {
                throw "Error! ������������ ������� ������������� = 0! ��������, �������� ������ ������������� ����� � ������� fixPointsAtCurve!";
            }

            // ������ ���� � ���������� �����
            return operation->solveEquation(coefficientMatrix, freeMembersMatrix);
        }

        // ���������� ���������� ����� ����� ������ ��� ����������
        static void correctionPoints(RGK::Vector<RGK::Math::Vector3DArray>& controlPointsBezierCurves, RGK::Vector<RGK::Vector<double>>& shiftPoints, size_t numberBezierCurves)
        {
            int tempCounter = 0;

            for (size_t i = 0; i != numberBezierCurves; ++i)
            {
                for (size_t j = 0; j != controlPointsBezierCurves[i].size(); ++j)
                {
                    double x = controlPointsBezierCurves[i][j].GetX() + shiftPoints[tempCounter][0];
                    double y = controlPointsBezierCurves[i][j].GetY() + shiftPoints[tempCounter][1];
                    double z = controlPointsBezierCurves[i][j].GetZ() + shiftPoints[tempCounter][2];
                    controlPointsBezierCurves[i][j].SetXYZ(x, y, z);
                    ++tempCounter;
                }
            }
        }

        // ������� ������ ������ ����� �� ��������� ������� ����������� ��������������� ������ �����
        static RGK::Vector<RGK::NURBSCurve> createBezierCurves(RGK::Vector<RGK::Math::Vector3DArray>& controlPointsBezierCurves, size_t numberBezierCurves, int degree)
        {
            RGK::Vector<RGK::NURBSCurve> newBezierCurves;
            RGK::NURBSCurve tempBezierCurve;

            for (size_t i = 0; i != numberBezierCurves; ++i)
            {
                // TODO! �� ����, ����� �� ������ ��� ������������� rgkContext � ����������� ��� � NURBSCurve::CreateBezier...
                RGK::Context rgkContext;
                RPLM::EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

                // ������ ����� ������ ����� � ��������� � ������, ����� ������� ���������� ���
                RGK::NURBSCurve::CreateBezier(rgkContext, controlPointsBezierCurves[i], degree, tempBezierCurve);
                newBezierCurves.push_back(tempBezierCurve);
            }

            return newBezierCurves;
        }

        // ���������� ��������� ������� ������
        static DoubleArray fillEvenlyNodalVector(int degree, int numVertices)
        {
            // ���-�� ����� (�����) �������� ����� �������� �������
            int _numRealRangeKnots = numVertices - degree + 1;
            // ���-�� ����� � ���. �������
            int numKnots = numVertices + degree + 1;

            // ������/����� ��������� ��������� ���. �������
            int realRangeStart = degree;
            int realRangeEnd = numKnots - degree - 1;

            DoubleArray knots(numKnots);

            // ��� � �������� ���������
            double step = 1 / static_cast<double>(_numRealRangeKnots - 1);

            // ��������� �������� ��������
            for (int i = realRangeStart + 1; i < realRangeEnd; ++i)
                knots[i] = knots[i - 1] + step;

            // ��������� ��������� ��������� ���������
            for (size_t i = realRangeEnd; i < knots.size(); ++i)
                knots[i] = 1;

            return knots;
        }

        // ��������� ������ ������ ����� � ���� ������ NURBS
        static RGK::NURBSCurve bezierCurvesToNURBSCurve(const RGK::Vector<RGK::NURBSCurve>& bezierCurves, int degree)
        {
            RGK::Vector<RGK::Math::Vector3D> newControlPoints;
            // ��� ����, ����� �� ���� ������������� �����
            bool firstCheck = false;

            for (size_t curveCount = 0; curveCount != bezierCurves.size(); ++curveCount)
            {
                RGK::Vector<RGK::Math::Vector3D> tempControlPoints = bezierCurves[curveCount].GetControlPoints();

                for (size_t i = 0; i != tempControlPoints.size(); ++i)
                {
                    if (firstCheck && i == 0)
                    {
                        continue;
                    }
                    if (curveCount == 0)
                    {
                        firstCheck = true;
                    }

                    newControlPoints.push_back(tempControlPoints[i]);
                }
            }
            // TODO! �� ����, ����� �� ������ ��� ������������� rgkContext � ����������� ��� � NURBSCurve::CreateBezier...
            RGK::Context rgkContext;
            RPLM::EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

            DoubleArray knots = fillEvenlyNodalVector(degree, static_cast<int>(newControlPoints.size()));

            RGK::NURBSCurve newCurve;
            // ������ ����� ������ ����� � ��������� � ������, ����� ������� ���������� ���
            RGK::NURBSCurve::Create(rgkContext, newControlPoints, degree, knots, false, newCurve);

            return newCurve;
        }
    }

    RGK::NURBSCurve ConjugationMethods::conjugateCurve(const RGK::NURBSCurve& iCurve, bool fixBeginningCurve = false, bool fixEndCurve = false)
    {
        // ��������� NURBS ������ �� ������ �����
        RGK::Vector<RGK::NURBSCurve> bezierCurves = ImplConjugateCurve::splitting�urveIntoBezierCurves(iCurve);

        // ��������� �������� ������� � �� ����������� � ��������� 1 (������� ������ � basisFuncs - ������� �����������, ������ ������ � basisFuncs - ������ ������. � �.�.)
        double curveParameter = 1;
        RGK::Vector<RGK::Vector<double>> basisFuncs = ImplConjugateCurve::calculateBasisFuncs(bezierCurves[0], curveParameter);

        const size_t NUMBER_BASIS_FUNCS = static_cast<size_t>(iCurve.GetDegree()) + 1;                      // ���������� �������� �������
        const size_t NUMBER_BEZIER_CURVES = bezierCurves.size();                                            // ���������� ������ �����
        const size_t NUMBER_BREAK_POINTS = NUMBER_BEZIER_CURVES - 1;                                        // ���������� ������������� ����� ������� ����� �������
        const size_t NUMBER_EPSILONS = bezierCurves.size() * bezierCurves[0].GetControlPoints().size();     // ���������� �������, ������� ����� ������������ ����������� �����
        const size_t MATRIX_SIZE = NUMBER_BASIS_FUNCS * (2 * NUMBER_BEZIER_CURVES - 1);  // ������ ������� �������������

        // ������� �������������
        RGK::Vector<RGK::Vector<double>> coefficientMatrix(MATRIX_SIZE, RGK::Vector<double>(MATRIX_SIZE));

        // ��������� ������� �������������
        ImplConjugateCurve::fillCoefficientsMatrix(coefficientMatrix, basisFuncs, NUMBER_EPSILONS, NUMBER_BREAK_POINTS);

        // ��������� ������ � ��������� ����� � ������ � �� ������ �����������
        ImplConjugateCurve::fixPointsAtCurve(coefficientMatrix, NUMBER_EPSILONS, NUMBER_BASIS_FUNCS, fixBeginningCurve, fixEndCurve);

        // ����������� ����� ������ �����
        RGK::Vector<RGK::Math::Vector3DArray> controlPointsBezierCurves(NUMBER_BEZIER_CURVES);

        for (size_t i = 0; i != NUMBER_BEZIER_CURVES; ++i)
        {
            controlPointsBezierCurves[i] = bezierCurves[i].GetControlPoints();
        }

        // ������� ��������� ������. RGK::Vector<double>(3) - ������ ��� 3 ���������� x, y, z. ����� �������� � ����������, ���� ������ ��� ������ ��� �����
        RGK::Vector<RGK::Vector<double>> freeMembersMatrix(MATRIX_SIZE, RGK::Vector<double>(3));

        // ��������� ����������� �������� ������� � �� ����������� � ��������� 0
        curveParameter = 0;
        RGK::Vector<RGK::Vector<double>> reverseBasisFuncs = ImplConjugateCurve::calculateBasisFuncs(bezierCurves[0], curveParameter);

        // ��������� ������� ��������� ������
        ImplConjugateCurve::fillFreeMemberMatrix(freeMembersMatrix, controlPointsBezierCurves, basisFuncs, reverseBasisFuncs, NUMBER_EPSILONS);

        // ��������� ����� �������� ��� ����� ����������� �����
        RGK::Vector<RGK::Vector<double>> shiftPoints = ImplConjugateCurve::calculateShiftPoints(coefficientMatrix, freeMembersMatrix);

        // ������ ����� �������� ����������� ����� ��� ����������
        ImplConjugateCurve::correctionPoints(controlPointsBezierCurves, shiftPoints, NUMBER_BEZIER_CURVES);

        // ��������� ����� ������ �����, ������� ����� ����������
        RGK::Vector<RGK::NURBSCurve> newBezierCurves = ImplConjugateCurve::createBezierCurves(controlPointsBezierCurves, NUMBER_BEZIER_CURVES, bezierCurves[0].GetDegree());

        // ������������ ������ ������ ����� ��� ���� ������ NURBS
        RGK::NURBSCurve merdgedCurve = ImplConjugateCurve::bezierCurvesToNURBSCurve(newBezierCurves, bezierCurves[0].GetDegree());

        return merdgedCurve;
    }
}