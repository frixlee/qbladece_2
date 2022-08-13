/**********************************************************************

    Copyright (C) 2021 Joseph Saverin <joseph.saverin@qblade.org>

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

#include "Box_Tree.h"

#ifndef PARTICLE_GRID_H
#define PARTICLE_GRID_H

namespace  VPML
{

class Particle_Grid : public Box_Tree
{
protected:

    //--- Field corrections

    void Add_Freestream(StateVector &DP, const Vector3 Freestream);
    void Add_Freestream(const StateVector &P, StateVector &DP);

public:

    //--- Constructor

    Particle_Grid() : Box_Tree() {}

    //--- Particle Set filtering procedures
    void Constrain_Particle_Set(StateVector &P, StateVector &PM1, StateVector &DPM1);

    //--- Remeshing
    void Remesh_Particles(StateVector &P);
    void Remesh_Particles_Tree(StateVector &P);
    void Remesh_Particles_Block(StateVector &P);

    //--- Magnitude Filter
    void Set_Max_Gamma(StateVector &P);
    void Filter_Magnitude(StateVector &P);
    void Filter_Magnitude_Tree(StateVector &P);
    void Filter_Magnitude_Block(StateVector &P);

    //--- Position Filtering
    void Filter_Position(StateVector &P);

    //--- Divergence filter
    void Filter_Divergence(const StateVector &S, StateVector &P);
    void Filter_Divergence_Tree(const StateVector &S, StateVector &P);
    void Filter_Divergence_Block(const StateVector &S, StateVector &P)              {qDebug() << "Filter_Divergence_Block template function not implemented";}
    virtual void Filter_Divergence_Block_GPU(const StateVector &S, StateVector &P)  {qDebug() << "Filter_Divergence_Block_GPU template function not implemented";}

    //--- Nullifying source
    void Prepare_Volume_Sources(const StateVector &VX, const StateVector &X, StateVector &XTot);

    //--- Auxiliary functions
    void Overlap_Omega();

    double Max_Gamma;
};

}

#endif // PARTICLE_GRID_H
