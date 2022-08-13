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

#ifndef DEFORMATIONVECTOR_H
#define DEFORMATIONVECTOR_H
#include "node.h"
class DeformationVector : public Node
{
public:
    DeformationVector();
    double zAxisAngle;
    double xAxisAngle;
    double yAxisAngle;
    DeformationVector Subtract(DeformationVector VectorB);

};

#endif // DEFORMATIONVECTOR_H
