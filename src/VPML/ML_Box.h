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

#ifndef Box_H
#define Box_H

#include "Math_Types.h"
#include "Octtree.h"
#include "ML_Euler.h"
//---------------------------------------------------------

namespace VPML
{


inline Vector3      Node_Pos(const Cart_ID &ID)         {return Vector3(ID(0),ID(1),ID(2))*Grid_Vars->H_Grid;}
inline Vector3      Node_Pos_Global(const Cart_ID &ID)  {return Node_Pos(ID) + Grid_Vars->Origin;}
inline void         Create_Surrounding_Grid(Vector3 &Pos, StateVector &S)
{
    // This is just a testing function to create a grid around the given particle @ Pos
    Cart_ID PID;    // = Get_CartID(Pos);

    if (Grid_Vars->Mapping==M2)
    {
        for (int i=0; i<=1; i++)
        {
            for (int j=0; j<=1; j++)
            {
                for (int k=0; k<=1; k++)
                {
                    Vector8 VP = Vector8::Zero();
                    VP(0) = (PID(0)+i)*Grid_Vars->H_Grid;
                    VP(1) = (PID(1)+j)*Grid_Vars->H_Grid;
                    VP(2) = (PID(2)+k)*Grid_Vars->H_Grid;
                    S.push_back(VP);
                }
            }
        }
    }

    if (Grid_Vars->Mapping==M4D)
    {
        for (int i=-1; i<=2; i++)
        {
            for (int j=-1; j<=2; j++)
            {
                for (int k=-1; k<=2; k++)
                {
                    Vector8 VP = Vector8::Zero();
                    VP(0) = (PID(0)+i)*Grid_Vars->H_Grid;
                    VP(1) = (PID(1)+j)*Grid_Vars->H_Grid;
                    VP(2) = (PID(2)+k)*Grid_Vars->H_Grid;
                    S.push_back(VP);
                }
            }
        }
    }

}

//--------------Mappings to Eulerian Grid---------------

inline void Shift_Pos(Vector &V)
{
    Real SHF = 0.5*Grid_Vars->H_Grid;
    V(0) += SHF;
    V(1) += SHF;
    if (Vars->Dim==SIM_3D)  V(2) += SHF;
}

inline void Shift_Neg(Vector &V)
{
    Real SHF = 0.5*Grid_Vars->H_Grid;
    V(0) -= SHF;
    V(1) -= SHF;
    if (Vars->Dim==SIM_3D)  V(2) -= SHF;
}

//------------------------------
//--- General particle functions
//------------------------------

inline Vector8      Create_Particle(const Vector3 &P, const Vector3 &A, const Real &S, const Real &V)
{
    Vector8 Vout;
    Vout << P(0), P(1), P(2), A(0), A(1), A(2), S, V;
    return Vout;
}
inline Vector8      Create_Particle(const Vector3 &P, const Vector3 &A) {return Create_Particle(P,A,Grid_Vars->Sigma_Char, Grid_Vars->Vol_Char);}
inline Vector8      Mirror_X_Particle(const Vector &P) {Vector8 V = P; V(0)*=-1; V(4)*=-1; V(5)*=-1; return V;}
inline Vector8      Mirror_Y_Particle(const Vector &P) {Vector8 V = P; V(1)*=-1; V(3)*=-1; V(5)*=-1; return V;}

typedef std::vector<uint> TreeID;   // Tree ID Type // Re-declare treeid here

class Box //  : public Tree_Node<Box>
{

protected:

    TreeID TID;          // Box tree ID
    Cart_ID CID;        // Box cartesian ID
    uint Branch;

    //--- Node access functions

    Vector3  Node_Pos(const Vector &V)              {return Vector3(V(0),V(1),V(2)) - Grid_Vars->Origin;}
    Vector3  Node_Vort(const Vector &V)             {return Vector3(V(3),V(4),V(5));}
    Vector3  Node_Gamma(const Vector &V)            {return Node_Vort(V)*V(7);}
    Cart_ID  Get_CartID(const Vector3 &V);
    Cart_ID  Get_CartID(const Vector3 &V, const Real &H);
    Cart_ID  Get_CartID_Safe(const Vector3 &V, const Cart_ID &CID_Eul, const int &OLL, const int &OLR);

    //--- Mapping functions

    Vector       M0_2D(const Vector3 &PR);
    Vector       M1_2D(const Vector3 &PR);
    Vector       M0_3D(const Vector3 &PR);
    Vector       M1_3D(const Vector3 &PR);
    Vector       Get_Interp_Matrix(const GRID_MAP &MAP, const Cart_ID &PCID, const Vector3 &Pos);

    //------------ Barycentric variables and interpolants -----------------

    void    Calc_Li(const int &Ns, const Vector &Xs, const Vector &Ws, const Real &x, Real *Li);
    void    Set_BL_Interp_Mat_2D(const int &Ns, const Vector &Xs, const Vector &Ws, const Matrix &Rel_pos, Matrix &Int_mat);
    void    Set_BL_Interp_Mat_3D(const int &Ns, const Vector &Xs, const Vector &Ws, const Matrix &Rel_pos, Matrix &Int_mat);

public:

    //--- Constructors

    Box()       {}
    Box(const TreeID &m_ID)   {TID = m_ID;}
    Box(const Cart_ID &C_ID, const uint Br=0)   {CID = C_ID; Branch = Br;}
    Box(const TreeID &tID, const Cart_ID &cID, const uint br=0)
    {
        TID = tID;
        CID = cID;
        Branch = br;
    }
    Box(const int &X, const int &Y, const int &Z=0)
    {
        CID(0) = X;
        CID(1) = Y;
        CID(2) = Z;
    }

    //--- Source variables

    //--- Vars
    StateVector         Src_Nodes;          // Sources
    StateVector         Spat_Src_Nodes;     // Spatial source nodes
    std::vector<int>    Src_ID;             // Source IDs

    //--- Funcs

    Matrix              Get_Rel_Position(const StateVector &P, const Vector3 &Ref, const Real &H);
    void                Get_Probe_Position(Matrix &M);

    //--- Probe variables

    //--- Vars
    StateVector         Prb_Nodes;          // Probes
    std::vector<int>    Prb_ID;             // Probe global position
    Matrix              Prb_IntMat;         // In/Anterpolation matrix sources

    //--- Funcs

    int X()             {return CID(0);}
    int Y()             {return CID(1);}
    int Z()             {return CID(2);}

    //---  Activity registers

    bool Active = false;
    bool ProActive = false;
//    bool Periodic_Active = false;
//    bool NBProActive = false;

    bool ML_Active = false;
    bool ML_ProActive = false;

    TreeID      Get_TID()                   {return TID;}
    Cart_ID     Get_BoxCID()                {return CID;}
    int         Get_Branch()                {return Branch;}
    Vector3     Get_Centre();
    Matrix      Get_Corners();

    //--- Source and probe handling

    void        Add_Source(const Vector &V, const int &i)
    {
            Src_Nodes.push_back(V);
            Src_ID.push_back(i);             // For later identification
    }
    void        Add_Source(const Vector &V, const int &i, const Vector3 &Orig)
    {
            Vector VM = V;
            VM(0) -= Orig(0);
            VM(1) -= Orig(1);
            VM(2) -= Orig(2);
            Src_Nodes.push_back(VM);
            Src_ID.push_back(i);             // For later identification
    }
    void        Add_Probe(const Vector &V, const int &i)
    {
            Prb_Nodes.push_back(V);
            Prb_ID.push_back(i);             // For later identification
    }

    void    Magnitude_Filtering(const Real &GammaMin);
    Vector3 Gamma_Rest = Vector3::Zero();

    Matrix  Get_Omega_Array();

    //--- Interpolation nodes

//    std::vector<SPNode> Int_Nodes;

    // Near-field variables
    int IDNF = 0;
//    int NTiles = 0;
    std::vector<int> Src_Tiles;
//    std::vector<int> Prb_Tiles;

    //--- Influence variables
    StateVector NF_Inf;                 // Probes
    StateVector FF_Inf;                 // Probes
    Matrix      ML_Src;                 // Influence vector
    Matrix      ML_Inf;                 // Influence vector

    int         Get_Octant()    {return TID[Tree_Lim-Branch];}

    Matrix              Get_Source_Strength();

    //--- Variables for the Poisson solver

    Matrix      NF_Inf_Mat;             // Base box influence
    SP_EGrid Omega_Grid = nullptr;                 // Pointer to the Eulerian grid of this box (Poisson solver)
    SP_EGrid Psi_Grid = nullptr;                // Pointer to the Eulerian grid of this box (Poisson JL -> Require additional grid for source term)
    std::vector<Cart_ID>    Prb_Particle_CID;   // This is the cartesian ID of the given particle (global)
    std::vector<Vector>     Prb_Particle_Map;   // The map to the neighboring nodes

    //--- Variables for the Monolithic Poisson solver

    bool FaceX1 = false, FaceX2 = false, FaceY1 = false, FaceY2 = false, FaceZ1 = false, FaceZ2 = false;

    //--- Clear data

    void        Clear_Source_Data()
    {
        // Clears the source data of the box.
        Active = false;
        ML_Active = false;
//        Periodic_Active = false;
        Src_Nodes.clear();
        Spat_Src_Nodes.clear();
        ML_Src.resize(0,0);
        Src_ID.clear();
        Src_Tiles.clear();
        IDNF = 0;
//        NSpatSrc = 0;

        Gamma_Rest.setZero();

        // Clears source grid data
        if (Omega_Grid)    Omega_Grid->Reset_Arrays();
        if (Psi_Grid)      Psi_Grid->Reset_Arrays();
    }

    void        Clear_Probe_Data()
    {
        // Clears the probe data of the box.
        ProActive = false;
        ML_ProActive = false;
        Prb_Nodes.clear();
        Prb_ID.clear();
        ML_Inf.resize(0,0);
        NF_Inf.clear();
        FF_Inf.clear();
//        Prb_Tiles.clear();

        // Clears probe grid data
        Prb_Particle_CID.clear();
        Prb_Particle_Map.clear();
        Influence.clear();

        // Clears probe grid data
        if (Omega_Grid)    Omega_Grid->Reset_Arrays();
        if (Psi_Grid)      Psi_Grid->Reset_Arrays();
    }

    void        Clear_Probe_Grid_Data()
    {
        Prb_Particle_CID.clear();
        Prb_Particle_Map.clear();
        Influence.clear();
    }

    // Influence variables
    StateVector Influence;             // Probes

    //--- Functions for the Poisson solver

    void    Set_BC_PsiGrid();

    //--- Functions for the Poisson monolithic

    void    Set_BC_Poisson_Mono(Eul_Grid *Mono, const int &I);

    //--- Functions to localise the data prep

    //--- Source Prep

//    void Prepare_Source_Data();
    void Prepare_Source_Data_BS3();
    void Prepare_Source_Data_Poisson_Dir();
    void Prepare_Source_Data_Poisson_JL();
//    void Set_Spatial_Source_Cells();
//    void Nullify_Sources();
    void Map_Sources_to_ML_Grid();
    void Map_Sources_to_Eul_Grid(const GRID_MAP &MAP, bool IncPsi, bool IncOmega, bool SetActive, bool Prbs_Are_Srcs = false);
    void M2Anterp();

    //--- Probe Prep

//    void Prepare_Probe_Data();
    void Prepare_Probe_Data_BS3();
    void Prepare_Probe_Data_Poisson_Dir();
    void Map_Probes_to_ML_Grid();
    void Set_Probe_Map(const GRID_MAP &MAP);

    //--- Output Prep

//    void Prepare_Output_Data();
    void Prepare_Output_Data_BS3();
    void Prepare_Output_Data_Poisson_Dir();
    void Map_Grid_to_Probes(const OUTVAR &Output);
//    void Calc_L2P();
    void Calc_L2P_BS_3D();
    void Calc_L2P_PSI_3D_G();
    void Calc_L2P_PSI_3D();
    void Calc_L2P_PSI_2D();

    //--- Remeshing

    void Remesh_Box(const GRID_MAP &MAP);
//    void Remesh_Step1();
//    void Remesh_Step2();
    void Clear_Remesh_Data();

    //--- Divergence Filtering

    void Divergence_Filtering_Step1();
    void Divergence_Filtering_Step2();

    // Grid functions

//    void        Create_Eul_Grids();
    void        Create_Omega_Grid();
    void        Create_Psi_Grid();
    void        Create_Eul_Grids();
    SP_EGrid    Create_Grid(const Cart_ID &OSuper, const int &O);
};

typedef std::shared_ptr<Box> SPBox;

typedef std::vector<SPBox>      SPBoxList;

}
//class ML_Box : public
#endif // Box_H
