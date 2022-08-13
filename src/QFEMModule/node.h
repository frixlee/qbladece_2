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
#ifndef NODE_H
#define NODE_H

#include <vector>
#include <stdlib.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>
using Eigen::Vector4d;
/*!
 * The Node Class is a way to storing the node identifier, its degrees of freedom
 * and its coordinates
 *
 */
class Node
{

public:

    Node();
    int IDnumber; /**< ID Identifier */
    std::vector<int> DOFNums;/**< Vectors of integers which match the DOF numbers */
    Vector4d Coords;/**< Coordinates initialized to 0,0,0,1 in the constructor */
    void PrintCoords();
    Node Subtract(Node VectorB);
};

#endif // NODE_H
