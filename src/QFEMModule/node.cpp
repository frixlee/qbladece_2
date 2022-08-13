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

#include "node.h"
#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Geometry>
using Eigen::Vector4d;
Node::Node()
{
    Coords.setZero();
    Coords(3)=1;
}

void Node::PrintCoords(){
    std::cout<<Coords(0)<<","<<Coords(1)<<","<<Coords(2)<<std::endl;
}

Node Node::Subtract(Node VectorB)
{
    Vector4d DiffVec = Coords - VectorB.Coords;
    Node DiffNode;
    DiffNode.Coords = DiffVec;
    return DiffNode;
}
