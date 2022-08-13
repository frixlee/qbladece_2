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

#ifndef ML_Euler_H
#define ML_Euler_H

#include "VPML_Gobal_Vars.h"

#ifdef OPENCL
    #include "Opt_OpenCL.h"        // Include opencl header
#endif

#ifdef MKLFastPoiss
    #include "mkl_poisson.h"        // Include mkl header
#endif

namespace VPML
{
// Finite Difference Stencils:

//--- Laplacian stencils

const Real L21 = 1.0/6.0, L22 = 4.0/6.0, L23 = -20.0/6.0;

static StdVector const LAP_ISO_2D =  {L21, L22, L21,   L22, L23, L22,  L21, L22, L21};

//const Real L1 = 2.0/30.0, L2 = 1.0/30.0, L3 = 18.0/30.0, L4 = -136.0/30.0;    // Cocle 2008
//const Real L1 = 0.0, L2 = 1.0/6.0, L3 = 1.0/3.0, L4 = -4.0;                   // Patra 2006 - variant 2 (compact)
const Real L1 = 1.0/30.0, L2 = 3.0/30.0, L3 = 14.0/30.0, L4 = -128.0/30.0;    // Patra 2006 - variant 5

static StdVector const LAP_ISO_3D =  {L1,L2,L1,L2,L3,L2,L1,L2,L1,      L2,L3,L2,L3,L4,L3,L2,L3,L2,     L1,L2,L1,L2,L3,L2,L1,L2,L1};

static RowVector D1_CD = []{RowVector V = RowVector(3); V << -0.5, 0, 0.5; return V; }();
static RowVector D1_OSL = []{RowVector V = RowVector(3); V << -1.5, 2, -0.5; return V; }();
static RowVector D1_OSR = []{RowVector V = RowVector(3); V << 0.5, -2, 1.5; return V; }();

//static RowVector LAP_ISO_2D = []{Matrix V = Matrix::Zero(1,9); V << 1,4,1,4,-20,4,1,4,1; V *= 1/6.0; return V; }();

static int MPS02X[2] = {0,1};
static int MPS12X[4] = {-1,0,1,2};

// The Eulerian domain stores essentially arrays of Reals and operates on them to calc finite differences
// Accessing the values is carried out depending on the type of implementation

class Eul_Grid //  : public ML_Box
{
protected:

    //--- Finite difference calcs

    Real            H2;
    Real            InvH;
    Real            InvH2;
    Real            InvH3;
    bool            OutputArrays=false;

    //--- Variables MKL solver ---

    Real ax=0, bx=0;                    // Domain limits xmax, xmin etc
    Real ay=0, by=0;
    Real az=0, bz=0;
    Real q;                             // RHS factor (zero for Poisson equation)
    Real *spar;                         // Intermediate var for Helmholtz solver
    Real *f, *f0;                       // RHS of poisson equation. (Vorticity values)
    char *BCtype;                       // Identification of boundary conditions

    #ifdef MKLFastPoiss
        MKL_INT ipar[128];                  // Internal information for MKL solver
        MKL_INT stat;                       // status of calculation
        DFTI_DESCRIPTOR_HANDLE xhandle;// = 0; // FFT Handle
        DFTI_DESCRIPTOR_HANDLE yhandle = 0; // FFT Handle
    #else
        uint ipar[128];
        uint stat;
    #endif

    //------------------------------

    //--- Solution arrays ---

    Real **Psi;
    Real **Omega;
    Real **U;
    Real **DAlphaDt;
    Real **LapOmega;                // Laplacian of Omega

    // Strain rates
    Real *Nu_SGS;                   // RVM Turb model: Sub-grid scale viscosity
    Real **Omega_SS;                // RVM Turb model: Small-scale vorticity
    Real **Omega_SS2;               // Filtered out vorticity

    //--- Markers for array activity

    bool Psi_Active         = false;
    bool Omega_Active       = false;
    bool U_Active           = false;
    bool DAlphaDt_Active    = false;
    bool LapOmega_Active    = false;

    bool Nu_SGS_Active      = false;
    bool Omega_SS_Active    = false;

    bool BC_Active          = false;

    int Last_PA_Timestep = Vars->Timestep;

    void Reset_Array(Real *A, const int &N)     {memset(A, 0, N*sizeof(Real));}
//    void Reset_Array2D(Real **A) {Reset_Array(A[2]);}
    void Reset_Array3D(Real **A, const int &N)  {for (int i=0; i<3; i++) Reset_Array(A[i],N);}

    //-----------------------




    //--- Set Boundary Condition ---

    virtual void Set_BC_Positions() {}

    virtual void Get_BC_Area_Ratio(StdVector &dA) {}

    //-- Auxiliary functions --

    int Factorial(int n);

    //--- Glob ID Access ---

//    uint Cell_ID_3D(const Cart_ID &ID)                          {return ID(0)*nx*ny + ID(1)*ny + ID(2);}

    //--- MKL ID Access ---

    uint ID_MKL_2D(const int &i, const int &j)                   {return i+j*(nx+1);}
    uint ID_MKL_2D_PX(const int &i, const int &j)                {return IDPX(i)+j*(nx+1);}
    uint ID_MKL_2D_PY(const int &i, const int &j)                {return i+IDPY(j)*(nx+1);}
    uint ID_MKL_2D_PX_PY(const int &i, const int &j)             {return IDPX(i)+IDPY(j)*(nx+1);}
    uint ID_MKL_3D(const int &i, const int &j, const int &k)     {return i+j*(nx+1)+k*(nx+1)*(ny+1);}
    uint ID_MKL_3D_PX(const int &i, const int &j, const int &k)
    {
        if      (i<0)   return ID_MKL_3D(i+nx, j, k);
        else if (i>nx)  return ID_MKL_3D(i-nx, j, k);
        else            return ID_MKL_3D(i, j, k);
    }

    virtual uint ID_MKL(const int &i, const int &j)                 {}
    virtual uint ID_MKL(const int &i, const int &j, const int &k)   {}
    virtual uint ID_MKL(const Cart_ID &ID)                          {}

    virtual uint ID_CELL(const Cart_ID &ID)                         {}

    int IDPX(const int &i);
    int IDPY(const int &j);
    int IDPZ(const int &k);

    //--- Solution steps ---

    virtual void    Init_Solver()      {}
    virtual void    Commit_Arrays()    {}
    virtual void    Execute_Solver()   {}

    //--- Extract grid values

    void Get_Grid_2D(Matrix &M, const int &GX, const int &GY, const int &SL, const int &SR, Real **Vec);
//    void Get_Grid_2D_Periodic(Matrix &M, const int &GX, const int &GY, const int &SL, const int &SR, Real **Vec);

public:

    //--- Constructors ---

//    Eul_Grid()    {}        // Empty
    Eul_Grid(const Cart_ID &OSuper, const Cart_ID &OSub, const Cart_ID &N);

    //--- Cart IdS ---
    Cart_ID ID_Super;
    Cart_ID Origin_Glob;                // Domain corner point. x=y=z = 0

    //--- Grid Functions ---

    virtual void    Init_Grid()     {}
    virtual void    Solve_Poisson() {}
    virtual void    Solve_Poisson(const int &Dim)   {}
    virtual void    Reset_Arrays()          {}
    virtual void    Reset_Omega()          {}
    virtual void    Reset_Psi()             {}

    //--- Grid Activity ---

    void    Set_Psi_Active()        {Psi_Active = true;}
    void    Set_Omega_Active()      {Omega_Active = true;}
    void    Set_U_Active()          {U_Active = true;}
    void    Set_DADt_Active()       {DAlphaDt_Active = true;}
    void    Set_LapOmega_Active()   {LapOmega_Active = true;}
    void    Set_Nu_SGS_Active()     {Nu_SGS_Active = true;}
    void    Set_Omega_SS_Active()   {Omega_SS_Active = true;}
    void    Set_BC_Active()         {BC_Active = true;}

    //--- Poisson JL Functions ---

    virtual void Set_Boundary_Gradient()                    {}
    virtual void Set_Boundary_Psi0()                        {}
    virtual void PoissonJL_Compact_Support()                {}
    StateVector Get_Boundary_Sources();
//    virtual void Receive_Omega_Parent_Grid(const std::vector<Cart_ID> ID, const std::vector<int> IDMKLP, Real **Omega_P) {}

    //--- Activity

    virtual void    Set_Cell_Active(const Cart_ID &CID)             {}
    virtual void    Mark_Surr_Node_Activity(const Cart_ID &CID)     {}
    virtual void    Set_Direct_Node_Activity()                      {}
    virtual void    Set_Node_Active(const int &IDX,const int &IDY,const int &IDZ)  {}
    virtual void    Set_Node_Activity(const GRID_MAP &Map) {}

    MatrixRM BG;                                        // Boundary gradient
    MatrixRM BPsi0;                                     // Boundary psi0 value
//    std::shared_ptr<Eul_Grid> Sub_Grid = nullptr;       // Sub_Grid

    virtual MatrixRM    BG_Face(const int &i)  {}
    virtual Vector3     BG_Face_Centre(const int &i)  {}
    virtual MatrixRM    BG_FacePos(const int &i) {}
    virtual void        BG_Face_Pos(const int &i, StateVector &B)     {}

    //--- Variables MKL solver ---

    int N_Points = 0;
    int N_BPoints = 0;
    int N_Cells = 0;

//    //--- Variables Multilevel ---
//    int NFID;      // Specifies near field index.

    //--- Omega Spec ---

    virtual void    Increment_Omega(const Cart_ID &CID, const Matrix &Src, const GRID_MAP &Map)        {}
    void            Increment_Omega(const Cart_ID &CID, const Matrix &Src);
    virtual void    Increment_Omega_Node(const Cart_ID &CID, const Vector3 &Src)                        {}
    virtual void    Set_Omega_Node(const int &i, const Vector3 &Src)                                    {}
    #ifdef OPENCL
        virtual void    Set_Omega_Grid(std::vector<cl_float3> &Omega_Grid)                                  {}
    #endif
    virtual void    Increment_Psi_Node(const Cart_ID &CID, const Vector3 &Src)                        {}
    virtual void    Trans_Om_Psi()  {}
    virtual void    Trans_Psi_Om()  {}
//    virtual void    Periodic_Correction()       {}


    //--- Remeshing

//    void    Expand_Activity();
//    void    Set_Remeshed_Nodes();

    //--- Divergence projection
    Real    Div_Om_Max = 0;
    virtual void    Calc_Omega_Div()   {}
    virtual void    Project_Omega()   {}

    //--- Boundary conditions ---

//    StateVector BC_Positions;

//    void SetBC_Spec(char* S) {BCtype = S;}      // For cases where we need to modify this...

    virtual void    Set_BC(const StateVector &BC)               {}
    virtual void    Set_BC_Monolithic()                         {}
    //--- Post-processing ---

    virtual void    Allocate_Output_Arrays()                    {}
//    virtual void    DeAllocate_Output_Arrays()                  {}
    virtual void    Calc_Finite_Differences()                   {}
    virtual void    Set_Diffusion_Node(const Cart_ID &CID)      {}
    virtual void    Set_BiLaplacian_Node(const Cart_ID &CID)    {}
    virtual void    Set_U_Node(const Cart_ID &CID)              {}
    virtual void    SG_Disc_Filter_Node(const Cart_ID &CID, const int &D, Real **I, Real **O) {}
//    virtual void    Set_Strain_Node(const Cart_ID &CID)         {}
    virtual void    Set_Stretching_Node(const Cart_ID &CID)     {}
    virtual void    Set_Laplacian_Sq_Node(const Cart_ID &CID)   {}

    //--- Finite difference
    virtual Real    D_DX(const Cart_ID &CID, Real *A)           {}
    virtual Real    D_DX(const Cart_ID &CID, Real *A, Real *B)  {}
    virtual Real    D_DY(const Cart_ID &CID, Real *A)           {}
    virtual Real    D_DY(const Cart_ID &CID, Real *A, Real *B)  {}
    virtual Real    D_DZ(const Cart_ID &CID, Real *A)           {}
    virtual Real    D_DZ(const Cart_ID &CID, Real *A, Real *B)  {}

    virtual Vector3 D_DX(const Cart_ID &CID, Real **A)          {}
    virtual Vector3 D_DY(const Cart_ID &CID, Real **A)          {}
    virtual Vector3 D_DZ(const Cart_ID &CID, Real **A)          {}

    virtual Real    D2_DX2(const Cart_ID &CID, Real *A)         {}
    virtual Real    D2_DY2(const Cart_ID &CID, Real *A)         {}
    virtual Real    D2_DZ2(const Cart_ID &CID, Real *A)         {}

//    Matrix3 Tensor(const Cart_ID &CID, Real **A, Real **B)  {return Comp_Vec(CID,A).transpose()*Comp_Vec(CID,B);}


    //--- Domain Limits ---

    #ifdef MKLFastPoiss
        MKL_INT nx, ny, nz;                 // Discretisations in x,y,z directions
    #else
        uint nx, ny, nz;                 // Discretisations in x,y,z directions
    #endif

    int X_L, Y_L, Z_L=0;
    Cart_ID CID_Loc(const Cart_ID &CID)     {return CID-Cart_ID(X_L, Y_L, Z_L);}

    //--- Monolithic treatment ---
    Cart_ID CID_MonoLower = Cart_ID::Zero();
    Cart_ID CID_MonoUpper = Cart_ID::Zero();

//    int OLX, OLY, OLZ;

    //--- Face sizes ---
    int N_FaceX=0, N_FaceY=0, N_FaceZ=0;

    //--- BC arrays
    StateVector  BD_Nodes;

//    StateVector FF_Inf, NF_Inf;

    //    Cartesian Indices (j, k) is placed in bd_ax[j+k*(ny+1)].
    Real **BD_AX, **BD_BX;
//    std::vector<Vector>  BD_AX_Nds, BD_BX_Nds;

    // Boundary conditions on y faces
    // Cartesian Indices (i, k) is placed in bd_ay[i+k*(nx+1)].
    Real **BD_AY, **BD_BY;
//    std::vector<Vector>  BD_AY_Nds, BD_BY_Nds;

    // Boundary conditions on z faces
    //    Cartesian Indices (i, j) is placed in bd_az[i+j*(nx+1)].
    Real **BD_AZ, **BD_BZ;
//    std::vector<Vector>  BD_AZ_Nds, BD_BZ_Nds;

    // Value of single layper potential on the surface of the domain (James-Lackner algorithm)
    Real **Sigma;

    //--- Activity Marker

    bool *Active_Nodes;
    std::vector<Cart_ID> Active_Node_ID;

//    bool *Omega_Nodes;
//    std::vector<Cart_ID> Active_Omega_ID;

    bool *Active_Cells;
    bool *Active_Volume_Cells;
    std::vector<Cart_ID> Active_Cells_ID;
    StateVector Remesh_Nodes;

    void Reset_Activity();
    void Declare_Active_Volume_Cells()          {memcpy(Active_Volume_Cells,Active_Cells,N_Cells*sizeof(bool));}
    bool Check_Vol_Source(const Cart_ID &CID)   {return Active_Volume_Cells[ID_CELL(CID_Loc(CID))];}

    //--------Activity data -------------------

    bool            Active = false;
    bool            ProActive = false;

    //--- Extract results ---

    Matrix          Get_Grid1(const Cart_ID &CID, Real *Array, const GRID_MAP &Map);
    Matrix          Get_Grid3(const Cart_ID &CID, Real **Array, const GRID_MAP &Map);

    virtual Matrix  Get_S_Grid(const Cart_ID &CID, const GRID_MAP &Map)              {}
    virtual Matrix  Get_U_Grid(const Cart_ID &CID, const GRID_MAP &Map)              {}
    virtual Matrix  Get_Psi_Grid(const Cart_ID &CID, const GRID_MAP &Map)           {}
    virtual Matrix  Get_Omega_Grid(const Cart_ID &CID, const GRID_MAP &Map)          {}
    virtual Matrix  Get_Lap_Grid(const Cart_ID &CID, const GRID_MAP &Map)            {}
    virtual Matrix  Get_SGS_Grid(const Cart_ID &CID, const GRID_MAP &Map)            {}

    void            Get_Active_Nodes(StateVector &Nds);
    void            Get_Omega_Nodes(StateVector &Nds);

    Real**          Get_Omega_Array()       {return Omega;}
    Real**          Get_Psi_Array()         {return Psi;}


    virtual Vector3 Comp_Vec(const Cart_ID &CID, Real **Array) {}

    Vector3         Get_Omega(const Cart_ID &C) {int I = ID_MKL(C(0),C(1),C(2)); return Vector3(Omega[0][I],Omega[1][I],Omega[2][I]);}
    Vector3         Get_Psi(const Cart_ID &C)   {int I = ID_MKL(C(0),C(1),C(2)); return Vector3(Psi[0][I],Psi[1][I],Psi[2][I]);}

    //--- Getters/Setters

    virtual void    Get_Boundary_Nodes(StateVector &Bs) {}
    Cart_ID         Get_CID()                   {return ID_Super;}
    void            Get_Corner_Points(StateVector &C);
    Vector3         Debug_Psi_at(int H);

    //--- Extraction (Poisson JL)

    virtual void Extract_Grid(Real **Grid, const Cart_ID &CO, const Cart_ID &W, StateVector &M) {}
    virtual void Superpose_Grid(Real **Grid, const Cart_ID &CO, const Cart_ID &W, StateVector &M){}

    virtual void Extract_Omega_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {}
    virtual void Superpose_Omega_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {}

    virtual void Extract_Psi_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {}
    virtual void Superpose_Psi_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {}

    virtual void Nullify_Omega(const Cart_ID &CO, const Cart_ID &W) {}

    //--- Monolithic grid

//    virtual void Superpose_Omega(Eul_Grid *SrcGrid);

//    virtual void    Reduce_Set() {}

    //--- Debugging ---

    void    Debug_Domain_Corners();
//    void    Debug_Omega();
    virtual void Debug_BC() {}
    void    Get_Pos_Grid_2D(std::vector<Vector3> *P, const int &CX, const int &CY, const int &SL, const int &SR);

    StateVector Get_Plane(const Plane &P, const int &I);
//    void    Surf(const int &H);

    //--- Destructor ---
    ~Eul_Grid();

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

};

template <class Type, size_t size>
void addArrays(const Type(&a)[size],
               const Type(&b)[size],
               Type(&result)[size])
{
    std::transform(a, a + size, b, result, std::plus<Type>());
}

typedef std::shared_ptr<Eul_Grid>    SP_EGrid;

//extern Eul_Grid *EGrid_Mono;

// 2D Euler domain:

// For each derived class we need to implement new:

// Virtual Functions

//Set_BC_Positions()

//--------------------------------------------------------------
//----------------------2D UNBOUND------------------------------
//--------------------------------------------------------------

class Eul_Grid_2D : public Eul_Grid
{
protected:


public:

    //--- Constructor
    Eul_Grid_2D(const Cart_ID &CID, const Cart_ID &XE, const Cart_ID &NE) : Eul_Grid(CID,XE,NE)
    {
        BCtype = "DDDD";
        Init_Grid();
    }

    //--- Init
    void    Init_Grid();

    //--- Solver
    void    Solve_Poisson()
    {
        Solve_Poisson(2);
        Set_Psi_Active();
    }
    void    Solve_Poisson(const int &Dim);
    void    Init_Solver();
    void    Commit_Arrays();
    void    Execute_Solver();
    void    Reset_Omega()   {Reset_Array(Omega[2],N_Points);}
    void    Reset_Psi()     {Reset_Array(Psi[2],N_Points);}

    //--- Poisson JL Functions ---

    void    Set_Boundary_Gradient();
    void    Set_Boundary_Psi0();
    void    PoissonJL_Compact_Support();

    //--- Omega Spec ---
    void    Trans_Om_Psi()  {memcpy(Psi[2],Omega[2],N_Points*sizeof(Real));}
    void    Increment_Omega(const Cart_ID &CID, const Matrix &Src, const GRID_MAP &Map);

    //--- Extract results ---
    Matrix  Get_U_Grid(const Cart_ID &CID, const GRID_MAP &Map)      {return Get_Grid3(CID,U,Map);}
    Matrix  Get_S_Grid(const Cart_ID &CID, const GRID_MAP &Map)      {return Get_Grid3(CID,DAlphaDt,Map);}

    //--- Activity

    void    Set_Cell_Active(const Cart_ID &CID);
    void    Mark_Surr_Node_Activity(const Cart_ID &CID);
    void    Set_Direct_Node_Activity();
    void    Set_Node_Active(const int &IDX,const int &IDY,const int &IDZ);
    void    Set_Node_Activity(const GRID_MAP &MAP);

    //--- Finite difference (higher order for Poisson JL method)

    Real    D_DX_3(const Cart_ID &CID, Real *A);
    Real    D_DY_3(const Cart_ID &CID, Real *A);

    //--- Extraction (Poisson JL)

    void Extract_Grid(Real *Grid,   const Cart_ID &C, const Cart_ID &W, StateVector &M);
    void Superpose_Grid(Real *Grid, const Cart_ID &C, const Cart_ID &W, StateVector &M);
    void Nullify_Array(const Cart_ID &C, const Cart_ID &W, Real **A);

    void Extract_Omega_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Extract_Grid(Omega[2],CO,W,M);}
    void Superpose_Omega_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Superpose_Grid(Omega[2],CO,W,M);}

    void Extract_Psi_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Extract_Grid(Psi[2],CO,W,M);}
    void Superpose_Psi_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Superpose_Grid(Psi[2],CO,W,M);}

    void Nullify_Psi(const Cart_ID &C, const Cart_ID &W)            {Nullify_Array(C,W,Psi);}
    void Nullify_Omega(const Cart_ID &C, const Cart_ID &W)          {Nullify_Array(C,W,Omega);}

    //--- Post processing

    void    Calc_Finite_Differences();
    void    Set_Diffusion_Node(const Cart_ID &CID);
    void    Set_U_Node(const Cart_ID &CID);
    void    SG_Disc_Filter_Node(const Cart_ID &CID, const int &D, Real **I, Real **O)     {}
//    void    Set_Strain_Node(const Cart_ID &CID)         {}

    //--- BC
//    void    Reset_BC();
    void        Set_BC_Positions();
    void        Set_BC(const StateVector &BC);       // Set BC nodes
    void        Get_Boundary_Nodes(StateVector &Bs);

    //--- ID ref
    uint ID_MKL(const int &i, const int &j)     {return ID_MKL_2D(i,j);}
    uint ID_MKL(const Cart_ID &CID)             {return ID_MKL_2D(CID(0),CID(1));}

    //--- Debugging ---
    void Debug_BC();
};

//--------------------------------------------------------------
//----------------------2D CYL----------------------------------
//--------------------------------------------------------------

class Eul_Grid_2D_Cyl : public Eul_Grid_2D
{
    // Special implementation for the cylindrical style grids


};

//--------------------------------------------------------------
//----------------------3D UNBOUND------------------------------
//--------------------------------------------------------------

class Eul_Grid_3D : public Eul_Grid
{
protected:

    //--- Finite difference calcs

    Real            H3;
    Real            InvH;
    Real            InvH3;

public:

    //--- Constructor
    Eul_Grid_3D(const Cart_ID &CID, const Cart_ID &XE, const Cart_ID &NE) : Eul_Grid(CID,XE,NE)
    {
        BCtype = "DDDDDD";
        Init_Grid();
    }

    //--- Init
    void    Init_Grid();
    void    Allocate_Output_Arrays();
//    void    DeAllocate_Output_Arrays();

    //--- Solver
    void    Solve_Poisson()
    {
        Solve_Poisson(0);
        Solve_Poisson(1);
        Solve_Poisson(2);
        Set_Psi_Active();
    }
    void    Solve_Poisson(const int &Dim);
    void    Init_Solver(const int &Dim);
    void    Commit_Arrays(const int &Dim);
    void    Execute_Solver(const int &Dim);
    void    Reset_Arrays();
    void    Reset_Omega()   {Reset_Array3D(Omega,N_Points);}
    void    Reset_Psi()     {Reset_Array3D(Psi,N_Points);}

    //--- Poisson JL Functions ---

    void    Set_Boundary_Gradient();
    void    Set_Boundary_Psi0();
    void    PoissonJL_Compact_Support();

    //--- Monolithic grid

//    void    Superpose_Omega(Eul_Grid *SrcGrid);

    //--- Omega Spec ---
    void    Increment_Omega_Node(const Cart_ID &CID, const Vector3 &Src);
    void    Increment_Psi_Node(const Cart_ID &CID, const Vector3 &Src);
    void    Increment_Omega(const Cart_ID &CID, const Matrix &Src, const GRID_MAP &Map);
    void    Trans_Om_Psi()  {for (int Dim=0; Dim<3; Dim++)  memcpy(Psi[Dim],Omega[Dim],N_Points*sizeof(Real));}
    void    Trans_Psi_Om()  {for (int Dim=0; Dim<3; Dim++)  memcpy(Omega[Dim],Psi[Dim],N_Points*sizeof(Real));}

    void    Set_Omega_Node(const int &i, const Vector3 &Src);
    #ifdef OPENCL
        void    Set_Omega_Grid(std::vector<cl_float3> &Omega_Grid);
    #endif

    //--- Divergence projection
    void    Calc_Omega_Div();
    void    Project_Omega();

    //--- Extract results ---

    Matrix  Get_Psi_Grid(const Cart_ID &CID, const GRID_MAP &Map)    {return Get_Grid3(CID,Psi,Map);}
    Matrix  Get_Omega_Grid(const Cart_ID &CID, const GRID_MAP &Map)  {return Get_Grid3(CID,Omega,Map);}
    Matrix  Get_U_Grid(const Cart_ID &CID, const GRID_MAP &Map)      {return Get_Grid3(CID,U,Map);}
    Matrix  Get_S_Grid(const Cart_ID &CID, const GRID_MAP &Map)      {return Get_Grid3(CID,DAlphaDt,Map);}
    Matrix  Get_SGS_Grid(const Cart_ID &CID, const GRID_MAP &Map)    {return Get_Grid1(CID,Nu_SGS,Map);}
//    Matrix  Get_NablaUOm_Grid(const Cart_ID &CID)   {return Get_Grid3(CID,NablaUOm);}
//    Matrix  Get_Lap_Grid(const Cart_ID &CID)    {return Get_Grid3(CID,Lap);}

    //--- Activity

    void    Set_Cell_Active(const Cart_ID &CID);
    void    Mark_Surr_Node_Activity(const Cart_ID &CID);
    void    Set_Direct_Node_Activity();
    void    Set_Node_Active(const int &IDX,const int &IDY,const int &IDZ);
    void    Set_Node_Activity(const GRID_MAP &Map);

    //--- Post processing
    void    Calc_Finite_Differences();
    void    Set_Diffusion_Node(const Cart_ID &CID);
    void    Calc_Turb_Stress_SGM();
    void    Calc_Turb_Stress_RVM();
    void    Set_U_Node(const Cart_ID &CID);
    void    SG_Disc_Filter_Node(const Cart_ID &CID, const int &D, Real **I, Real **O);
    void    Set_Stretching_Node(const Cart_ID &CID);

    Vector3 Comp_Vec(const Cart_ID &CID, Real **Array);
    Vector3 Comp_Vec(const int &i, const int &j, const int &k, Real **Array)    {return Comp_Vec(Cart_ID(i,j,k),Array);}

    //--- BC
//    void    Reset_BC();
    void        Set_BC_Positions();
    void        Set_BC(const StateVector &BC);          // Set BC nodes
    void        Set_BC_Monolithic();                    // Monolithic solver BC vec
    void        Set_BC_JL(const StateVector &BC);       // Set BC nodes
    void        Get_Boundary_Nodes(StateVector &Bs);

    //--- ID ref
    virtual uint    ID_MKL(const int &i, const int &j, const int &k)        {return ID_MKL_3D(i,j,k);}
    uint    Cell_ID_MKL(const int &i, const int &j, const int &k)   {return ID_MKL_3D(i,j,k);}
    uint    ID_MKL(const Cart_ID &CID)                              {return ID_MKL_3D(CID(0),CID(1),CID(2));}
    uint    ID_CELL(const Cart_ID &CID)                             {return CID(0)*ny*nz + CID(1)*nz + CID(2);}
//    void    Mark_Omega_Activity(const Cart_ID &CID);

    //--- Finite difference
    Real    D_DX(const Cart_ID &CID, Real *A);
    Real    D_DX(const Cart_ID &CID, Real *A, Real *B);
    Real    D_DY(const Cart_ID &CID, Real *A);
    Real    D_DY(const Cart_ID &CID, Real *A, Real *B);
    Real    D_DZ(const Cart_ID &CID, Real *A);
    Real    D_DZ(const Cart_ID &CID, Real *A, Real *B);

    Vector3 D_DX(const Cart_ID &CID, Real **A);
    Vector3 D_DY(const Cart_ID &CID, Real **A);
    Vector3 D_DZ(const Cart_ID &CID, Real **A);

    Real    D2_DX2(const Cart_ID &CID, Real *A);
    Real    D2_DY2(const Cart_ID &CID, Real *A);
    Real    D2_DZ2(const Cart_ID &CID, Real *A);

    //--- Finite difference (higher order for Poisson JL method)
    Real    D_DX_2(const Cart_ID &CID, Real *A);
    Real    D_DY_2(const Cart_ID &CID, Real *A);
    Real    D_DZ_2(const Cart_ID &CID, Real *A);

    Real    D_DX_3(const Cart_ID &CID, Real *A);
    Real    D_DY_3(const Cart_ID &CID, Real *A);
    Real    D_DZ_3(const Cart_ID &CID, Real *A);

    Real    D_DX_4(const Cart_ID &CID, Real *A);
    Real    D_DY_4(const Cart_ID &CID, Real *A);
    Real    D_DZ_4(const Cart_ID &CID, Real *A);

    //--- Extraction (Poisson JL)

    void Extract_Grid(Real **Grid,   const Cart_ID &C, const Cart_ID &W, StateVector &M);
    void Superpose_Grid(Real **Grid, const Cart_ID &C, const Cart_ID &W, StateVector &M);
    void Nullify_Array(const Cart_ID &C, const Cart_ID &W, Real **A);

    void Extract_Omega_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Extract_Grid(Omega,CO,W,M);}
    void Superpose_Omega_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Superpose_Grid(Omega,CO,W,M);}

    void Extract_Psi_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Extract_Grid(Psi,CO,W,M);}
    void Superpose_Psi_Grid(const Cart_ID &CO, const Cart_ID &W, StateVector &M) {Superpose_Grid(Psi,CO,W,M);}

    void Nullify_Psi(const Cart_ID &C, const Cart_ID &W)            {Nullify_Array(C,W,Psi);}
    void Nullify_Omega(const Cart_ID &C, const Cart_ID &W)          {Nullify_Array(C,W,Omega);}

};

//--------------------------------------------------------------
//----------------------3D PERIODIC X---------------------------
//--------------------------------------------------------------

class Eul_Grid_3D_PX : public Eul_Grid_3D
{
public:

    //--- Constructor
    Eul_Grid_3D_PX(const Cart_ID &CID, const Cart_ID &XE, const Cart_ID &NE) : Eul_Grid_3D(CID,XE,NE)
    {
        BCtype = "PPDDDD";
        Init_Grid();
    }

    //--- ID ref
    uint    ID_MKL(const int &i, const int &j, const int &k)        {return ID_MKL_3D_PX(i,j,k);}
    uint    Cell_ID_MKL(const int &i, const int &j, const int &k)   {return ID_MKL_3D_PX(i,j,k);}
    uint    ID_MKL(const Cart_ID &CID)                              {return ID_MKL_3D_PX(CID(0),CID(1),CID(2));}
//    uint    ID_CELL(const Cart_ID &CID)                             {return CID(0)*ny*nz + CID(1)*nz + CID(2);}
//    void    Mark_Omega_Activity(const Cart_ID &CID);

//    void    Periodic_Correction();
};

////--------------------------------------------------------------
////----------------------3D NEUMANN X---------------------------
////--------------------------------------------------------------

class Eul_Grid_3D_N : public Eul_Grid_3D
{
public:

    //--- Constructor
    Eul_Grid_3D_N(const Cart_ID &CID, const Cart_ID &XE, const Cart_ID &NE) : Eul_Grid_3D(CID,XE,NE)
    {
        BCtype = "NNNNNN";
        Init_Grid();
    }

    //--- Post processing
    void    Calc_Finite_Differences();

    void    Allocate_Output_Arrays();
};

}

#endif // ML_Euler_H
