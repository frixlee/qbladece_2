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

#include "deformationvector.h"

DeformationVector::DeformationVector()

{
    Coords.setZero();
    xAxisAngle=0;
    yAxisAngle=0;
    zAxisAngle=0;

}

DeformationVector DeformationVector::Subtract(DeformationVector VectorB)
{
    //Just create difference between two vectors for Strain Calcs
    //Should use as DeformationVector Diff = VectorA.Substract(VectorB);
    Vector4d DiffVec = Coords - VectorB.Coords;
    DeformationVector DiffNode;
    DiffNode.Coords = DiffVec;
    DiffNode.xAxisAngle = xAxisAngle - VectorB.xAxisAngle;
    DiffNode.yAxisAngle = yAxisAngle - VectorB.yAxisAngle;
    DiffNode.zAxisAngle = zAxisAngle - VectorB.zAxisAngle;
    return DiffNode;


}
