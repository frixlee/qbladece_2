/**********************************************************************

    Copyright (C) 2019 Joseph Saverin <joseph.saverin@qblade.org>

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

#include "Math_Types.h"
#include "Octtree.h"

#ifndef TREE_METHODS_H
#define TREE_METHODS_H

//typedef unsigned int    uint;       // Shorthand

// Tree ID type.
//static uint const Tree_Lim = 10;     // Root node level

// Tree limits are dictated by +/- H_Grid*2**Tree_Lim;

//typedef std::vector<uint> TreeID;   // Tree ID Type

//static TreeID TreeTemp = [] {TreeID V; V.assign(Tree_Lim+1,0); return V;}();
//    for (int i=0; i<Tree_Lim+1; i++) V.push_back(0);
//    return V;
//}();

Cart_ID OctTree_CID(const Vector3 &Pos, const Real &H, const uint &Level=0)
{
    // This will return the TID by first identifying the cartesian ID of the location
    // Note: Pos must be in the local system

    int BS = 1<<Level;
    Real SideFac = 1.0/(H*BS);

    Real I = Pos(0)*SideFac;
    Real J = Pos(1)*SideFac;
    Real K = Pos(2)*SideFac;

    int II = int(I);
    int JJ = int(J);
    int KK = int(K);

    if (I<0) II--;
    if (J<0) JJ--;
    if (K<0) KK--;

    return Cart_ID(II,JJ,KK);
}

void OctTree_TID(const Cart_ID &ID,                         // Cartesian ID
                  TreeID &TID,                              // Tree ID
                  const uint &Level = 0,                    // Desired level in tree
                  const uint &Br = 0,                       // Initial branch is zero = Root node
                  const Cart_ID &Centre = Cart_ID(0,0,0))   // Initial Centre is zero
{
    // Recursive function to specify TID
    // Note: ID is specified at ROOT level!

    int Quad=0;
    int BS = 1<<(Tree_Lim-Br-1);    // Boxlength side
    Cart_ID NewCentre = Centre;

    // X quadrant
    if (ID(0)-Centre(0) >= 0)
    {
            Quad += 4;
            NewCentre(0) += BS;
    }
    else    NewCentre(0) -= BS;

    // Y quadrant
    if (ID(1)-Centre(1) >= 0)
    {
            Quad += 2;
            NewCentre(1) += BS;
    }
    else    NewCentre(1) -= BS;

    if (ID(2)-Centre(2) >= 0)
    {
            Quad += 1;
            NewCentre(2) += BS;
    }
    else    NewCentre(2) -= BS;

    TID[Br] = Quad;
    if (Br == (Tree_Lim-Level))    return;
    return OctTree_TID(ID,TID,Level,Br+1,NewCentre);
}


#endif // OCTTREE_H
