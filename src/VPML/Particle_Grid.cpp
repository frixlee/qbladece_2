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

//-----------------------------------------------------------------------------
//-------------------------Particle Grid Functions-----------------------------
//-----------------------------------------------------------------------------

#include "Particle.h"
#include "Particle_Grid.h"

namespace VPML
{

//--- Particle Set filtering procedures

void Particle_Grid::Constrain_Particle_Set(StateVector &P, StateVector &PM1, StateVector &DPM1)
{
    // This function carries out spatial checks on the particle set to ensure that the
    // particle always staw within a certain region. This is practical for flows quich are symmetrix or periodic

    // If the particle set has been resized, the data from the previous timestep does not yet exist
    if (!Vars->CONSTRAIN) return;

    //--- Periodicity constraints
    if (Vars->Periodic) Constrain_Periodic(P,PM1,DPM1,Vars->Lx_Per);

    //--- Symmetry constraints
    if (Vars->SYMM_X)   Constrain_XSymm(P,PM1,DPM1);
    if (Vars->SYMM_Y)   Constrain_YSymm(P,PM1,DPM1);
}

//--- Field corrections

void Particle_Grid::Add_Freestream(StateVector &DP, const Vector3 Freestream)
{
    // Adds the Vars->Freestream components onto an array
    // For the majority of simulations, this is simply a

    if (Freestream.norm() == 0) return;

    OpenMPfor
    for (int i=0; i<DP.size(); i++)
    {
        DP[i](0) += Freestream(0);
        DP[i](1) += Freestream(1);
        DP[i](2) += Freestream(2);
    }
}

void Particle_Grid::Add_Freestream(const StateVector &P, StateVector &DP)
{
    // Adds the Vars->Freestream components in the case that the
    // freestream component depends on the spatial position.

    Real zlim = 80.0*Grid_Vars->H_Grid;
    Real R = 1.0;
    Real Theta = 0.1;
    Real C = 0.25*R/Theta;
    Real Uj = 1.0;

    OpenMPfor
    for (int i=0; i<DP.size(); i++)
    {
        Real x = P[i](0);
        Real y = P[i](1);
        Real z = P[i](2);
        Real r = sqrt(x*x + y*y);
        if (r>R) continue;
        if (z>zlim) continue;
        Real Mu = C*(r/R - R/r);
//        Real U = 0.5*Uj*(1.0-tanh(Mu));
        DP[i](2) += 0.5*Uj*(1.0-tanh(Mu));
    }
}

//--- Remeshing

void Particle_Grid::Remesh_Particles(StateVector &P)
{
    // Two options are avaiable here for remeshing
    if (Vars->Grid_Option==TREE)     Remesh_Particles_Tree(P);
    if (Vars->Grid_Option==BLOCK)    Remesh_Particles_Block(P);
}

void Particle_Grid::Remesh_Particles_Tree(StateVector &P)
{
    // New method to remesh particles with ML domain

//    Timer Clock;            // Timer objects

    //--- Prepare particle set
    StateVector RmSrc, DMVec1,DMVec2;
    StdAppend(RmSrc,P);                 // Duplicate list
    uint NSizePrev = P.size();
//    Real G_MaxPrev = MaxGamma(P);       // Check max om prev

    //--- Remeshing step 1
    Bin_Sources(RmSrc);
    RmSrc.clear();

    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)   Active_Boxes[i]->Remesh_Box(Grid_Vars->Remesh_Mapping);
    for (int i=0; i<Active_Boxes.size(); i++)   StdAppend(RmSrc,Active_Boxes[i]->Src_Nodes);
    Constrain_Particle_Set(RmSrc,DMVec1,DMVec2);            // Carry out constraints if necessary

    //--- Clear data
    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)   Active_Boxes[i]->Clear_Source_Data();
    Active_Boxes.clear();

    //--- Remeshing step 2
    Bin_Sources(RmSrc);
    RmSrc.clear();
    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)   Active_Boxes[i]->Remesh_Box(M0);
    for (int i=0; i<Active_Boxes.size(); i++)   StdAppend(RmSrc,Active_Boxes[i]->Src_Nodes);

    //--- Clear data
    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)Active_Boxes[i]->Clear_Source_Data();
    Active_Boxes.clear();

    //--- Update particle set
    StdClearAppend(P,RmSrc);
    uint NSizePost = P.size();
//    Real G_MaxPost = MaxGamma(P);

//    Clock.Add_Entry("Remeshing");
//    Clock.Print_Time();

//    qDebug()    << "Particle Set Remeshed. Nparts prev = " << NSizePrev << ", Nparts post = " << NSizePost
//                << "Circ Max Prev = " << G_MaxPrev << ", Circ Max Post = " << G_MaxPost ;

//    qDebug()    << "Particle Set Remeshed. Nparts prev = " << NSizePrev << ", Nparts post = " << NSizePost;
}

inline Vector  M0_3D(const Vector3 &PR)
{
    // This returns the map as a vector. Will be assigned to each particle.

    Vector V = Vector::Zero(8);

    // M2 Scheme
    Real MX2[2] = {1-PR(0),PR(0)};
    Real MY2[2] = {1-PR(1),PR(1)};
    Real MZ2[2] = {1-PR(2),PR(2)};

    V(0) = MX2[0]*MY2[0]*MZ2[0];
    V(1) = MX2[0]*MY2[0]*MZ2[1];
    V(2) = MX2[0]*MY2[1]*MZ2[0];
    V(3) = MX2[0]*MY2[1]*MZ2[1];
    V(4) = MX2[1]*MY2[0]*MZ2[0];
    V(5) = MX2[1]*MY2[0]*MZ2[1];
    V(6) = MX2[1]*MY2[1]*MZ2[0];
    V(7) = MX2[1]*MY2[1]*MZ2[1];

    return V;
}

inline Vector  M1_3D(const Vector3 &PR)
{
    // This returns the map as a vector. Will be assigned to each particle.

    Vector V = Vector::Zero(64);

    // Set the X,Y,Z interpolant values
    Real X,Y,Z;

    // M4' Scheme
    Real MX4[4];
    X = 1+PR(0);   MX4[0] = 0.5*(1-X)*(2-X)*(2-X);
    X = PR(0);     MX4[1] = 0.5*(2-5*X*X+3*X*X*X);
    X = 1-PR(0);   MX4[2] = 0.5*(2-5*X*X+3*X*X*X);
    X = 2-PR(0);   MX4[3] = 0.5*(1-X)*(2-X)*(2-X);

    Real MY4[4];
    Y = 1+PR(1);   MY4[0] = 0.5*(1-Y)*(2-Y)*(2-Y);
    Y = PR(1);     MY4[1] = 0.5*(2-5*Y*Y+3*Y*Y*Y);
    Y = 1-PR(1);   MY4[2] = 0.5*(2-5*Y*Y+3*Y*Y*Y);
    Y = 2-PR(1);   MY4[3] = 0.5*(1-Y)*(2-Y)*(2-Y);

    Real MZ4[4];
    Z = 1+PR(2);   MZ4[0] = 0.5*(1-Z)*(2-Z)*(2-Z);
    Z = PR(2);     MZ4[1] = 0.5*(2-5*Z*Z+3*Z*Z*Z);
    Z = 1-PR(2);   MZ4[2] = 0.5*(2-5*Z*Z+3*Z*Z*Z);
    Z = 2-PR(2);   MZ4[3] = 0.5*(1-Z)*(2-Z)*(2-Z);

//    for (int x=0; x<4; x++)
//    {
//        for (int y=0; y<4; y++)
//        {
//            for (int z=0; z<4; z++) V(x*16+y*4+z) = MX4[x]*MY4[y]*MZ4[z];
//        }
//    }

    V(0)  =  MX4[0]*MY4[0]*MZ4[0];
    V(1)  =  MX4[0]*MY4[0]*MZ4[1];
    V(2)  =  MX4[0]*MY4[0]*MZ4[2];
    V(3)  =  MX4[0]*MY4[0]*MZ4[3];
    V(4)  =  MX4[0]*MY4[1]*MZ4[0];
    V(5)  =  MX4[0]*MY4[1]*MZ4[1];
    V(6)  =  MX4[0]*MY4[1]*MZ4[2];
    V(7)  =  MX4[0]*MY4[1]*MZ4[3];
    V(8)  =  MX4[0]*MY4[2]*MZ4[0];
    V(9)  =  MX4[0]*MY4[2]*MZ4[1];
    V(10) =  MX4[0]*MY4[2]*MZ4[2];
    V(11) =  MX4[0]*MY4[2]*MZ4[3];
    V(12) =  MX4[0]*MY4[3]*MZ4[0];
    V(13) =  MX4[0]*MY4[3]*MZ4[1];
    V(14) =  MX4[0]*MY4[3]*MZ4[2];
    V(15) =  MX4[0]*MY4[3]*MZ4[3];

    V(16) =  MX4[1]*MY4[0]*MZ4[0];
    V(17) =  MX4[1]*MY4[0]*MZ4[1];
    V(18) =  MX4[1]*MY4[0]*MZ4[2];
    V(19) =  MX4[1]*MY4[0]*MZ4[3];
    V(20) =  MX4[1]*MY4[1]*MZ4[0];
    V(21) =  MX4[1]*MY4[1]*MZ4[1];
    V(22) =  MX4[1]*MY4[1]*MZ4[2];
    V(23) =  MX4[1]*MY4[1]*MZ4[3];
    V(24) =  MX4[1]*MY4[2]*MZ4[0];
    V(25) =  MX4[1]*MY4[2]*MZ4[1];
    V(26) =  MX4[1]*MY4[2]*MZ4[2];
    V(27) =  MX4[1]*MY4[2]*MZ4[3];
    V(28) =  MX4[1]*MY4[3]*MZ4[0];
    V(29) =  MX4[1]*MY4[3]*MZ4[1];
    V(30) =  MX4[1]*MY4[3]*MZ4[2];
    V(31) =  MX4[1]*MY4[3]*MZ4[3];

    V(32) =  MX4[2]*MY4[0]*MZ4[0];
    V(33) =  MX4[2]*MY4[0]*MZ4[1];
    V(34) =  MX4[2]*MY4[0]*MZ4[2];
    V(35) =  MX4[2]*MY4[0]*MZ4[3];
    V(36) =  MX4[2]*MY4[1]*MZ4[0];
    V(37) =  MX4[2]*MY4[1]*MZ4[1];
    V(38) =  MX4[2]*MY4[1]*MZ4[2];
    V(39) =  MX4[2]*MY4[1]*MZ4[3];
    V(40) =  MX4[2]*MY4[2]*MZ4[0];
    V(41) =  MX4[2]*MY4[2]*MZ4[1];
    V(42) =  MX4[2]*MY4[2]*MZ4[2];
    V(43) =  MX4[2]*MY4[2]*MZ4[3];
    V(44) =  MX4[2]*MY4[3]*MZ4[0];
    V(45) =  MX4[2]*MY4[3]*MZ4[1];
    V(46) =  MX4[2]*MY4[3]*MZ4[2];
    V(47) =  MX4[2]*MY4[3]*MZ4[3];

    V(48) =  MX4[3]*MY4[0]*MZ4[0];
    V(49) =  MX4[3]*MY4[0]*MZ4[1];
    V(50) =  MX4[3]*MY4[0]*MZ4[2];
    V(51) =  MX4[3]*MY4[0]*MZ4[3];
    V(52) =  MX4[3]*MY4[1]*MZ4[0];
    V(53) =  MX4[3]*MY4[1]*MZ4[1];
    V(54) =  MX4[3]*MY4[1]*MZ4[2];
    V(55) =  MX4[3]*MY4[1]*MZ4[3];
    V(56) =  MX4[3]*MY4[2]*MZ4[0];
    V(57) =  MX4[3]*MY4[2]*MZ4[1];
    V(58) =  MX4[3]*MY4[2]*MZ4[2];
    V(59) =  MX4[3]*MY4[2]*MZ4[3];
    V(60) =  MX4[3]*MY4[3]*MZ4[0];
    V(61) =  MX4[3]*MY4[3]*MZ4[1];
    V(62) =  MX4[3]*MY4[3]*MZ4[2];
    V(63) =  MX4[3]*MY4[3]*MZ4[3];

    return V;
}

inline uint IDGrid(const uint &i,const uint &j,const uint &k, const uint &XG , const uint &YG, const uint &ZG)  {return i+j*XG+k*XG*YG;}

void Particle_Grid::Remesh_Particles_Block(StateVector &P)
{
    // I specify a global block and fill this. Basically the most straight-forward way to remesh.

//    Timer Clock;

    //--- Prepare particle set
    StateVector RmSrc, DMVec1,DMVec2;
    StdAppend(RmSrc,P);                 // Duplicate list
    uint NSizePrev = P.size();
    Real G_MaxPrev = MaxGamma(P);       // Check max om prev
    Real HG = Grid_Vars->H_Grid;

    OpenMPfor
    for (int i=0; i<P.size(); i++) Shift_Pos(P[i]);

    // Check that the grid is large enough
    Specify_Block_Grid(P);

    // Specify block grid for remeshing
    Cart_ID OSuper(0,0,0);
    Cart_ID OGlob = Block_Grid_Lower*Grid_Vars->N_ML;
    Cart_ID NGlob = Block_Grid_Delta*Grid_Vars->N_ML;
    Vector3 CornGlob = Vector3(OGlob(0),OGlob(1),OGlob(2))*HG;
    int NX = NGlob(0) + 1;
    int NY = NGlob(1) + 1;
    int NZ = NGlob(2) + 1;
    int N_Points = NX*NY*NZ;

    // Allocate block grid
    Real **Omega = new Real*[3];
    Omega[0] = (Real*)malloc(N_Points*sizeof(Real));
    Omega[1] = (Real*)malloc(N_Points*sizeof(Real));
    Omega[2] = (Real*)malloc(N_Points*sizeof(Real));
    memset(Omega[0], 0.0, N_Points*sizeof(Real));
    memset(Omega[1], 0.0, N_Points*sizeof(Real));
    memset(Omega[2], 0.0, N_Points*sizeof(Real));

    Bin_Sources_Block(P);           // Bin sources

    OpenMPfor                       // Map particle in each box to grid
    for (int i=0; i<Active_Boxes.size(); i++)
    {
        for (int n=0; n<Active_Boxes[i]->Src_Nodes.size(); n++)
        {
            Vector P = Active_Boxes[i]->Src_Nodes[n];
            Vector3 PosRel(P(0)-CornGlob(0),P(1)-CornGlob(1),P(2)-CornGlob(2));
            Cart_ID C = Get_CID(PosRel,HG);
            Vector3 PosCorn(C(0)*HG,C(1)*HG,C(2)*HG);
            Vector3 CRel = (PosRel-PosCorn)/HG;
            Vector3 O(P(3),P(4),P(5));

            int ID;
            int r=0;
            Real F;
            if (Grid_Vars->Remesh_Mapping==M2)
            {
                Vector M = M0_3D(CRel);
                ID = IDGrid(C(0)  ,C(1)  ,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)  ,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)  ,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)  ,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
            }

            if (Grid_Vars->Remesh_Mapping==M4D)
            {
                Vector M = M1_3D(CRel);
                ID = IDGrid(C(0)-1,C(1)-1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)-1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)-1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)-1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)  ,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)  ,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)  ,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)  ,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+2,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+2,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+2,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)-1,C(1)+2,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;

                ID = IDGrid(C(0)  ,C(1)-1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)-1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)-1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)-1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)  ,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)  ,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)  ,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)  ,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+2,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+2,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+2,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)  ,C(1)+2,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;

                ID = IDGrid(C(0)+1,C(1)-1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)-1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)-1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)-1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)  ,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)  ,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)  ,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)  ,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+2,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+2,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+2,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+1,C(1)+2,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;

                ID = IDGrid(C(0)+2,C(1)-1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)-1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)-1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)-1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)  ,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)  ,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)  ,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)  ,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+1,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+1,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+1,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+1,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+2,C(2)-1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+2,C(2)  ,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+2,C(2)+1,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
                ID = IDGrid(C(0)+2,C(1)+2,C(2)+2,NX,NY,NZ); F = M(r); Omega[0][ID] += F*O(0);  Omega[1][ID] += F*O(1); Omega[2][ID] += F*O(2); r++;
            }
        }
    }

    // Extract values from grid in pencils
    std::vector<Cart_ID> PenID;
    std::vector<StateVector> PBins;
    for (int y=0; y<NY; y++) {
        for (int z=0; z<NZ; z++) {
            PenID.push_back(Cart_ID(0,y,z));
            PBins.push_back(Empty_SV);
        }
    }

    OpenMPfor
    for (int p=0; p<PenID.size(); p++) {
        int yID =  PenID[p](1);
        int zID =  PenID[p](2);
        for (int i=0; i<NX; i++) {
            int IDG = IDGrid(i,yID,zID,NX,NY,NZ);
            Vector3 Om(Omega[0][IDG],Omega[1][IDG],Omega[2][IDG]);
            if (Om.norm()!=0.0){                // Skip particle if zero contribution
                Vector3 Pos = Vector3(i*HG,yID*HG,zID*HG) + CornGlob;
                Vector8 V;  V << Pos(0), Pos(1), Pos(2), Om(0), Om(1), Om(2), Grid_Vars->Sigma_Char, Grid_Vars->Vol_Char;
                PBins[p].push_back(V);
            }
        }
    }

    //--- Update particle set
    P.clear();
    for (int p=0; p<PenID.size(); p++)  StdAppend(P,PBins[p]);
    OpenMPfor
    for (int i=0; i<P.size(); i++) Shift_Neg(P[i]);
    Real G_MaxPost = MaxGamma(P);

    // Delete block grid
    free(Omega[0]);
    free(Omega[1]);
    free(Omega[2]);
    delete Omega;

    // Clear grid
    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)               Active_Boxes[i]->Clear_Source_Data();
    Active_Boxes.clear();

//    Clock.Add_Entry("Remeshing");
//    Clock.Print_Time();

//    qDebug()    << "Particle Set Remeshed. Nparts prev = " << NSizePrev << ", Nparts post = " << P.size()
//                << "Circ Max Prev = " << G_MaxPrev << ", Circ Max Post = " << G_MaxPost ;
}

//--- Magnitude filtering

void Particle_Grid::Filter_Magnitude(StateVector &P)
{
    // Two options are avaiable here for remeshing
    if (Vars->Grid_Option==TREE)     Filter_Magnitude_Tree(P);
    if (Vars->Grid_Option==BLOCK)    Filter_Magnitude_Block(P);
}

void Particle_Grid::Set_Max_Gamma(StateVector &P){
    if (P.empty()) return;
    Max_Gamma = MaxGamma(P);
}

void Particle_Grid::Filter_Magnitude_Tree(StateVector &P)
{
    // In this case I bin the sources and carry out the magnitude filtering locally in each box.

    uint NSizePrev = P.size();
    if (P.empty()) return;

    Real GamMax = MaxGamma(P);
    Real Gamma_Thresh = Vars->Mag_Filt_Fac*Max_Gamma; //using fixed threshold

//    qDebug() << Vars->Mag_Filt_Fac << GamMax << Gamma_Thresh;

    Bin_Sources(P);                  // Sort sources into boxes

    // Carry out filtering within box
    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)   Active_Boxes[i]->Magnitude_Filtering(Gamma_Thresh);

    // Extract new nodes
    P.clear();
    for (int i=0; i<Active_Boxes.size(); i++)   StdAppend(P,Active_Boxes[i]->Src_Nodes);

    // Add residues.
    Vector3 GamRestGlob = Vector3::Zero();
    for (int i=0; i<Active_Boxes.size(); i++)   GamRestGlob += Active_Boxes[i]->Gamma_Rest;
    GamRestGlob /= (P.size()*Grid_Vars->Vol_Char);
    for (int i=0; i<P.size(); i++)
    {
        P[i](3) += GamRestGlob(0);
        P[i](4) += GamRestGlob(1);
        P[i](5) += GamRestGlob(2);
    }

    OpenMPfor
    for (int i=0; i<Active_Boxes.size(); i++)   Active_Boxes[i]->Clear_Source_Data();
    Active_Boxes.clear();

//    qDebug() << "Magnitude Filtering complete. NPrev " << NSizePrev << " NPost " << P.size();
    Vars->Set_Resized = true;
}

void Particle_Grid::Filter_Magnitude_Block(StateVector &P)
{
    // Very simple, just loop over all particles, remove the weakest and supplement
    // The remaining positions with the missing circulation

    uint NSizePrev = P.size();
    if (P.empty()) return;

    Real GamMax =    MaxGamma(P);
    Real GamThresh = Vars->Mag_Filt_Fac*MaxGamma(P);

    StateVector Strong, Weak;
    Vector3 WeakGammaTot = Vector3::Zero();
    for (int i=0; i<P.size(); i++)
    {
        Vector3 G = Gamma(P[i]);
        if (G.norm()<GamThresh)
        {
                WeakGammaTot += G;
                Weak.push_back(P[i]);
        }
        else    Strong.push_back(P[i]);
    }

    // Now replace with an equivalent particle (not yet bro!)
//    Strong.push_back(Part_Equiv(Weak,Grid_Vars->Sigma_Char,Grid_Vars->Vol_Char));

    // Caclulate total Gamma
    WeakGammaTot *= 1.0/(Grid_Vars->Vol_Char*Strong.size());
    for (int i=0; i<Strong.size(); i++){
        Strong[i](3) += WeakGammaTot(0);
        Strong[i](4) += WeakGammaTot(1);
        Strong[i](5) += WeakGammaTot(2);
    }
    P.clear();
    StdAppend(P,Strong);

    qDebug() << "Magnitude Filtering complete. NPrev " << NSizePrev << " NPost " << P.size();
}

//--- Spatial filtering

void Particle_Grid::Filter_Position(StateVector &P)
{
    // This literally removes particle which do no satisfy a certain geometric relationship
    StateVector NewParts;
    for (int i=0; i<P.size(); i++)
    {
        if (P[i](2)>0.5*Grid_Vars->H_Grid) NewParts.push_back(P[i]);       // Free jet
    }

    qDebug() << "Filtering positions" << P.size() << NewParts.size();

    StdClearAppend(P,NewParts);

}

//--- Divergence filtering

void Particle_Grid::Filter_Divergence(const StateVector &S, StateVector &P)
{
    // Two options are avaiable here for remeshing
    if (Vars->Grid_Option==TREE)     Filter_Divergence_Tree(S,P);
    if (Vars->Grid_Option==BLOCK)    Filter_Divergence_Block_GPU(S,P);
}

void Particle_Grid::Filter_Divergence_Tree(const StateVector &S, StateVector &P)
{
    // This function maps the particle to the Eulerian grid and determines the divergence
    // If the value is above a specified value, the method of Cottet is applied and the vorticity is corrected.

    Bin_Sources(S);
    Bin_Probes(P);

    OpenMPfor
    for (int i=0; i<ProActive_Boxes.size(); i++)    ProActive_Boxes[i]->Divergence_Filtering_Step1();

    Overlap_Omega();            // Overlaps Psi grid

    OpenMPfor
    for (int i=0; i<ProActive_Boxes.size(); i++)    ProActive_Boxes[i]->Divergence_Filtering_Step2();

    P.clear();
    for (int i=0; i<ProActive_Boxes.size(); i++)    StdAppend(P,Active_Boxes[i]->Prb_Nodes);

    Clear_Grid();
}

//--- Nullifying source

void Particle_Grid::Prepare_Volume_Sources(const StateVector &VX, const StateVector &X, StateVector &XTot)
{
    // This is a simple spatial check which nullifies sources in case they are in the position
    // of a spatial source

    if (VX.empty())
    {
        StdAppend(XTot,X);
        return;
    }

    Real H = Grid_Vars->H_Grid;

//    Real XMax = MaxX(VX)+H, XMin = MinX(VX)-H;
//    Real YMax = 2.5,        YMin = -2.5;
//    Real ZMax = 2.5,        ZMin = -2.5;
    Real XMax = MaxX(VX)+H, XMin = MinX(VX)-H;
    Real YMax = MaxY(VX)+H, YMin = MinY(VX)-H;
    Real ZMax = MaxZ(VX)+H, ZMin = MinZ(VX)-H;

    int NX = (XMax-XMin)/H +1;
    int NY = (YMax-YMin)/H +1;
    int NZ = (ZMax-ZMin)/H +1;
    bool VG[NX][NY][NZ];
    std::fill(VG[0][0], VG[0][0]+NX*NY*NZ, 0);

    // Step 1: Fill the array with boxes which contain spatial nodes
    for (int i=0; i<VX.size(); i++)
    {
        Real DX =  VX[i](0) - XMin;     int IDX = int(DX/H);
        Real DY =  VX[i](1) - YMin;     int IDY = int(DY/H);
        Real DZ =  VX[i](2) - ZMin;     int IDZ = int(DZ/H);

//        qDebug() << VX[i](0) << VX[i](1) << VX[i](2);
        VG[IDX][IDY][IDZ] = true;
    }

    // Step 2: Loop over material sources and nullify if necessary
    StateVector SrcTemp;
    StdAppend(SrcTemp,X);
    OpenMPfor
    for (int i=0; i<SrcTemp.size(); i++)
    {
        if (SrcTemp[i](0)>XMax || SrcTemp[i](0)<XMin) continue;
        if (SrcTemp[i](1)>YMax || SrcTemp[i](1)<YMin) continue;
        if (SrcTemp[i](2)>ZMax || SrcTemp[i](2)<ZMin) continue;

        Real DX =  SrcTemp[i](0) - XMin;     int IDX = int(DX/H);
        Real DY =  SrcTemp[i](1) - YMin;     int IDY = int(DY/H);
        Real DZ =  SrcTemp[i](2) - ZMin;     int IDZ = int(DZ/H);

        if (VG[IDX][IDY][IDZ])      // Nullify
        {
            SrcTemp[i](3) = 0.0;
            SrcTemp[i](4) = 0.0;
            SrcTemp[i](5) = 0.0;
        }
    }

    // Clear elements which do not contribute
    SrcTemp.erase(std::remove_if(SrcTemp.begin(),SrcTemp.end(),VortNull), SrcTemp.end());

    // Step 3: Add material sources and return
    StdAppend(XTot,SrcTemp);
    StdAppend(XTot,VX);
}

//--- Auxiliary functions

void Particle_Grid::Overlap_Omega()
{
    // Vorticity is mapped from the neighboring boxes
    // This is carried out on the Psi_Grid variable.

    LOG_FUNC_ENTRY;

    OpenMPfor
    for (int i=0; i<ProActive_Boxes.size(); i++)   ProActive_Boxes[i]->Psi_Grid->Trans_Om_Psi();

    int NC = 0;

    for (int IPOS=-1; IPOS<=1; IPOS++)
    {
        for (int JPOS=-1; JPOS<=1; JPOS++)
        {
            for (int KPOS=-1; KPOS<=1; KPOS++)
            {
                Cart_ID PSHIFT(IPOS,JPOS,KPOS);
                if (PSHIFT == Cart_ID::Zero()) continue;     // No need to overlap onto self!
                // 1) Check how many of these boxes are active

//                OpenMPfor //(NOPE!)
                for (int i=0; i<ProActive_Boxes.size(); i++)
                {
                    SPBox Rcvr_Box = ProActive_Boxes[i];
                    Cart_ID SrcID = Rcvr_Box->Get_BoxCID()+PSHIFT;

                    if (Vars->Periodic)
                    {
                        if      (SrcID(0)<0)                    SrcID(0) += Vars->NB_Periodic;
                        else if (SrcID(0)>=Vars->NB_Periodic)   SrcID(0) -= Vars->NB_Periodic;
                    }

                    SPBox Src_Box = Get_Box(SrcID);

                    if (Src_Box->Active)
                    {
                        //--- Psi0 Overlap
                        Cart_ID SC, RC, W;  // Source corner ID, Destination corner ID, stencil width
                        Overlap_Params(Src_Box->Psi_Grid, Rcvr_Box->Psi_Grid, Grid_Vars->OL_PG, SC, RC, W);

                        StateVector SrcPsi;
                        Src_Box->Psi_Grid->Extract_Omega_Grid(SC,W,SrcPsi);     // Extract omega
                        Rcvr_Box->Psi_Grid->Superpose_Psi_Grid(RC,W,SrcPsi);    // Transfer to Psi
                    }
                }
            }
        }
    }

    // Transfer back to omega arrays
    OpenMPfor
    for (int i=0; i<ProActive_Boxes.size(); i++)
    {
        ProActive_Boxes[i]->Psi_Grid->Trans_Psi_Om();
        ProActive_Boxes[i]->Psi_Grid->Reset_Psi();
    }

    // Note: There is no need to mark any array in the Eul Grids as being active or inactive, this was in a previous step
}

}
