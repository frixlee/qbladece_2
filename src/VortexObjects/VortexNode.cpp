/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#include "VortexNode.h"
#include "VortexLine.h"

VortexNode::VortexNode(){
    x=0.0;
    y=0.0;
    z=0.0;

    fromTimestep = -1;
    fromStation = -1;
    fromBlade = -1;
    fromStrut = -1;
    wasConvected = false;
    attachedLines.clear();
    m_bisNew = false;
}

VortexNode::VortexNode(double const &xi, double const &yi, double const &zi)
    :Vec3 (xi, yi, zi) {

    fromTimestep = -1;
    fromStation = -1;
    fromBlade = -1;
    fromStrut = -1;
    wasConvected = false;
    attachedLines.clear();
    m_bisNew = false;
}

void VortexNode::StoreInitialState(){
    initial_position = *this;
}

void VortexNode::ClearStateArrays(){
    velocity_stored.clear();
}

void VortexNode::StoreRatesOfChange(){
    velocity_stored.append(velocity);
}

void VortexNode::Update(double dT){
        this->x += velocity.x*dT;
        this->y += velocity.y*dT;
        this->z += velocity.z*dT;
}

void VortexNode::attachLine(void *line){
    for (int i = 0; i< attachedLines.size();i++){
        if (attachedLines.at(i) == line) return;
    }
    attachedLines.push_back(line);
}

void VortexNode::detachLine(void *line){
    for (int i=0;i<attachedLines.size();i++){
        if (attachedLines.at(i) == line){
            attachedLines.erase(attachedLines.begin()+i);
        }
    }
}

bool VortexNode::hasLines(){
    if (attachedLines.size()) return true;
    else return false;
}
