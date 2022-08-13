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

//-----------------------------------------------------------------------------
//-------------------------ML Box Functions------------------------------------
//-----------------------------------------------------------------------------

#include "ML_Box.h"
#include "Particle.h"

namespace VPML
{

//--- Node access functions

Cart_ID  Box::Get_CartID(const Vector3 &V)
{
    // This returns the cartesian ID of the given position based on the grid size.
    Real H = Grid_Vars->H_Grid;
    Vector3 PN = V/H;
    int X = int(floor(PN(0)));    //if (V(0)<0) X--;
    int Y = int(floor(PN(1)));    //if (V(0)<0) X--;
    int Z = int(floor(PN(2)));    //if (V(0)<0) X--;

    return Cart_ID(X,Y,Z);
}

Cart_ID  Box::Get_CartID(const Vector3 &V, const Real &HT)
{
    // This returns the cartesian ID of the given position based on the grid size.
    Vector3 PN = V/HT;
    int X = int(floor(PN(0)));    if (X<0) X=0;     if (X>3) X=3;
    int Y = int(floor(PN(1)));    if (Y<0) Y=0;     if (Y>3) Y=3;
    int Z = int(floor(PN(2)));    if (Z<0) Z=0;     if (Z>3) Z=3;

    return Cart_ID(X,Y,Z);
}

Cart_ID  Box::Get_CartID_Safe(const Vector3 &V, const Cart_ID &CID_Eul, const int &OLL, const int &OLR)
{
    // This does the same thing as the code above, however is carries out an additional check
    // to ensure there are no issues with the overlap

    Cart_ID CIDL = Get_CartID(V);
    Cart_ID CID_Rel = CIDL - CID_Eul;

    if (CID_Rel(0)<OLL) CIDL(0)++;
    if (CID_Rel(1)<OLL) CIDL(1)++;
    if (CID_Rel(2)<OLL) CIDL(2)++;
    if (CID_Rel(0)>OLR) CIDL(0)--;
    if (CID_Rel(1)>OLR) CIDL(1)--;
    if (CID_Rel(2)>OLR) CIDL(2)--;

    return CIDL;
}

//--- Mapping functions

Vector  Box::M0_2D(const Vector3 &PR)
{
    // This returns the map as a vector. Will be assigned to each particle.

    Vector V = Vector::Zero(4);

    // Set the X,Y,Z interpolant values

    // M2 Scheme
    Real MX2[2] = {1-PR(0),PR(0)};
    Real MY2[2] = {1-PR(1),PR(1)};

    V(0) = MX2[0]*MY2[0];
    V(1) = MX2[0]*MY2[1];
    V(2) = MX2[1]*MY2[0];
    V(3) = MX2[1]*MY2[1];

    return V;
}

Vector  Box::M1_2D(const Vector3 &PR)
{
    // This returns the map as a vector. Will be assigned to each particle.

    Vector V = Vector::Zero(16);

    // Set the X,Y,Z interpolant values
    Real X0 = PR(0);
    Real Y0 = PR(1);
    Real X, Y;

    // M4' Scheme
    Real MX4[4];
    X = 1+X0;   MX4[0] = 0.5*(1-X)*(2-X)*(2-X);
    X = X0;     MX4[1] = 0.5*(2-5*X*X+3*X*X*X);
    X = 1-X0;   MX4[2] = 0.5*(2-5*X*X+3*X*X*X);
    X = 2-X0;   MX4[3] = 0.5*(1-X)*(2-X)*(2-X);

    Real MY4[4];
    Y = 1+Y0;   MY4[0] = 0.5*(1-Y)*(2-Y)*(2-Y);
    Y = Y0;     MY4[1] = 0.5*(2-5*Y*Y+3*Y*Y*Y);
    Y = 1-Y0;   MY4[2] = 0.5*(2-5*Y*Y+3*Y*Y*Y);
    Y = 2-Y0;   MY4[3] = 0.5*(1-Y)*(2-Y)*(2-Y);

    V(0) = MX4[0]*MY4[0];
    V(1) = MX4[0]*MY4[1];
    V(2) = MX4[0]*MY4[2];
    V(3) = MX4[0]*MY4[3];
    V(4) = MX4[1]*MY4[0];
    V(5) = MX4[1]*MY4[1];
    V(6) = MX4[1]*MY4[2];
    V(7) = MX4[1]*MY4[3];
    V(8) = MX4[2]*MY4[0];
    V(9) = MX4[2]*MY4[1];
    V(10) = MX4[2]*MY4[2];
    V(11) = MX4[2]*MY4[3];
    V(12) = MX4[3]*MY4[0];
    V(13) = MX4[3]*MY4[1];
    V(14) = MX4[3]*MY4[2];
    V(15) = MX4[3]*MY4[3];

    return V;
}

Vector  Box::M0_3D(const Vector3 &PR)
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

Vector  Box::M1_3D(const Vector3 &PR)
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

Vector  Box::Get_Interp_Matrix(const GRID_MAP &MAP, const Cart_ID &PCID, const Vector3 &Pos)
{
    // This calculates the interpolation matrix mased on the desired interpolation scheme
    Vector3 Corn_Pos = Vector3(PCID(0),PCID(1),PCID(2))*Grid_Vars->H_Grid;
    Vector3 Pos_Rel = (Pos-Corn_Pos)/Grid_Vars->H_Grid;

    if (Vars->Dim==SIM_3D)
    {
        switch (MAP)
        {
            case (M0):  return Matrix::Ones(1,1);
            case (M2):  return M0_3D(Pos_Rel);
            case (M4D): return M1_3D(Pos_Rel);
            default: break;
        }
    }
    if (Vars->Dim==SIM_2D)
    {
        switch (MAP)
        {
            case (M0):  return Matrix::Ones(1,1);
            case (M2):  return M0_2D(Pos_Rel);
            case (M4D): return M1_2D(Pos_Rel);
            default: break;
        }
    }
}

//------------ Barycentric variables and interpolants -----------------

void    Box::Calc_Li(const int &Ns, const Vector &Xs, const Vector &Ws, const Real &x, Real *Li)
{
    // This calculates for a single Dimension the value over Li
    // of the basis interpolation polynomials

    // Create sum denominator
    for (int i=0; i<Ns; i++)                // Check for exact positions (rare but important!)
    {
        if (x==Xs(i))
        {
            for (int j=0; j<Ns; j++)    Li[j] = 0.0;
            Li[i] = 1.0;
            return;
        }
    }

    Real Den = 0;
    for (int j=0; j<Ns; j++) Den += Ws(j)/(x - Xs(j));

    for (int i=0; i<Ns; i++)                // Cycle i (particles)
    {
        Real Num = Ws(i)/(x - Xs(i));       // Create numerator
        Li[i] = Num/Den;
    }
}

void    Box::Set_BL_Interp_Mat_2D(const int &Ns, const Vector &Xs, const Vector &Ws, const Matrix &Rel_pos, Matrix &Int_mat)
{
    // This function calculates the entries of the interpolation matrix.
    // See description for 3D function below

    int N_P = Rel_pos.rows();

    //---1) Calculate barycentric basis polynomials

    Real L[2][N_P][Ns]; // = {{{0}}};

    for (int P=0; P<N_P; P++)
    {
        Calc_Li(Ns, Xs, Ws, Rel_pos(P,0), L[0][P]);
        Calc_Li(Ns, Xs, Ws, Rel_pos(P,1), L[1][P]);
    }

//    Rel_pos

    //---2) Set up and fill interpolation matrix

    int NTOTs = 1;
    for (int i=0; i<2; i++) NTOTs *= Ns;

    Int_mat = Matrix::Zero(N_P,NTOTs);

    for (int P=0; P<N_P; P++)
    {
        for (int i=0; i<Ns; i++)
        {
            for (int j=0; j<Ns; j++)
            {
                Real Lx = L[0][P][i];
                Real Ly = L[1][P][j];

                Int_mat(P, i*Ns + j) = Lx*Ly;
            }
        }
    }
}

void    Box::Set_BL_Interp_Mat_3D(const int &Ns, const Vector &Xs, const Vector &Ws, const Matrix &Rel_pos, Matrix &Int_mat)
{
    // This function calculates the entries of the interpolation matrix.

    // The convenient case here is that the weights of the interpolation positions
    // are already known as they are analytical for Chebyshev node distributions...

    // For the Calc_Basis_Fns function, we utilize Wi, which was created during construction.
    // We also need to ensure that the Rel_Pos of the src_Nodes is defined correctly,
    // which likely occured during Interpolation step...

    // As was done for anterpolation, we make use of Calc_Li to determine the Lagrangian
    // basis polynomials

    int N_P = Rel_pos.rows();

    //---1) Calculate barycentric basis polynomials

    Real L[3][N_P][Ns];     // = {{{0}}};

    for (int i=0; i<N_P; i++)
    {
        Calc_Li(Ns, Xs, Ws, Rel_pos(i,0), L[0][i]);
        Calc_Li(Ns, Xs, Ws, Rel_pos(i,1), L[1][i]);
        Calc_Li(Ns, Xs, Ws, Rel_pos(i,2), L[2][i]);
    }

//    Rel_pos

    //---2) Set up and fill interpolation matrix

    int NTOTs = 1;
    for (int i=0; i<3; i++) NTOTs *= Ns;

    Int_mat = Matrix::Zero(N_P,NTOTs);

    for (int P=0; P<N_P; P++)
    {
        for (int i=0; i<Ns; i++)
        {
            for (int j=0; j<Ns; j++)
            {
                for (int k=0; k<Ns; k++)
                {
                    Real Lx = L[0][P][i];
                    Real Ly = L[1][P][j];
                    Real Lz = L[2][P][k];

                    Int_mat(P, i*Ns*Ns + j*Ns + k) = Lx*Ly*Lz;
                }
            }
        }
    }
}

//--- Aux functions

Matrix Box::Get_Omega_Array()
{
    Matrix OfTheMack = Matrix::Zero(Src_Nodes.size(),3);
    for (int i=0; i<Src_Nodes.size(); i++)
    {
        OfTheMack(i,0) = Src_Nodes[i](3);
        OfTheMack(i,1) = Src_Nodes[i](4);
        OfTheMack(i,2) = Src_Nodes[i](5);
    }
    return OfTheMack;   // Oh my god!
}

Matrix Box::Get_Rel_Position(const StateVector &P, const Vector3 &Ref, const Real &H)
{
    // Here I return the relative position of the source nodes
    Matrix M = Matrix::Zero(P.size(),3);
    for (int i=0; i<P.size(); i++)
    {
        M(i,0) = P[i](0) - Ref(0);
        M(i,1) = P[i](1) - Ref(1);
        M(i,2) = P[i](2) - Ref(2);
    }
    return M/H;     // Make relative!!
}

void Box::Get_Probe_Position(Matrix &M)
{
//    M = Matrix::Zero(Probe_Nodes.size(),3);
//    for (int i=0; i<Probe_Nodes.size(); i++)
//    {
//        M.row(i) <<  Probe_Nodes[i](0), Probe_Nodes[i](1), Probe_Nodes[i](2);
//    }

    M = Matrix::Zero(Prb_Nodes.size(),3);
    for (int i=0; i<Prb_Nodes.size(); i++)
    {
        M.row(i) <<  Prb_Nodes[i](0), Prb_Nodes[i](1), Prb_Nodes[i](2);
    }
}

//--- Source prep

void Box::Prepare_Source_Data_BS3()
{
    // Map to ML grid
    Map_Sources_to_ML_Grid();
}

void Box::Prepare_Source_Data_Poisson_Dir()
{
    // Map to ML grid
    Create_Psi_Grid();      // Ensure Eul Grid allocated
    Map_Sources_to_Eul_Grid(Grid_Vars->Mapping,true,false,false,Vars->Prbs_Are_Srcs);
}

void Box::Prepare_Source_Data_Poisson_JL()
{
    Create_Psi_Grid();
    Create_Omega_Grid();
    Omega_Grid->BG = MatrixRM::Zero(Omega_Grid->N_BPoints,3);     // Allocate boundary gradient matrix
    Omega_Grid->BPsi0 = MatrixRM::Zero(Psi_Grid->N_BPoints,3);    // Allocate psi0 matrix
    Map_Sources_to_Eul_Grid(Grid_Vars->Mapping,true,true,false,Vars->Prbs_Are_Srcs);
    M2Anterp();                   // Method 2 Eul grid style interp
    Omega_Grid->Trans_Om_Psi();
    Omega_Grid->PoissonJL_Compact_Support();
    Omega_Grid->Solve_Poisson();
    Omega_Grid->Set_Boundary_Gradient();
}

void Box::Map_Sources_to_ML_Grid()
{
    // This function is called when anterpolating the SOURCE NODES contained within the
    // box to the INTERPOLATION nodes

    // First compose elements together which are going to contribute to the ML far field sources
    StateVector MLSrcNodes;
    StdAppend(MLSrcNodes,Src_Nodes);
    StdAppend(MLSrcNodes,Spat_Src_Nodes);

    // Calculate the relative source positions wthin the box
    Real SL = Grid_Vars->H_ML*pow(2,Branch);
    Matrix Pos_Rel = Get_Rel_Position(MLSrcNodes,Get_Centre(),0.5*SL);

    // Now specify sources
    Matrix IntMat;
    if (Vars->Dim==SIM_2D) Set_BL_Interp_Mat_2D(Grid_Vars->Poly_ML,Grid_Vars->Node_Template_1D,Grid_Vars->Weight_Template_1D,Pos_Rel,IntMat);
    if (Vars->Dim==SIM_3D) Set_BL_Interp_Mat_3D(Grid_Vars->Poly_ML,Grid_Vars->Node_Template_1D,Grid_Vars->Weight_Template_1D,Pos_Rel,IntMat);
    ML_Src = IntMat.transpose()*Get_Source_Strength();
}

void Box::M2Anterp()
{
    // This function carries out an M2 style interpolation of the grid and then maps these to the ML grid.
    // This should significantly speed up calculations for a dense grid, as we avoid the many calls to the Barycentric Polynomial calculation

    // Trial new approach. Much simpler. Anterpolate to 5*5 grid
    Matrix SrcGrid = Matrix::Zero(125,3);
    Real HG = Grid_Vars->H_Grid;
    Real HS = Grid_Vars->H_ML/4.0;
    Cart_ID CIDB = CID*Grid_Vars->N_ML;
    Vector3 BoxCorner(CIDB(0)*HG,CIDB(1)*HG,CIDB(2)*HG);

    for (int p=0; p<Src_Nodes.size(); p++)  // Loop over material particles
    {
        Vector3 Pos = Node_Pos(Src_Nodes[p]);
        Cart_ID PCIDL = Get_CartID(Pos-BoxCorner,HS);
        Vector3 CellCorner = BoxCorner + Vector3(PCIDL(0)*HS,PCIDL(1)*HS,PCIDL(2)*HS);
        Vector3 Pos_Rel = (Pos-CellCorner)/HS;
        Vector  PMAP = M0_3D(Pos_Rel);
        Vector3 Omega(Src_Nodes[p](3),Src_Nodes[p](4),Src_Nodes[p](5));
        Vector3 SRC = Src_Nodes[p](7)*Omega.transpose();

        // Increment the intermediate source grid
        SrcGrid.row((PCIDL(0)+0)*25 + (PCIDL(1)+0)*5 + PCIDL(2)  ) += PMAP(0)*SRC;
        SrcGrid.row((PCIDL(0)+0)*25 + (PCIDL(1)+0)*5 + PCIDL(2)+1) += PMAP(1)*SRC;
        SrcGrid.row((PCIDL(0)+0)*25 + (PCIDL(1)+1)*5 + PCIDL(2)  ) += PMAP(2)*SRC;
        SrcGrid.row((PCIDL(0)+0)*25 + (PCIDL(1)+1)*5 + PCIDL(2)+1) += PMAP(3)*SRC;
        SrcGrid.row((PCIDL(0)+1)*25 + (PCIDL(1)+0)*5 + PCIDL(2)  ) += PMAP(4)*SRC;
        SrcGrid.row((PCIDL(0)+1)*25 + (PCIDL(1)+0)*5 + PCIDL(2)+1) += PMAP(5)*SRC;
        SrcGrid.row((PCIDL(0)+1)*25 + (PCIDL(1)+1)*5 + PCIDL(2)  ) += PMAP(6)*SRC;
        SrcGrid.row((PCIDL(0)+1)*25 + (PCIDL(1)+1)*5 + PCIDL(2)+1) += PMAP(7)*SRC;
    }

    ML_Src = Grid_Vars->M2MLMap*SrcGrid;
}

Vector3 Box::Get_Centre()
{
    // Calculates the centre of the box
    Real SL = Grid_Vars->H_ML*pow(2,Branch);
    Vector3 Corn = Vector3(CID(0),CID(1),CID(2))*SL + Grid_Vars->Origin;
    if (Vars->Dim==SIM_3D) return Corn + Vector3(0.5,0.5,0.5)*SL;
    if (Vars->Dim==SIM_2D) return Corn + Vector3(0.5,0.5,0.0)*SL;
}

Matrix Box::Get_Corners()
{
    // Calculates the centre of the box
    Real SL = Grid_Vars->H_ML*pow(2,Branch);
    Vector3 C0 = Vector3(CID(0),CID(1),CID(2))*SL;      // + Grid_Vars->Origin;
    Matrix Coorns = Matrix::Zero(8,3);
    Coorns.row(0) = C0+Vector3(0,0,0);
    Coorns.row(1) = C0+Vector3(0,0,SL);
    Coorns.row(2) = C0+Vector3(0,SL,0);
    Coorns.row(3) = C0+Vector3(0,SL,SL);
    Coorns.row(4) = C0+Vector3(SL,0,0);
    Coorns.row(5) = C0+Vector3(SL,0,SL);
    Coorns.row(6) = C0+Vector3(SL,SL,0);
    Coorns.row(7) = C0+Vector3(SL,SL,SL);
    return Coorns;
}

Matrix Box::Get_Source_Strength()
{
    // Need to multiply vorticity by volume
    Matrix M = Get_Omega_Array();
    for (int i=0; i<Src_Nodes.size(); i++)  M.row(i) *= Src_Nodes[i](7);
    return M;
}

void Box::Map_Sources_to_Eul_Grid(const GRID_MAP &MAP, bool IncPsi, bool IncOmega, bool SetActive, bool Prbs_Are_Srcs)
{
    // In the case that we are using the Poisson solver, the mapping variables and Cart ID
    // variables for the particles are stored here

//    Create_Eul_Grids();    // Ensure Eul Grids allocated

    for (int p=0; p<Src_Nodes.size(); p++)     // Loop over material particles
    {
        Vector3 Pos = Node_Pos(Src_Nodes[p]);
        Cart_ID PCID = Get_CartID_Safe(Pos, Psi_Grid->Origin_Glob, Grid_Vars->OL_PG, Grid_Vars->OL_RPG);
        Matrix PMAP = Get_Interp_Matrix(MAP,PCID,Pos);
        Vector3 Omega(Src_Nodes[p](3),Src_Nodes[p](4),Src_Nodes[p](5));
        Matrix PSRC = PMAP*Omega.transpose();

        // Increment the eulerian grids
        if (IncPsi)
        {
            Psi_Grid->Increment_Omega(PCID,PSRC,MAP);
            if (SetActive)  Psi_Grid->Set_Cell_Active(PCID);
        }
        if (IncOmega)
        {
            Omega_Grid->Increment_Omega(PCID,PSRC,MAP);
            if (SetActive)  Omega_Grid->Set_Cell_Active(PCID);
        }

        if (Prbs_Are_Srcs)      // We store here the interps, as these are later necessary
        {
            Prb_Particle_CID.push_back(PCID);
            Prb_Particle_Map.push_back(PMAP);
            Psi_Grid->Set_Cell_Active(PCID);
        }
    }

    // Declare array activity
    if (IncPsi)         Psi_Grid->Set_Omega_Active();
    if (IncOmega)       Omega_Grid->Set_Omega_Active();
    if (Prbs_Are_Srcs)  Psi_Grid->Set_Omega_Active();
}

//--- Probe prep

//void Box::Prepare_Probe_Data()
//{
//    // Bin Symmetries is carried out elsewhere
//    if (!ProActive)         return;
//    if (Prb_Nodes.empty())  return;

//    if (Vars->Solver==GREEN)
//    {
//        ML_Inf = Matrix::Zero(Grid_Vars->N_Int,6);
//        Map_Probes_to_ML_Grid();
//        return;
//    }
//    else
//    {
//        ML_Inf = Matrix::Zero(Grid_Vars->N_Int,3);          // Ensure ML_Inf created
//        if (Vars->Prbs_Are_Srcs) return;
//        Set_Probe_Map(Grid_Vars->Mapping);
//    }
//}

void Box::Prepare_Probe_Data_BS3()
{
    ML_Inf = Matrix::Zero(Grid_Vars->N_Int,6);
    Map_Probes_to_ML_Grid();
}

void Box::Prepare_Probe_Data_Poisson_Dir()
{
    if (!ProActive)         return;
    if (Prb_Nodes.empty())  return;

    ML_Inf = Matrix::Zero(Grid_Vars->N_Int,3);          // Ensure ML_Inf created
    if (Vars->Prbs_Are_Srcs) return;
    Create_Psi_Grid();                             // Ensure Eul Grids allocated
    Set_Probe_Map(Grid_Vars->Mapping);
}

void Box::Map_Probes_to_ML_Grid()
{
    // This function specifies the mapping from the M2L mesh to the probe points
    Real SL = Grid_Vars->H_ML*pow(2,Branch);
    Matrix Pos_Rel = Get_Rel_Position(Prb_Nodes,Get_Centre(),0.5*SL);
    if (Vars->Dim==SIM_2D) Set_BL_Interp_Mat_2D(Grid_Vars->Poly_ML,Grid_Vars->Node_Template_1D,Grid_Vars->Weight_Template_1D,Pos_Rel,Prb_IntMat);
    if (Vars->Dim==SIM_3D) Set_BL_Interp_Mat_3D(Grid_Vars->Poly_ML,Grid_Vars->Node_Template_1D,Grid_Vars->Weight_Template_1D,Pos_Rel,Prb_IntMat);
}

void Box::Set_Probe_Map(const GRID_MAP &MAP)
{
    // This specifies the mapping matrices to extract data from the Eulerian grid
//    Create_Eul_Grids();                             // Ensure Eul Grids allocated

    for (int p=0; p<Prb_Nodes.size(); p++)
    {
        Vector3 Pos = Node_Pos(Prb_Nodes[p]);
        Cart_ID PCID = Get_CartID_Safe(Pos, Psi_Grid->Origin_Glob, Grid_Vars->OL_PG, Grid_Vars->OL_RPG);        // HERE!!!
        Matrix PMAP = Get_Interp_Matrix(MAP,PCID,Pos);

        // Store values
        Prb_Particle_CID.push_back(PCID);
        Prb_Particle_Map.push_back(PMAP);
        Psi_Grid->Set_Cell_Active(PCID);
    }
}

//--- Output prep

void Box::Prepare_Output_Data_Poisson_Dir()
{
    if (!ProActive) return;
    Psi_Grid->Trans_Om_Psi();
    Psi_Grid->Solve_Poisson();
    Psi_Grid->Calc_Finite_Differences();

    // Map grid to probes
    for (int p=0; p<Prb_Nodes.size(); p++)         // Loop over particles
    {
        // For each particle we extract the corresponding grid entries

        Cart_ID CID = Prb_Particle_CID[p];
        Matrix U = Psi_Grid->Get_U_Grid(CID, Grid_Vars->Mapping);
        Matrix S = Psi_Grid->Get_S_Grid(CID, Grid_Vars->Mapping);
        Vector3 pU = Prb_Particle_Map[p].transpose()*U;
        Vector3 pS = Prb_Particle_Map[p].transpose()*S;
        Vector6 I;  I << pU(0), pU(1), pU(2), pS(0), pS(1), pS(2);
        Influence.push_back(I);
    }
}

void Box::Prepare_Output_Data_BS3()
{
    if (!ProActive) return;
    Calc_L2P_BS_3D();
}

void Box::Calc_L2P_BS_3D()
{
    // Interpolate influence from base boxes onto nodes
    Real Scalefac = 2.0/Grid_Vars->H_ML;                          // Requires scaling

    Matrix U = ML_Inf;                 // Extract vel field
    Matrix IntU = Prb_IntMat*U;        // Interpolated U
    Matrix NabU = Matrix::Zero(Grid_Vars->N_Int,9);

    NabU.block(0,0,Grid_Vars->N_Int,3) = Scalefac*Grid_Vars->DDXS*U;
    NabU.block(0,3,Grid_Vars->N_Int,3) = Scalefac*Grid_Vars->DDYS*U;
    NabU.block(0,6,Grid_Vars->N_Int,3) = Scalefac*Grid_Vars->DDZS*U;

    Matrix IntNabU = Prb_IntMat*NabU;  // Interpolated Nab U

    // Specify stretching for each particle
    int NP = Prb_Nodes.size();
    Matrix IntStretch = Matrix::Zero(NP,3);
    for (int p=0; p<NP; p++)
    {
        Matrix3 NabU = Matrix3::Zero();
        NabU <<     IntNabU(p,0), IntNabU(p,1), IntNabU(p,2),  \
                    IntNabU(p,3), IntNabU(p,4), IntNabU(p,5),  \
                    IntNabU(p,6), IntNabU(p,7), IntNabU(p,8) ;
        Vector3 Omega(Prb_Nodes[p](3),Prb_Nodes[p](4),Prb_Nodes[p](5));
        Vector3 FFStretch;
        switch (Vars->Stretching_Scheme)
        {
            case CLASSIC:       FFStretch = Omega.transpose()*NabU;                         break;  // Classic
            case TRANSPOSE:     FFStretch = Omega.transpose()*NabU.transpose();             break;  // Transpose
            case MIXED:         FFStretch = Omega.transpose()*0.5*(NabU+NabU.transpose());  break;  // Mixed
            default: ;
        }
        FFStretch *= Prb_Nodes[p](7);     // Convert to Circulation!
        IntStretch.row(p) = FFStretch;
    }

    Matrix MOut = Matrix::Zero(NP,6);
    MOut.block(0,0,NP,3) = IntU;
    MOut.block(0,3,NP,3) = IntStretch;
    StdAppend(FF_Inf,Conv_Mat_to_SVec(MOut));
}

void Box::Calc_L2P_PSI_3D_G()
{
    // Interpolate influence from base boxes onto nodes
    StdAppend(FF_Inf,Conv_Mat_to_SVec(Prb_IntMat*ML_Inf));
}

void Box::Calc_L2P_PSI_3D()
{
    // Interpolate influence from base boxes onto nodes
    Matrix E_FF_Inf = Grid_Vars->PJL_WallSrc2Vol*ML_Inf;
    StdAppend(FF_Inf,Conv_Mat_to_SVec(E_FF_Inf));
}

void Box::Calc_L2P_PSI_2D()
{
    // Interpolate influence from base boxes onto nodes

    int NOm = Psi_Grid->N_BPoints;
    FF_Inf.assign(NOm,Vector3::Zero());
}

void Box::Map_Grid_to_Probes(const OUTVAR &Output)
{
    // Mapping the finite difference results back to the probes

    for (int p=0; p<Prb_Nodes.size(); p++)         // Loop over particles
    {
        // For each particle we extract the corresponding grid entries

        Cart_ID CID = Prb_Particle_CID[p];

        switch (Output)
        {
            case (VEL):
            {
                Matrix U = Psi_Grid->Get_U_Grid(CID, Grid_Vars->Mapping);
                Matrix S = Psi_Grid->Get_S_Grid(CID, Grid_Vars->Mapping);
                Vector3 pU = Prb_Particle_Map[p].transpose()*U;
                Vector3 pS = Prb_Particle_Map[p].transpose()*S;
                Vector6 I;  I << pU(0), pU(1), pU(2), pS(0), pS(1), pS(2);
                Influence.push_back(I);
                break;
            }
            case (PSI):
            {
                Matrix Psi = Psi_Grid->Get_Psi_Grid(CID, Grid_Vars->Mapping);
                Vector3 pPsi = Prb_Particle_Map[p].transpose()*Psi;
                Influence.push_back(pPsi);
                break;
            }
            case (OMEGA):
            {
                Matrix GradF = Psi_Grid->Get_Omega_Grid(CID,Grid_Vars->Mapping);
                Vector3 pGradF = Prb_Particle_Map[p].transpose()*GradF;
                Influence.push_back(pGradF);
                break;
            }
            case (NUSGS):
            {
                Matrix GradF = Psi_Grid->Get_SGS_Grid(CID,Grid_Vars->Mapping);
                Vector3 pGradF = Prb_Particle_Map[p].transpose()*GradF;
                Influence.push_back(pGradF);
                break;
            }
            default: break;
        }
    }
}

void Box::Magnitude_Filtering(const Real &GammaMin)
{
    // This uses the reference value (global field) and removes weak particles.
    // The strength is redistributed the the other particles in this box.

    // First seperate the strong and weak particles

    StateVector StrongNds;      //,WeakNds;
    Real Gam_Tot = 0.0;
    Vector3 Pos_C = Vector3::Zero();        //, Gam_C = Vector3::Zero();

    for (int i=0; i<Src_Nodes.size();i++)
    {
        Vector3 Gamma = Node_Gamma(Src_Nodes[i]);
        Real GamNorm = Gamma.norm();
        bool Filt = false;

        if (GamNorm < GammaMin) Filt = true;

        if (Vars->Spat_Mag_Filt)            // Additional spatial flag
        {
            Vector3 Pos = Node_Pos(Src_Nodes[i]);
//            if (sqrt(Pos(1)*Pos(1) + Pos(2)*Pos(2)) < 2.25) Filt = false;
//            if (Pos(0)<Vars->Spat_Mag_Fac)                                  Filt = true;
//            if (Pos(0) > Vars->T*Vars->Freestream(0)+20*Grid_Vars->H_Grid)  Filt = true;
//            if (Pos(2) < 0.0)  Filt = true;     // Free jet
        }

        if (Filt)
        {
            Gam_Tot += GamNorm;
            Pos_C += GamNorm*Node_Pos(Src_Nodes[i]);
            Gamma_Rest += Gamma;
        }
        else    StrongNds.push_back(Src_Nodes[i]);
    }

    if (!StrongNds.empty())     // Add vorticity to other members
    {
        Vector3 Gam_C_Frac = Gamma_Rest/(StrongNds.size()*Grid_Vars->Vol_Char);
        for (int i=0; i<StrongNds.size(); i++)
        {
            StrongNds[i](3) += Gam_C_Frac(0);
            StrongNds[i](4) += Gam_C_Frac(1);
            StrongNds[i](5) += Gam_C_Frac(2);
        }
        Gamma_Rest.setZero();
    }
    else   Src_Nodes.clear();

    // Replace source list
    StdClearAppend(Src_Nodes, StrongNds);
}

//--- Remeshing

void Box::Remesh_Box(const GRID_MAP &MAP)
{
    // This is a new function which maps directly the data here to a much simpler grid (not with an Eulerian grid)

    // Specify grid size
    Real HG = Grid_Vars->H_Grid;
    Real HML = Grid_Vars->H_ML;
    int NG1 = Grid_Vars->N_ML;
    if (MAP==M2)      NG1 += 2;
    if (MAP==M4D)     NG1 += 4;
    int NG2 = NG1*NG1;
    int NG3 = NG2*NG1;

    // Specify grid corner
    Vector3 MeshCorn;
    if (MAP==M0)      MeshCorn = Vector3(CID(0)*HML,CID(1)*HML,CID(2)*HML)+Vector3(0.5*HG,0.5*HG,0.5*HG);
    if (MAP==M2)      MeshCorn = Vector3(CID(0)*HML,CID(1)*HML,CID(2)*HML)-Vector3(0.5*HG,0.5*HG,0.5*HG);
    if (MAP==M4D)     MeshCorn = Vector3(CID(0)*HML,CID(1)*HML,CID(2)*HML)-Vector3(1.5*HG,1.5*HG,1.5*HG);

    // Now loop over sources and map
    Matrix OmegaMesh = Matrix::Zero(NG3,3);
    for (int i=0; i<Src_Nodes.size(); i++)
    {
        Vector3 Pos = Node_Pos(Src_Nodes[i]);
        if (MAP==M0) Pos += Vector3(0.05*HG,0.05*HG,0.05*HG);               // Shift to ensure correct node capture
        Vector3 PosRel = Pos - MeshCorn;
        Cart_ID CIDM = Get_CID(PosRel,HG);
        Vector3 O = Node_Gamma(Src_Nodes[i])/Grid_Vars->Vol_Char;

        switch (MAP)
        {
            case (M0):
            {
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)  )*NG1 + (CIDM(2)  )) += O;
                break;
            }
            case (M2):
            {
                Vector3 CPosRel = PosRel - Vector3(CIDM(0)*HG,CIDM(1)*HG,CIDM(2)*HG);
                Vector3 CRel = CPosRel/HG;
                Vector M = M0_3D(CRel);
                int r=0;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)  )*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                break;
            }
            case (M4D):
            {
                Vector3 CPosRel = PosRel - Vector3(CIDM(0)*HG,CIDM(1)*HG,CIDM(2)*HG);
                Vector3 CRel = CPosRel/HG;
                Vector M = M1_3D(CRel);
                int r=0;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)-1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;

                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)  )*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)  )*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)  )*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;

                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+1)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;

                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)-1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)  )*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+1)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)-1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)  )) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+1)) += M(r)*O; r++;
                OmegaMesh.row((CIDM(0)+2)*NG2+(CIDM(1)+2)*NG1 + (CIDM(2)+2)) += M(r)*O; r++;
                break;
            }
            default:{ break;}
        }
    }

    // Now extract out real positions
    Src_Nodes.clear();

    for (int i=0; i<NG3; i++)
    {
        int IX = i/NG2;
        int IY= (i-IX*NG2)/NG1;
        int IZ= (i-IX*NG2-IY*NG1);
        Vector3 OmRow(OmegaMesh(i,0),OmegaMesh(i,1),OmegaMesh(i,2));
        if (OmRow.norm()==0.0) continue;
        Vector3 Pos = Vector3(IX*HG,IY*HG,IZ*HG) + MeshCorn + Grid_Vars->Origin;
        Vector8 ND; ND << Pos(0) , Pos(1) , Pos(2) , OmRow(0) , OmRow(1) , OmRow(2) , Grid_Vars->Sigma_Char , Grid_Vars->Vol_Char;
        Src_Nodes.push_back(ND);
    }

}

void Box::Clear_Remesh_Data()
{
    Src_Nodes.clear();
    Src_ID.clear();
    Omega_Grid->Reset_Arrays();
}

//--- Divergence Filtering

void Box::Divergence_Filtering_Step1()
{
    // This is the first process carried out. Vorticity is assigned to the grid.
    // The source points act as probes. We solve for the divergence correction and map back

    Create_Psi_Grid();          // Ensure Psi Grids allocated
    Map_Sources_to_Eul_Grid(Grid_Vars->Mapping,true,false,true,false); // Ensure mapping correctly.
    Set_Probe_Map(Grid_Vars->Mapping);
}

void Box::Divergence_Filtering_Step2()
{
    // Vorticity overlap has been specified, so we need to carry out the checks here for the divergence of the field

    Psi_Grid->Set_Node_Activity(Grid_Vars->Mapping);    // Unsure why we need this? Necessary??
    Psi_Grid->Calc_Omega_Div();                         // Calculate the divergence of vorticity on the grid

    // Do we wish to carry this out?

//    Real DOMax = Psi_Grid->Div_Om_Max;
//    Real DivOmMax = 0;
//    Real DivOmMax = Vars->DivOmMax;
//    if (DOMax>DivOmMax) DivOmMax = DOMax;
//    Real Val = DOMax*Grid_Vars->H_Grid/Vars->Vort_Max;

    Psi_Grid->Solve_Poisson(0);             // Solve for the divergence variable
    Psi_Grid->Project_Omega();              // Cast the gradient back to the vorticity grid

    for (int p=0; p<Prb_Nodes.size(); p++)         // Loop over particles
    {
        // For each particle we extract the corresponding grid entries

        Cart_ID CID = Prb_Particle_CID[p];
        Matrix GradF = Psi_Grid->Get_Omega_Grid(CID,Grid_Vars->Mapping);
        Vector3 pGradF = Prb_Particle_Map[p].transpose()*GradF;
        Influence.push_back(pGradF);
    }

    // Now update nodes
    StateVector NewNodes;

    for (int i=0; i<Prb_Nodes.size(); i++)
    {
        Vector3 Pos(Prb_Nodes[i](0),Prb_Nodes[i](1),Prb_Nodes[i](2));
        Vector3 Om(Prb_Nodes[i](3)-Influence[i](0),
                   Prb_Nodes[i](4)-Influence[i](1),
                   Prb_Nodes[i](5)-Influence[i](2));
        NewNodes.push_back(Create_Particle(Pos,Om));
    }

    StdClearAppend(Prb_Nodes,NewNodes);     // Switch out arrays
}

//--- Functions for the Poisson monolithic

void Box::Set_BC_Poisson_Mono(Eul_Grid *Mono, const int &I)
{
    // This function writes the value of the streamfunction into the Psi grid. Depending on the wall, the position and value
    // is automatically correctly specified

    int DX = CID(0)*Grid_Vars->N_ML - Mono->X_L;
    int DY = CID(1)*Grid_Vars->N_ML - Mono->Y_L;
    int DZ = CID(2)*Grid_Vars->N_ML - Mono->Z_L;

    int NML  = Grid_Vars->Poly_ML;
    int NML2 = NML*NML;
    int NES  = Grid_Vars->N_ML+1;
    int NES2 = NES*NES;

    switch (I)
    {
        case 1:     // Wall X1
        {
            Matrix FFWall = ML_Inf.block(0,0,NML2,3);
            Matrix FFWallInt = Grid_Vars->PMono_W2W*FFWall;
            for (int j=0; j<NES; j++)
            {
                for (int k=0; k<NES; k++)   Mono->Increment_Psi_Node(Cart_ID(0,DY+j,DZ+k),          FFWallInt.row(j*NES+k));
            }
            break;
        }
        case 2:     // Wall X2
        {
            Matrix FFWall = ML_Inf.block(NML2*(NML-1),0,NML2,3);
            Matrix FFWallInt = Grid_Vars->PMono_W2W*FFWall;
            for (int j=0; j<NES; j++)
            {
                for (int k=0; k<NES; k++)   Mono->Increment_Psi_Node(Cart_ID(Mono->nx,DY+j,DZ+k),   FFWallInt.row(j*NES+k));
            }
            break;
        }
        case 3:     // Wall Y1
        {
            Matrix FFWall = Matrix::Zero(NML2,3);
            for (int i=0; i<NML; i++)
            {
                for (int k=0; k<NML; k++)   FFWall.row(i*NML+k) = ML_Inf.row(i*NML2+k);
            }
            Matrix FFWallInt = Grid_Vars->PMono_W2W*FFWall;
            for (int i=0; i<NES; i++)
            {
                for (int k=0; k<NES; k++)   Mono->Increment_Psi_Node(Cart_ID(DX+i,0,DZ+k),          FFWallInt.row(i*NES+k));
            }
            break;
        }
        case 4:     // Wall Y2
        {
            Matrix FFWall = Matrix::Zero(NML2,3);
            for (int i=0; i<NML; i++)
            {
                for (int k=0; k<NML; k++)   FFWall.row(i*NML+k) = ML_Inf.row(i*NML2+NML*(NML-1)+k);
            }
            Matrix FFWallInt = Grid_Vars->PMono_W2W*FFWall;
            for (int i=0; i<NES; i++)
            {
                for (int k=0; k<NES; k++)   Mono->Increment_Psi_Node(Cart_ID(DX+i,Mono->ny,DZ+k),   FFWallInt.row(i*NES+k));
            }
            break;
        }
        case 5:     // Wall Z1
        {
            Matrix FFWall = Matrix::Zero(NML2,3);
            for (int i=0; i<NML; i++)
            {
                for (int j=0; j<NML; j++)   FFWall.row(i*NML+j) = ML_Inf.row(i*NML2+NML*j);
            }
            Matrix FFWallInt = Grid_Vars->PMono_W2W*FFWall;
            for (int i=0; i<NES; i++)
            {
                for (int j=0; j<NES; j++)   Mono->Increment_Psi_Node(Cart_ID(DX+i,DY+j,0),          FFWallInt.row(i*NES+j));
            }
            break;
        }
        case 6:     // Wall Z2
        {
            Matrix FFWall = Matrix::Zero(NML2,3);
            for (int i=0; i<NML; i++)
            {
                for (int j=0; j<NML; j++)   FFWall.row(i*NML+j) = ML_Inf.row(i*NML2+j*NML+NML-1);
            }
            Matrix FFWallInt = Grid_Vars->PMono_W2W*FFWall;
            for (int i=0; i<NES; i++)
            {
                for (int j=0; j<NES; j++)   Mono->Increment_Psi_Node(Cart_ID(DX+i,DY+j,Mono->nz),    FFWallInt.row(i*NES+j));
            }
            break;
        }
        default: ;
    }
}

//--- Functions for the Poisson solver

void Box::Set_BC_PsiGrid()
{
    // This function specifies the boundary condition for the solver
    StateVector BCA;
    for (int i=0; i<FF_Inf.size(); i++) BCA.push_back(FF_Inf[i]+NF_Inf[i]);
    Psi_Grid->Set_BC(BCA);
//    for (int i=0; i<FF_Inf.size(); i++)
//    qDebug() << FF_Inf[i](0) << FF_Inf[i](1) << FF_Inf[i](2) << NF_Inf[i](0) << NF_Inf[i](1) << NF_Inf[i](2);
}

//void Box::Create_Eul_Grids()
//{
//    // This checks if the box has an Eulerian grid.

//    if (Get_Branch()>0)          return;     // Only need to generate Eul Grid for base boxes!
//    if (Omega_Grid!=nullptr)     return;     // Already allocated. Need not repeat!

//    // Overview:
//    // Omega_Grid is the grid onto which the vorticity will be mapped
//    // Psi_Grid is the grid on which the finite difference calculation will be carried out (Solution is stored there)
//    // If we are using the James-Lackner algorithm these are two different grids, as the free-space grid requires a different
//    // overlap parameter, if we are using the standard poisson solver however the two coincide.

//    //--- Create Omega grid: Here the vorticity will be stored on the regular grid
//    Omega_Grid = Create_Grid(CID,Grid_Vars->OL_OG);  // We have no need for the Psi Grid

//    if (Vars->Solver==POISSON_JL)
//    {
//        Psi_Grid = Create_Grid(CID,Grid_Vars->OL_PG);             // Grid on which finite differences will be carried out
//        Omega_Grid->BG = MatrixRM::Zero(Omega_Grid->N_BPoints,3);     // Allocate boundary gradient matrix
//        Omega_Grid->BPsi0 = MatrixRM::Zero(Psi_Grid->N_BPoints,3);    // Allocate psi0 matrix
//    }
//    else    Psi_Grid = Omega_Grid;
//}

void Box::Create_Psi_Grid()
{
    // This function creates the eulerian grid object. Under normal circumstances only a Psi Grid will be allocated.
    // If the Poisson JL method is being used, a second grid is allocated (Create_Omega_Grid). This is the grid used to
    // calculate the homogeneous solution
    if (Get_Branch()>0)         return;     // Only need to generate Eul Grid for base boxes!
    if (Psi_Grid!=nullptr)      return;     // Already allocated. Need not repeat!
    Psi_Grid = Create_Grid(CID,Grid_Vars->OL_PG);             // Grid on which finite differences will be carried out
}

void Box::Create_Omega_Grid()
{
    //This is the grid used to calculate the homogeneous solution

    if (Get_Branch()>0)          return;     // Only need to generate Eul Grid for base boxes!
    if (Omega_Grid!=nullptr)     return;     // Already allocated. Need not repeat!
    Omega_Grid = Create_Grid(CID,Grid_Vars->OL_OG);  // We have no need for the Psi Grid
}

SP_EGrid Box::Create_Grid(const Cart_ID &OSuper, const int &O)
{
    // Generic function for creating a box and assigning it to a leaf
    //    SP_EGrid B = std::make_shared<Box>(TID,CID,Branch);
//    Cart_ID Overlap(O,O,O);

    // Domain corner point (local CS)

    Cart_ID Osub;       // Origin of this Eulerian grid
    Cart_ID N;          // Dimensions of this Eulerian grid

    if (Vars->Dim==SIM_2D)
    {
        Osub(0) = OSuper(0)*Grid_Vars->ES-O;
        Osub(1) = OSuper(1)*Grid_Vars->ES-O;
        Osub(2) = 0;

        N(0) = Grid_Vars->ES + 2*O;
        N(1) = Grid_Vars->ES + 2*O;
        N(2) = 1;

        return std::make_shared<Eul_Grid_2D>(OSuper,Osub,N);
    }
    if (Vars->Dim==SIM_3D)
    {
        Osub(0) = OSuper(0)*Grid_Vars->ES-O;
        Osub(1) = OSuper(1)*Grid_Vars->ES-O;
        Osub(2) = OSuper(2)*Grid_Vars->ES-O;

        N(0) = Grid_Vars->ES + 2*O;
        N(1) = Grid_Vars->ES + 2*O;
        N(2) = Grid_Vars->ES + 2*O;

        return std::make_shared<Eul_Grid_3D>(OSuper,Osub,N);
    }

//    return E_Grid;
}

}
