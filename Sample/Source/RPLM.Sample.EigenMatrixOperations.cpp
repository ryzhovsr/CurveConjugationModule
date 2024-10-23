#include "RPLM.Sample.EigenMatrixOperations.h"

// В ручную привязываю библиотеку Eigen. Изменить если узнать как сделать динамический путь
#include "Eigen\Dense"

using namespace Eigen;

// Переводит двумерный вектор в матрицу класса Eigen
inline MatrixXd vector2DToMatrix(const IMatrixOperations::vector2D &vec2D)
{
    size_t rows = vec2D.size(), cols = vec2D[0].size();
    MatrixXd matrix = MatrixXd::Constant(rows, cols, 0);

    for (size_t i = 0; i < rows; ++i)
    {
        for (size_t j = 0; j < cols; ++j)
        {
            matrix(i, j) = vec2D[i][j];
        }
    }

    return matrix;
}

// Переводит матрицу класса Eigen в двумерный вектор
inline IMatrixOperations::vector2D matrixToVector2D(const MatrixXd &matrix)
{
    auto rows = matrix.rows(), cols = matrix.cols();
    IMatrixOperations::vector2D vec2D(rows, std::vector<double>(cols));

    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            vec2D[i][j] = matrix(i, j);
        }
    }

    return vec2D;
}

IMatrixOperations::vector2D EigenMatrixOperations::solveEquation(const vector2D &coefficients, const vector2D &freeMembers)
{
    // Переводим двумерные векторы в матрицу класса Eigen
    MatrixXd coefficientMatrix = vector2DToMatrix(coefficients);
    MatrixXd freeTermMatrix = vector2DToMatrix(freeMembers);

    // Решаем СЛАУ
    Eigen::MatrixXd decisionMatrix = Eigen::MatrixXd::Constant(freeMembers.size(), freeMembers[0].size(), 0);
    decisionMatrix = coefficientMatrix.lu().solve(freeTermMatrix);

    // Revert convertion
    vector2D decisionVector2D = matrixToVector2D(decisionMatrix);

    return decisionVector2D;
}

double EigenMatrixOperations::getMatrixDet(const vector2D &vec2D)
{
    MatrixXd matrix = vector2DToMatrix(vec2D);
    return matrix.determinant();
}

int EigenMatrixOperations::getMatrixRank(const vector2D &matrix)
{
    MatrixXd m = vector2DToMatrix(matrix);
    // Используем LU-разложение
    FullPivLU<MatrixXd> lu_decomp(m);
    return static_cast<int>(lu_decomp.rank());
}
