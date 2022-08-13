/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

    This program is licensed under the Academic Public License
    (APL) v1.0; You can use, redistribute and/or modify it in
    non-commercial academic environments under the terms of the
    APL as published by the QBlade project; See the file 'LICENSE'
    for details; Commercial use requires a commercial license
    (contact info@qblade.org).

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

***********************************************************************/
#define EIGEN_DONT_VECTORIZE
#define EIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT
#ifndef MODE_H
#define MODE_H

#include <Eigen/Dense>
#include "taperedelem.h"
#include "node.h"
#include "unitvector.h"
#include "deformationvector.h"
#include <QVector>
using Eigen::MatrixXd;
using Eigen::Matrix2f;
using Eigen::MatrixXcd;
enum ModeTypesEnum{Flap, Edge, Torsion, Radial, UnSorted};
class Mode
{
public:
    Mode();

    MatrixXd Polynomial;
    double Frequency;
    QVector<DeformationVector> ModeShape;
    ModeTypesEnum ModeType;

    void PrintMode();


};

#endif // MODE_H
