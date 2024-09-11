#pragma once

#include "RPLM.Sample.IMatrixOperations.h"

class EigenMatrixOperations final : public IMatrixOperations
{
public:
    // Решает СЛАУ
    vector2D solveEquation(const vector2D &coefficients, const vector2D &freeMembers) override;

    // Возвращает ранг матрицы
    int getMatrixRank(const vector2D &matrix) override;

    // Возвращяет определитель матрицы
    double getMatrixDet(const vector2D &vec2D) override;
};
