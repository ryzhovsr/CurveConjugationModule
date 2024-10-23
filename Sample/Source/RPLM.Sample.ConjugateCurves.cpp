#include "RPLM.Sample.ConjugateCurves.h"
#include <RGPSession.h>
#include "RPLM.Sample.IMatrixOperations.h"
#include "RPLM.Math.ConstraintSolver.EquationSolver/EquationsMatrix.h"

namespace Sample
{
    // Пространсто со вспомогательным функциями для сопряжения кривой
    namespace ImplConjugateCurve
    {
        using DoubleArray = RPLM::Math::Geometry2D::Geometry::DoubleArray;

        // Разбивает NURBS кривую на кривые Безье
        static RGK::Vector<RGK::NURBSCurve> splittingСurveIntoBezierCurves(const RGK::NURBSCurve& originalCurve)
        {
            // Контрольные точки оригинальной кривой
            RGK::Vector<RGK::Math::Vector3D> controlPointsOriginalCurve = originalCurve.GetControlPoints();
            int degree = originalCurve.GetDegree();

            // Число кривых Безье, на которое будем делить originalCurve
            size_t numberBezierCurves = controlPointsOriginalCurve.size() / degree;
            RGK::Vector<RGK::NURBSCurve> bezierCurves(numberBezierCurves);
            RGK::NURBSCurve tempBezierCurve;

            for (size_t i = 0; i != numberBezierCurves; ++i)
            {
                RGK::Vector<RGK::Math::Vector3D> tempControlPoints;

                // Добавляем по частям контрольные точки оригинальной кривой для каждой отдельной Безье кривой
                for (size_t j = 0; j != static_cast<size_t>(degree) + 1; ++j)
                {
                    tempControlPoints.push_back(controlPointsOriginalCurve[j + i * degree]);
                }

                // TODO! Не знаю, нужно ли каждый раз пересоздавать rgkContext и прописывать его в NURBSCurve::CreateBezier...
                RGK::Context rgkContext;
                RPLM::EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

                RGK::NURBSCurve::CreateBezier(rgkContext, tempControlPoints, degree, tempBezierCurve);
                bezierCurves[i] = tempBezierCurve;
            }

            return bezierCurves;
        }

        // Находит номер-интервал (спан) для заданного curveParameter
        static int findSpanForParameter(const std::vector<double>& nodalVector, int degree, double curveParameter)
        {
            size_t numKnots = nodalVector.size();

            if (curveParameter < nodalVector[degree] || curveParameter > nodalVector[numKnots - degree - 1])
            {
                throw _STR("Error! findSpanForParameter: parameter вышел за реальный диапазон!");
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

        // Вычисляет нулевые базисные функции
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
                    // Нижний треугольник
                    tempStorage[i][j] = right[j + 1] + left[i - j];
                    double temp = tempStorage[j][i - 1] / tempStorage[i][j];
                    // Верхний треугольник
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

        // Вычисляет производные базисных функций
        static void calcDerivsBasisFuncs(RGK::Vector<RGK::Vector<double>>& basisFuncs, RGK::Vector<RGK::Vector<double>>& tempStorage, int degree)
        {
            // Хранит два вычесленных ряда
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

            // Умножаем на коэффициенты
            for (int i = 1; i <= degree; ++i)
            {
                for (int j = 0; j < degree + 1; ++j)
                {
                    basisFuncs[i][j] *= k;
                }

                k *= degree - i;
            }
        }

        // Вычисляет базисные функции и их производные
        static RGK::Vector<RGK::Vector<double>> calculateBasisFuncs(const RGK::NURBSCurve& curve, double curveParameter)
        {
            int degree = curve.GetDegree();

            // Для хранения базисных функций и их производных (нулевая строка - нулевые производные, первая строка - первые произв. и т.д.)
            RGK::Vector<RGK::Vector<double>> basisFuncs(degree + 1, RGK::Vector<double>(degree + 1));
            // Для хранения базисных функций и узлов различия
            RGK::Vector<RGK::Vector<double>> tempStorage(degree + 1, RGK::Vector<double>(degree + 1));

            // Вычисляем нулевые базисные функции
            calculateZeroBasisFuncs(basisFuncs, tempStorage, curve.GetKnots(), degree, curveParameter);
            // Вычисляем все производные базисных функций
            calcDerivsBasisFuncs(basisFuncs, tempStorage, degree);

            double sum = 0;

            // Для контроля суммируем значения базисных функций в точке
            for (int i = 0; i < degree + 1; ++i)
            {
                sum += basisFuncs[0][i];
            }

            // Сумма базисных функций должна = 1
            if (sum < (1 - 1e-10) || (sum > 1 + 1e-10))
            {
                throw _STR("Error! calculateBasisFuncs: cумма базисных функций не равна 1!");
            }

            return basisFuncs;
        }

        // Заполняет элементы матрицы коээфициентов над её главной диагональю
        static void fillUpperTriangularCoefficientMatrix(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, RGK::Vector<RGK::Vector<double>>& basisFuncs, size_t numberEpsilons, size_t numberBreakPoints)
        {
            // Количество базисных функций
            const size_t NUMBER_BASIS_FUNCS = basisFuncs.size();

            // Каждый breakPoint - одна итерация заполнения базисных функций в coefficientMatrix
            for (size_t breakPointsCounter = 0; breakPointsCounter != numberBreakPoints; ++breakPointsCounter)
            {
                // Реверс строка для противоположной сторны треугольника
                size_t reverseRow = NUMBER_BASIS_FUNCS * 2 - 1 + NUMBER_BASIS_FUNCS * breakPointsCounter;
                size_t colBasisFunc = 0;

                // Итерируемся по общему числу базисных функций
                for (size_t row = 0 + NUMBER_BASIS_FUNCS * breakPointsCounter; row != NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointsCounter; ++row)
                {
                    // Строка базисных функций
                    size_t rowBasisFunc = 0;
                    // Предыдущее значение базисной функции (для правильного заполнения коэффициентов с нужным знаком "+" или "-")
                    double prevBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];

                    for (size_t col = numberEpsilons + NUMBER_BASIS_FUNCS * breakPointsCounter; col != numberEpsilons + NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointsCounter; ++col)
                    {
                        double nextBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];
                        coefficientMatrix[row][col] = nextBasisFuncVal;

                        // Регулируем знак у противоположной части базисных функций
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

        // Заполняет элементы матрицы коээфициентов под её главной диагональю
        static void fillLowerTriangularCoefficientMatrix(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, RGK::Vector<RGK::Vector<double>>& basisFuncs, size_t numberEpsilons, size_t numberBreakPoints)
        {
            // Количество базисных функций
            const size_t NUMBER_BASIS_FUNCS = basisFuncs.size();

            // Каждый breakPoint - одна итерация заполнения базисных функций в coefficientMatrix
            for (size_t breakPointCounter = 0; breakPointCounter != numberBreakPoints; ++breakPointCounter)
            {
                size_t rowBasisFunc = 0;

                // Итерируемся по общему числу базисных функций
                for (size_t row = numberEpsilons + NUMBER_BASIS_FUNCS * breakPointCounter; row != numberEpsilons + NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointCounter; ++row)
                {
                    // Реверс столбец для противоположной части
                    size_t reverseCol = NUMBER_BASIS_FUNCS * 2 - 1 + NUMBER_BASIS_FUNCS * breakPointCounter;
                    size_t colBasisFunc = 0;
                    // Предыдущее значение базисной функции (для правильного заполнения коэффициентов с нужным знаком "+" или "-")
                    double prevBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];

                    for (size_t col = 0 + NUMBER_BASIS_FUNCS * breakPointCounter; col != NUMBER_BASIS_FUNCS + NUMBER_BASIS_FUNCS * breakPointCounter; ++col)
                    {
                        double nextBasisFuncVal = basisFuncs[rowBasisFunc][colBasisFunc];
                        coefficientMatrix[row][col] = nextBasisFuncVal;

                        // Регулируем знак у противоположной части базисных функций
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

        // Заполнение матрицы коэффициентов
        static void fillCoefficientsMatrix(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, RGK::Vector<RGK::Vector<double>>& basisFuncs, size_t numberEpsilons, size_t numberBreakPoints)
        {
            // Заполняем двойками главную диагональ
            for (size_t i = 0; i != numberEpsilons; ++i)
            {
                coefficientMatrix[i][i] = 2;
            }

            // Заполняем матрицу коэффциентов базисными функциями над главной диагональю и под ней
            fillUpperTriangularCoefficientMatrix(coefficientMatrix, basisFuncs, numberEpsilons, numberBreakPoints);
            fillLowerTriangularCoefficientMatrix(coefficientMatrix, basisFuncs, numberEpsilons, numberBreakPoints);
        }

        // Фиксирует первую и последнюю точки кривой и их первые производные (зануление определенных столбцов и строк у матрицы коэффициентов). По умолчанию фиксируются все 4 точки
        static void fixPointsAtCurve(RGK::Vector<RGK::Vector<double>>& coefficientMatrix, size_t numberEpsilons, size_t numberBasisFuncs, bool fixBeginningCurve, bool fixEndCurve)
        {
            int orderFixFirstDeriv = 1;
            int orderFixLastDeriv = 1;

            if (fixBeginningCurve)
            {
                // Фиксация первой граничной точки кривой
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


            // Не работает для 1 порядка производной
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

        // Заполняет матрицу свободных членов
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
                        // Текущая кривая
                        freeMembersMatrix[indexFreeMembers][0] += controlPointsBezierCurves[row][i].GetX() * -basisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][1] += controlPointsBezierCurves[row][i].GetY() * -basisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][2] += controlPointsBezierCurves[row][i].GetZ() * -basisFuncs[rowBasisFunc][i];
                        // Следующая кривая
                        freeMembersMatrix[indexFreeMembers][0] += controlPointsBezierCurves[row + 1][i].GetX() * reverseBasisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][1] += controlPointsBezierCurves[row + 1][i].GetY() * reverseBasisFuncs[rowBasisFunc][i];
                        freeMembersMatrix[indexFreeMembers][2] += controlPointsBezierCurves[row + 1][i].GetZ() * reverseBasisFuncs[rowBasisFunc][i];
                    }

                    ++rowBasisFunc;
                    ++indexFreeMembers;
                }
            }
        }

        // Вычисляет точки сдвига для полного сопряжения кривой
        static RGK::Vector<RGK::Vector<double>> calculateShiftPoints(const RGK::Vector<RGK::Vector<double>>& coefficientMatrix, const RGK::Vector<RGK::Vector<double>>& freeMembersMatrix)
        {
            // Создаём указатель на интерфейс операций СЛАУ
            auto operation = IMatrixOperations::GetMatrixOperationsClass(OperationClass::eigen);

            if (operation == nullptr)
            {
                throw "Error! conjugateCurve: operation = nullptr";
            }

            // Вычисляем определитель матрицы коэффициентов
            double coefficientMatrixDet = operation->getMatrixDet(coefficientMatrix);

            if (coefficientMatrixDet == 0)
            {
                throw "Error! Определитель матрицы коэффициентов = 0! Возможно, сделайте меньше фиксированных точек в функции fixPointsAtCurve!";
            }

            // Решаем СЛАУ и возвращаем ответ
            return operation->solveEquation(coefficientMatrix, freeMembersMatrix);
        }

        // Регулирует котрольные точки Безье кривых для сопряжения
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

        // Создает вектор кривых Безье из заданного вектора контрольных многоугольников кривых Безье
        static RGK::Vector<RGK::NURBSCurve> createBezierCurves(RGK::Vector<RGK::Math::Vector3DArray>& controlPointsBezierCurves, size_t numberBezierCurves, int degree)
        {
            RGK::Vector<RGK::NURBSCurve> newBezierCurves;
            RGK::NURBSCurve tempBezierCurve;

            for (size_t i = 0; i != numberBezierCurves; ++i)
            {
                // TODO! Не знаю, нужно ли каждый раз пересоздавать rgkContext и прописывать его в NURBSCurve::CreateBezier...
                RGK::Context rgkContext;
                RPLM::EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

                // Создаём новую кривую Безье и добавляем в вектор, чтобы функция возвратила его
                RGK::NURBSCurve::CreateBezier(rgkContext, controlPointsBezierCurves[i], degree, tempBezierCurve);
                newBezierCurves.push_back(tempBezierCurve);
            }

            return newBezierCurves;
        }

        // Равномерно заполняет узловой вектор
        static DoubleArray fillEvenlyNodalVector(int degree, int numVertices)
        {
            // Кол-во узлов (длина) реальной части узлового вектора
            int _numRealRangeKnots = numVertices - degree + 1;
            // Кол-во узлов в узл. векторе
            int numKnots = numVertices + degree + 1;

            // Начало/конец реального диапазона узл. вектора
            int realRangeStart = degree;
            int realRangeEnd = numKnots - degree - 1;

            DoubleArray knots(numKnots);

            // Шаг в реальном диапазоне
            double step = 1 / static_cast<double>(_numRealRangeKnots - 1);

            // Заполняем реальный диапазон
            for (int i = realRangeStart + 1; i < realRangeEnd; ++i)
                knots[i] = knots[i - 1] + step;

            // Заполняем последние параметры единицами
            for (size_t i = realRangeEnd; i < knots.size(); ++i)
                knots[i] = 1;

            return knots;
        }

        // Переводит вектор кривых Безье в одну кривую NURBS
        static RGK::NURBSCurve bezierCurvesToNURBSCurve(const RGK::Vector<RGK::NURBSCurve>& bezierCurves, int degree)
        {
            RGK::Vector<RGK::Math::Vector3D> newControlPoints;
            // Для того, чтобы не было повторяющихся точек
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
            // TODO! Не знаю, нужно ли каждый раз пересоздавать rgkContext и прописывать его в NURBSCurve::CreateBezier...
            RGK::Context rgkContext;
            RPLM::EP::Model::Session::GetSession()->GetRGKSession().CreateMainContext(rgkContext);

            DoubleArray knots = fillEvenlyNodalVector(degree, static_cast<int>(newControlPoints.size()));

            RGK::NURBSCurve newCurve;
            // Создаём новую кривую Безье и добавляем в вектор, чтобы функция возвратила его
            RGK::NURBSCurve::Create(rgkContext, newControlPoints, degree, knots, false, newCurve);

            return newCurve;
        }
    }

    RGK::NURBSCurve ConjugationMethods::conjugateCurve(const RGK::NURBSCurve& iCurve, bool fixBeginningCurve = false, bool fixEndCurve = false)
    {
        // Разбиваем NURBS кривую на кривые Безье
        RGK::Vector<RGK::NURBSCurve> bezierCurves = ImplConjugateCurve::splittingСurveIntoBezierCurves(iCurve);

        // Вычисляем базисные функции и их производные в параметре 1 (нулевая строка в basisFuncs - нулевые производные, первая строка в basisFuncs - первые произв. и т.д.)
        double curveParameter = 1;
        RGK::Vector<RGK::Vector<double>> basisFuncs = ImplConjugateCurve::calculateBasisFuncs(bezierCurves[0], curveParameter);

        const size_t NUMBER_BASIS_FUNCS = static_cast<size_t>(iCurve.GetDegree()) + 1;                      // Количество базисных функций
        const size_t NUMBER_BEZIER_CURVES = bezierCurves.size();                                            // Количество кривых Безье
        const size_t NUMBER_BREAK_POINTS = NUMBER_BEZIER_CURVES - 1;                                        // Количество потенциальных точек разрыва между кривыми
        const size_t NUMBER_EPSILONS = bezierCurves.size() * bezierCurves[0].GetControlPoints().size();     // Количество эпсилон, которые будут регулировать контрольные точки
        const size_t MATRIX_SIZE = NUMBER_BASIS_FUNCS * (2 * NUMBER_BEZIER_CURVES - 1);  // Размер матрицы коэффициентов

        // Матрица коэффициентов
        RGK::Vector<RGK::Vector<double>> coefficientMatrix(MATRIX_SIZE, RGK::Vector<double>(MATRIX_SIZE));

        // Заполняем матрицу коэффициентов
        ImplConjugateCurve::fillCoefficientsMatrix(coefficientMatrix, basisFuncs, NUMBER_EPSILONS, NUMBER_BREAK_POINTS);

        // Фиксируем первую и последнюю точки у кривой и их первые производные
        ImplConjugateCurve::fixPointsAtCurve(coefficientMatrix, NUMBER_EPSILONS, NUMBER_BASIS_FUNCS, fixBeginningCurve, fixEndCurve);

        // Контрольные точки кривых Безье
        RGK::Vector<RGK::Math::Vector3DArray> controlPointsBezierCurves(NUMBER_BEZIER_CURVES);

        for (size_t i = 0; i != NUMBER_BEZIER_CURVES; ++i)
        {
            controlPointsBezierCurves[i] = bezierCurves[i].GetControlPoints();
        }

        // Матрица свободных членов. RGK::Vector<double>(3) - потому что 3 координаты x, y, z. Можно испрвить в дальнейшем, если узнать тип данных для точки
        RGK::Vector<RGK::Vector<double>> freeMembersMatrix(MATRIX_SIZE, RGK::Vector<double>(3));

        // Вычисляем реверсивные базисные функции и их производные в параметре 0
        curveParameter = 0;
        RGK::Vector<RGK::Vector<double>> reverseBasisFuncs = ImplConjugateCurve::calculateBasisFuncs(bezierCurves[0], curveParameter);

        // Заполняем матрицу свободных членов
        ImplConjugateCurve::fillFreeMemberMatrix(freeMembersMatrix, controlPointsBezierCurves, basisFuncs, reverseBasisFuncs, NUMBER_EPSILONS);

        // Вычисляем точки смещения для новых контрольных точек
        RGK::Vector<RGK::Vector<double>> shiftPoints = ImplConjugateCurve::calculateShiftPoints(coefficientMatrix, freeMembersMatrix);

        // Делаем сдивг исходных контрольных точек для сопряжения
        ImplConjugateCurve::correctionPoints(controlPointsBezierCurves, shiftPoints, NUMBER_BEZIER_CURVES);

        // Вычисляем новые кривые Безье, которые будут непрерывны
        RGK::Vector<RGK::NURBSCurve> newBezierCurves = ImplConjugateCurve::createBezierCurves(controlPointsBezierCurves, NUMBER_BEZIER_CURVES, bezierCurves[0].GetDegree());

        // Представляем вектор кривых Безье как одну кривую NURBS
        RGK::NURBSCurve merdgedCurve = ImplConjugateCurve::bezierCurvesToNURBSCurve(newBezierCurves, bezierCurves[0].GetDegree());

        return merdgedCurve;
    }
}