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
//-------------------------VPML Euler Grid Functions---------------------------
//-----------------------------------------------------------------------------

#include "ML_Euler.h"
#include "Box_Tree.h"

namespace VPML
{

//--- Constructor ---

Eul_Grid::Eul_Grid(const Cart_ID &OSuper, const Cart_ID &OSub, const Cart_ID &N)
{
    // General constructor

    ID_Super = OSuper;

    Origin_Glob = OSub;

    X_L = OSub(0);
    Y_L = OSub(1);
    Z_L = OSub(2);
//    CID_G = Cart_ID(X_L,Y_L,Z_L);

    nx = N(0);
    ny = N(1);
    nz = N(2);

//    qDebug() << nx << ny << nz;

//    CMatrix URFD;

//    URFD = CMatrix::Zero(NR+1,NR+1);

//    for (int i=0; i<Nr+1; i++)
//    {
//        for (int j=0; j<Nr+1; j++)
//        {
//            URFD(i,j) = Blah;
//        }
//    }

}

//--- Omega Spec ---

//void Eul_Grid::Increment_Activate_Omega(const Cart_ID &CID, const Vector3 &Src)
//{
//    Cart_ID CIDL = CID_Loc(CID);
//    uint IDCell = CIDL(0)*ny*nz + CIDL(1)*nz + CIDL(2);    // Local ID!
//    if (!Active_Cells[IDCell])
//    {
//        Active_Cells[IDCell] = true;
//        Active_Cells_ID.push_back(CIDL);
//    }
//    // Now increment Omega
//    int IDMKL = ID_MKL(CIDL(0),CIDL(1),CIDL(2));
//    Omega[0][IDMKL] += Src(0);
//    Omega[1][IDMKL] += Src(1);
//    Omega[2][IDMKL] += Src(2);
//}

//--- Destructor ---

Eul_Grid::~Eul_Grid()
{
    qDebug() << "Delete Eulerian grid;";

//    qDebug() << "Freeing helopmholtz;";
//    if (Vars->Dim==SIM_2D) free_Helmholtz_2D(&xhandle, ipar, &stat);
//    if (Vars->Dim==SIM_3D) free_Helmholtz_3D(&xhandle, &yhandle, ipar, &stat);

    int Dim;
    if (Vars->Dim==SIM_2D) Dim=1;
    if (Vars->Dim==SIM_3D) Dim=3;

    qDebug() << "Deallocating  BClists";

    free(spar);
    for (int i=0; i<Dim; i++)
    {
        free(BD_AX[i]);
        free(BD_BX[i]);
        free(BD_AY[i]);
        free(BD_BY[i]);
        free(BD_AZ[i]);
        free(BD_BZ[i]);
    }

    qDebug() << "Deleting BD objects;";

    delete BD_AX;
    delete BD_BX;
    delete BD_AY;
    delete BD_BY;
    delete BD_AZ;
    delete BD_BZ;

    qDebug() << "Freeing arrays";

    for (int i=0; i<Dim; i++)
    {
        free(Psi[i]);
//        free(DAlphaDt[i]);
        free(Omega[i]);
    }

    qDebug() << "Deleting arrays";

    delete Psi;
//    delete DAlphaDt;
    delete Omega;

    delete Active_Nodes;
    delete Active_Cells;
    delete Active_Volume_Cells;
}

//--- Set Boundary conditions ---

void Eul_Grid::Reset_Activity()
{
    // This resets all nodes to be inactive
    memset(Active_Nodes, false, N_Points*sizeof(bool));
    Active_Node_ID.clear();

    memset(Active_Cells, false, N_Cells*sizeof(bool));
    Active_Cells_ID.clear();
}

//--- Getters/Setters

void Eul_Grid::Get_Corner_Points(StateVector &C)
{
    // This is used simply for visualisation. Extract the corner points in the
    // global system.

    if (Vars->Dim==SIM_2D)
    {
        C.push_back(Node_Pos_Global(Cart_ID(X_L,Y_L,0)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L,Y_L+ny,0)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L+nx,Y_L,0)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L+nx,Y_L+ny,0)));
    }
    if (Vars->Dim==SIM_3D)
    {
        C.push_back(Node_Pos_Global(Cart_ID(X_L,Y_L,Z_L)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L,Y_L,Z_L+nz)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L,Y_L+ny,Z_L)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L,Y_L+ny,Z_L+nz)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L+nx,Y_L,Z_L)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L+nx,Y_L,Z_L+nz)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L+nx,Y_L+ny,Z_L)));
        C.push_back(Node_Pos_Global(Cart_ID(X_L+nx,Y_L+ny,Z_L+nz)));
    }
}

//--- Poisson JL Functions ---

StateVector Eul_Grid::Get_Boundary_Sources()
{
    // This is a function which return an array of the sources of the NF

    StateVector BCG;    //, BCG;
//    Get_Boundary_Nodes(BCN);          // Positions of BC

    Real H2 = Grid_Vars->H_Grid*Grid_Vars->H_Grid;
    BCG.assign(N_BPoints,Vector8::Zero());
    for (int i=0; i<N_BPoints; i++)
    {
        BCG[i](0) = BD_Nodes[i](0);
        BCG[i](1) = BD_Nodes[i](1);
        BCG[i](2) = BD_Nodes[i](2);
        BCG[i](3) = -BG(i,0);
        BCG[i](4) = -BG(i,1);
        BCG[i](5) = -BG(i,2);
        BCG[i](6) = Grid_Vars->Sigma_Char;
        BCG[i](7) = H2;                 // Surface integral. We multiply this by characteristic area, not volume!
    }

    return BCG;
}

//--- Extract results ---

void Eul_Grid::Get_Active_Nodes(StateVector &Nds)
{
    // This simply returns the active nodes. Keep in mind the activity here...
    // Do NOT return nodes which are in the overlap region.

//    Vector SH = 0.5*Grid_Vars->H_Grid*Vector3(1,1,1);   // Shifting...

    for (int i=0; i<Active_Node_ID.size(); i++)
    {
        int IDMKL = ID_MKL(Active_Node_ID[i]);
        Vector3 O(Omega[0][IDMKL],Omega[1][IDMKL],Omega[2][IDMKL]);
//        if (O.norm()==0.0) continue;      // Ignore if particle has zero strength
        if (O.norm()<1.0e-9) continue;      // Ignore if particle has zero strength
        Vector3 Pos = Node_Pos_Global(Active_Node_ID[i]+Origin_Glob);
        Nds.push_back(Create_Particle(Pos,O));
    }
}

//--- Extract results ---

Matrix Eul_Grid::Get_Grid1(const Cart_ID &CID, Real *Array, const GRID_MAP &Map)
{
    //    // This extracts the array of interest
    //    if (Grid_Vars->Mapping_Order==0)   return Get_Grid3_M0(CID,Array);
    //    if (Grid_Vars->Mapping_Order==1)   return Get_Grid3_M1(CID,Array);
    Cart_ID S = CID_Loc(CID);

    switch (Map)
    {
    case (M0):
    {
        Matrix M = Matrix(1,1);
        int IDMKL = ID_MKL(S(0),S(1),S(2));
        M(0) = Array[IDMKL];
        return M;
    }
    case (M2):
    {
        Matrix M = Matrix::Zero(8,1);
        for (int i=0; i<2; i++)
        {
            int IDXL = S(0)+MPS02X[i];
            for (int j=0; j<2; j++)
            {
                int IDYL = S(1)+MPS02X[j];
                for (int k=0; k<2; k++)
                {
                    int IDZL = S(2)+MPS02X[k];
                    int IDMKL = ID_MKL(IDXL,IDYL,IDZL);
                    int IDB = i*4+j*2+k;
                    M(IDB) = Array[IDMKL];
                }
            }
        }
        return M;
    }
    case (M4D):
    {
        Matrix M = Matrix::Zero(64,1);
        for (int i=0; i<4; i++)
        {
            int IDXL = S(0)+MPS12X[i];
            for (int j=0; j<4; j++)
            {
                int IDYL = S(1)+MPS12X[j];
                for (int k=0; k<4; k++)
                {
                    int IDZL = S(2)+MPS12X[k];
                    int IDMKL = ID_MKL(IDXL,IDYL,IDZL);
                    int IDB = i*16+j*4+k;
                    M(IDB) = Array[IDMKL];
                }
            }
        }
        return M;
    }
    default: break;
    }

}

Matrix Eul_Grid::Get_Grid3(const Cart_ID &CID, Real **Array, const GRID_MAP &Map)
{
//    // This extracts the array of interest
//    if (Grid_Vars->Mapping_Order==0)   return Get_Grid3_M0(CID,Array);
//    if (Grid_Vars->Mapping_Order==1)   return Get_Grid3_M1(CID,Array);
    Cart_ID S = CID_Loc(CID);

    switch (Map)
    {
        case (M0):
        {
            Matrix M = Matrix(1,3);
            int IDMKL = ID_MKL(S(0),S(1),S(2));
            M(0) = Array[0][IDMKL];
            M(1) = Array[1][IDMKL];
            M(2) = Array[2][IDMKL];
            return M;
        }
        case (M2):
        {
            Matrix M = Matrix::Zero(8,3);
            for (int i=0; i<2; i++)
            {
                int IDXL = S(0)+MPS02X[i];
                for (int j=0; j<2; j++)
                {
                    int IDYL = S(1)+MPS02X[j];
                    for (int k=0; k<2; k++)
                    {
                        int IDZL = S(2)+MPS02X[k];
                        int IDMKL = ID_MKL(IDXL,IDYL,IDZL);
                        int IDB = i*4+j*2+k;
                        M(IDB,0) = Array[0][IDMKL];
                        M(IDB,1) = Array[1][IDMKL];
                        M(IDB,2) = Array[2][IDMKL];
                    }
                }
            }

            return M;
        }
        case (M4D):
        {
            Matrix M = Matrix::Zero(64,3);
            for (int i=0; i<4; i++)
            {
                int IDXL = S(0)+MPS12X[i];
                for (int j=0; j<4; j++)
                {
                    int IDYL = S(1)+MPS12X[j];
                    for (int k=0; k<4; k++)
                    {
                        int IDZL = S(2)+MPS12X[k];
                        int IDMKL = ID_MKL(IDXL,IDYL,IDZL);
                        int IDB = i*16+j*4+k;
                        M(IDB,0) = Array[0][IDMKL];             // CHECK THIS!!!
                        M(IDB,1) = Array[1][IDMKL];
                        M(IDB,2) = Array[2][IDMKL];
                    }
                }
            }

            return M;
        }
        default: break;
    }

}

//--- Debugging ---

void Eul_Grid::Debug_Domain_Corners()
{
    std::vector<Vector3> Cs;
//    if (Vars->Dim==SIM_2D)
//    {
//        Cs.push_back(Node_Pos(Cart_ID(X_L,Y_L,0)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L,Y_L+ny,0)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L+nx,Y_L+ny,0)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L+nx,Y_L,0)));
//    }
//    if (Vars->Dim==SIM_3D)
//    {
//        Cs.push_back(Node_Pos(Cart_ID(X_L,Y_L,Z_L)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L,Y_L,Z_L+nz)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L,Y_L+ny,Z_L)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L,Y_L+ny,Z_L+nz)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L+nx,Y_L,Z_L)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L+nx,Y_L,Z_L+nz)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L+nx,Y_L+ny,Z_L)));
//        Cs.push_back(Node_Pos(Cart_ID(X_L+nx,Y_L+ny,Z_L+nz)));
//    }

    for (int i=0; i<Cs.size(); i++)
    {
//        qDebug() << "[" << Cs[i](0) << ", " << Cs[i](1) << ", " << Cs[i](2) << "],";
        qDebug() << Cs[i](0) << Cs[i](1) << Cs[i](2) ;
    }
    qDebug() << Cs[0](0) << Cs[0](1) << Cs[0](2) ;      // Close loop
}

StateVector Eul_Grid::Get_Plane(const Plane &P, const int &I)
{

    StateVector Array;
    if (    (P==XPlane && (I<0 || I>nx)) ||
            (P==YPlane && (I<0 || I>ny)) ||
            (P==ZPlane && (I<0 || I>nz))    )
    {
        qDebug() << "Eul_Grid::I OUT OF RANGE!!!!";
        return Array;
    }

    int ID1,ID2;
    if (P==XPlane) ID1=ny+1;
    if (P==XPlane) ID2=nz+1;
    if (P==YPlane) ID1=nx+1;
    if (P==YPlane) ID2=nz+1;
    if (P==ZPlane) ID1=nx+1;
    if (P==ZPlane) ID2=ny+1;

    // Set source
    Real **A = Omega;
//    Real *A = Nu_SGS;
//    bool *A = Active_Nodes;

    for (int id1=0; id1<ID1; id1++)
    {
        for (int id2=0; id2<ID2; id2++)
        {
            Cart_ID CID;
            if (P==XPlane)  CID = Origin_Glob + Cart_ID(I,id1,id2);
            if (P==YPlane)  CID = Origin_Glob + Cart_ID(id1,I,id2);
            if (P==ZPlane)  CID = Origin_Glob + Cart_ID(id1,id2,I);

            int IDJ;
            if (P==XPlane)  IDJ = id1*(nz+1) + id2;
            if (P==YPlane)  IDJ = id1*(nz+1) + id2;
            if (P==ZPlane)  IDJ = id1*(ny+1) + id2;

            int IDMKL;
            if (P==XPlane)  IDMKL = ID_MKL(I,id1,id2);
            if (P==YPlane)  IDMKL = ID_MKL(id1,I,id2);
            if (P==ZPlane)  IDMKL = ID_MKL(id1,id2,I);

            // Position
            Vector3 Pos = Node_Pos_Global(CID);

            // Value (Grid solver)
            Vector3 Val(A[0][IDMKL],A[1][IDMKL],A[2][IDMKL]);
//            qDebug() << IDMKL << N_Points << A[0][IDMKL] << A[1][IDMKL] << A[2][IDMKL];
//            Vector3 Val(0,0,A[IDMKL]);

            // Value (JL boundary values)
//            int SH;
//            if (P==XPlane && I==0)  SH=0;
//            if (P==XPlane && I==nx) SH=N_FaceX;
//            if (P==YPlane && I==0)  SH=2*N_FaceX;
//            if (P==YPlane && I==ny) SH=2*N_FaceX+N_FaceY;
//            if (P==ZPlane && I==0)  SH=2*N_FaceX+2*N_FaceY;
//            if (P==ZPlane && I==nz) SH=2*N_FaceX+2*N_FaceY+N_FaceZ;
//            Vector3 Val(Sigma[0][IDJ+SH],Sigma[1][IDJ+SH],Sigma[2][IDJ+SH]);

            Vector6 V; V << Pos(0), Pos(1), Pos(2), Val(0), Val(1), Val(2);
            Array.push_back(V);
        }
    }

    return Array;
}

//-- Auxiliary functions --

int Eul_Grid::Factorial(int n)
{
    return (n==0 || n==1) ? 1 : Factorial(n-1)*n;
}

//-- MKL ID Access --

int Eul_Grid::IDPX(const int &i)
{
    if (i<0)            return i+nx;
    else if (i>nx)      return i-nx;
    else                return i;
}

int Eul_Grid::IDPY(const int &j)
{
    if (j<0)            return j+ny;
    else if (j>ny)      return j-ny;
    else                return j;
}

int Eul_Grid::IDPZ(const int &k)
{
    if (k<0)            return k+nz;
    else if (k>nz)      return k-nz;
    else                return k;
}

//--------------------------------------------------------------
//----------------------2D UNBOUND------------------------------
//--------------------------------------------------------------

//--- Initialize grid vars

void Eul_Grid_2D::Init_Grid()
{
    // Initialize 2D grid

    // Boundary positions

    ax = X_L*Grid_Vars->H_Grid;
    bx = (X_L+nx+1)*Grid_Vars->H_Grid;

    ay = Y_L*Grid_Vars->H_Grid;
    by = (Y_L+ny+1)*Grid_Vars->H_Grid;

    // Allocate memory blocks -- spar, f, bd_a..... ( Internal var Helmholtz Solver, RHS, BCs)

    spar=(Real*)malloc((13*nx/2+7)*sizeof(Real));
//    f=(Real*)calloc((nx+1)*(ny+1)*sizeof(Real),sizeof(Real));
//    f0=(Real*)calloc((nx+1)*(ny+1)*sizeof(Real),sizeof(Real));

    BD_AX = new Real*[1];
    BD_BX = new Real*[1];
    BD_AY = new Real*[1];
    BD_BY = new Real*[1];

    BD_AX[0] = (Real*)calloc((ny+1)*sizeof(Real),sizeof(Real));
    BD_BX[0] = (Real*)calloc((ny+1)*sizeof(Real),sizeof(Real));
    BD_AY[0] = (Real*)calloc((nx+1)*sizeof(Real),sizeof(Real));
    BD_BY[0] = (Real*)calloc((nx+1)*sizeof(Real),sizeof(Real));

    Set_BC_Positions();                 // Set initial BC vals

    N_Points = (nx+1)*(ny+1);
    N_FaceX = (ny+1);
    N_FaceY = (nx+1);
    N_BPoints = 2*N_FaceX + 2*N_FaceY;
    N_Cells = nx*ny;

    // Set q value (0 for Poisson problem)

    q = 0;

    // Initializing ipar array to make it free from garbage

    for (int i=0;i<128;i++)  ipar[i]=0;

    // Allocate Solution arrays

    Psi = new Real*[3];
    Psi[2] = new Real[N_Points];

    Omega = new Real*[3];
    Omega[2] = new Real[N_Points];

    U = new Real*[3];
    U[0] = new Real[N_Points];
    U[1] = new Real[N_Points];
//    U[2] = new Real[N_Points];
//    memset(U[2], 0, N_Points*sizeof(Real));     // This will never be modified. Set zero

    DAlphaDt = new Real*[1];
    DAlphaDt[0] = new Real[N_Points];
    Active_Nodes = new bool[N_Points];  // Allocate activity array
    Active_Cells = new bool[N_Cells];   // Allocate activity array

    // Set finite diff vals
    H2 = Grid_Vars->H_Grid*Grid_Vars->H_Grid;
    InvH = 1.0/Grid_Vars->H_Grid;
    InvH2 = 1.0/H2;

    // Reset initial arrays
    Reset_Array(BD_AX[0],N_FaceX);
    Reset_Array(BD_BX[0],N_FaceX);
    Reset_Array(BD_AY[0],N_FaceY);
    Reset_Array(BD_BY[0],N_FaceY);
//    Reset_Array(BD_AZ[0],N_FaceZ);
//    Reset_Array(BD_BZ[0],N_FaceZ);
    Reset_Array(Psi[2],N_Points);
    Reset_Array(Omega[2],N_Points);
    memset(Active_Nodes, false, N_Points*sizeof(bool));
    memset(Active_Cells, false, N_Cells*sizeof(bool));
}

//--- Solver

void Eul_Grid_2D::Solve_Poisson(const int &Dim)
{
    // Solves the Poisson equation in the given dimension

    // This executes the fast poisson Solver using the in-built library in the Intel Math Kernel Library
    // As the library is not set up to Solve 3D inputs, we must Solve equation for each Vars->Dimension.

    // Notes:   "commit" modifies f, ipar, spar
    //          "helmholtz" modifies only f

    #ifdef MKLFastPoiss
        memset(ipar, 0, 128*sizeof(MKL_INT));       // Clear integer params setting
    #else
        memset(ipar, 0, 128*sizeof(uint));       // Clear integer params setting
    #endif
    Init_Solver();           // Init Solver
    //    ipar[23] = 32;               // Set number of threads // Leave this alone....
    Commit_Arrays();         // Commit arrays to carry out checks
    Execute_Solver();        // Execute Solver
    Set_Psi_Active();           // Set Psi grid active
}

void Eul_Grid_2D::Init_Solver()
{
    #ifdef MKLFastPoiss
        #ifdef SinglePrec
            s_init_Helmholtz_2D(&ax, &bx, &ay, &by, &nx, &ny, BCtype, &q, ipar, spar, &stat);
        #endif

        #ifdef DoublePrec
            d_init_Helmholtz_2D(&ax, &bx, &ay, &by, &nx, &ny, BCtype, &q, ipar, spar, &stat);
        #endif

        if (stat!=0)
        {
            qDebug() << "Eul_Grid_2D::Init_Solver init has failed.";
            return;
        }
    #endif
}

void Eul_Grid_2D::Commit_Arrays()
{
    memcpy(Psi[2],Omega[2],N_Points*sizeof(Real));

    #ifdef MKLFastPoiss
        #ifdef SinglePrec
            s_commit_Helmholtz_2D(Psi[2], BD_AX[0], BD_BX[0], BD_AY[0], BD_BY[0], &xhandle, ipar, spar, &stat);
        #endif

        #ifdef DoublePrec
            d_commit_Helmholtz_2D(Psi[0], BD_AX[0], BD_BX[0], BD_AY[0], BD_BY[0], &xhandle, ipar, spar, &stat);
        #endif

    //    ipar[2]=0;                  // Do not output error message for Neuemann BC solver!

        if (stat!=0)
        {
            qDebug() << "Eul_Grid::Commit_Arrays commit has failed.";
            return;
        }
    #endif
}

void Eul_Grid_2D::Execute_Solver()
{
    #ifdef MKLFastPoiss
        #ifdef SinglePrec
            s_Helmholtz_2D(Psi[2], BD_AX[0], BD_BX[0], BD_AY[0], BD_BY[0], &xhandle, ipar, spar, &stat);
        #endif

        #ifdef DoublePrec
            d_Helmholtz_2D(Psi[0], BD_AX[0], BD_BX[0], BD_AY[0], BD_BY[0], &xhandle, ipar, spar, &stat);
        #endif

        if (stat!=0 && ipar[2]!=0)
        {
            qDebug() << "Eul_Grid::Execute_Solver Solve has failed.";
            return;
        }
    #endif
}

//--- Set Omega

void Eul_Grid_2D::Increment_Omega(const Cart_ID &CID, const Matrix &Src, const GRID_MAP &Map)
{
    // This is a general mapping function. depending on the choice of mapping,
    // the nodes grids are incremeneted appropriately
    // CID is the neighboring particle ID, Src is the interpolating matrix.
    // This is calculated elsewhere, as sometimes we need to map to multiple grids

    Cart_ID S = CID_Loc(CID);

    switch (Map)
    {
        case (M0):
        {
            int IDMKL = ID_MKL(S(0),S(1));
            Omega[2][IDMKL] += Src(2);
            break;
        }
        case (M2):
        {
            for (int i=0; i<2; i++)
            {
                int IDXL = S(0)+MPS02X[i];
                for (int j=0; j<2; j++)
                {
                    int IDYL = S(1)+MPS02X[j];
                    int IDMKL = ID_MKL(IDXL,IDYL);
                    int ID_B = 2*i+j;
                    Omega[2][IDMKL] += Src(ID_B,2);
                }
            }
            break;
        }
        case (M4D):
        {
            for (int i=0; i<4; i++)
            {
                int IDXL = S(0)+MPS12X[i];
                for (int j=0; j<4; j++)
                {
                    int IDYL = S(1)+MPS12X[j];
                    int IDMKL = ID_MKL(IDXL,IDYL);
                    int ID_B = 4*i+j;
                    Omega[2][IDMKL] += Src(ID_B,2);
                }
            }
            break;
        }
        default: break;
    }
}

//--- BC Spec

void Eul_Grid_2D::Set_BC_Positions()
{
    // BDX Faces

    std::vector<Vector>  BD_AX_Nds, BD_BX_Nds;

    for (int i=0; i<ny+1; i++)
    {
        BD_AX_Nds.push_back(Vector3(X_L,        Y_L+i,      0.0)*Grid_Vars->H_Grid);
        BD_BX_Nds.push_back(Vector3(X_L+nx,     Y_L+i,      0.0)*Grid_Vars->H_Grid);
    }

    // BDY Faces

    std::vector<Vector>  BD_AY_Nds, BD_BY_Nds;

    for (int i=0; i<nx+1; i++)
    {
        BD_AY_Nds.push_back(Vector3(X_L+i,      Y_L,        0.0)*Grid_Vars->H_Grid);
        BD_BY_Nds.push_back(Vector3(X_L+i,      Y_L+ny,     0.0)*Grid_Vars->H_Grid);
    }

    // Shift

    for (int i=0; i<BD_AX_Nds.size(); i++)  BD_AX_Nds[i] += Grid_Vars->Origin;
    for (int i=0; i<BD_BX_Nds.size(); i++)  BD_BX_Nds[i] += Grid_Vars->Origin;
    for (int i=0; i<BD_AY_Nds.size(); i++)  BD_AY_Nds[i] += Grid_Vars->Origin;
    for (int i=0; i<BD_BY_Nds.size(); i++)  BD_BY_Nds[i] += Grid_Vars->Origin;

    StdAppend(BD_Nodes,BD_AX_Nds);
    StdAppend(BD_Nodes,BD_AY_Nds);
    StdAppend(BD_Nodes,BD_BX_Nds);
    StdAppend(BD_Nodes,BD_BY_Nds);
}

void Eul_Grid_2D::Set_BC(const StateVector &BC)
{
    // Sorts BCs into 2D array structure

    // The array should have length (nx+2)*(ny+2)-4

    //---------------------------------------------------

    //  Method 1: Streamfunction FF -> Dirichlet BC

    // X-arrays. These represent the first 2*(ny+2) values.

    for (int i=0; i<ny+1; i++)
    {
        BD_AX[0][i] = BC[i](2);
        BD_BX[0][i] = BC[i+ny+1](2);
    }

    // Y-arrays. Remember "edge" points

    BD_AY[0][0] = BD_AX[0][0];
    BD_AY[0][nx] = BD_BX[0][0];

    BD_BY[0][0] = BD_AX[0][ny];
    BD_BY[0][nx] = BD_BX[0][ny];

    int ShiftAY0 = 2*(ny+1);
    int ShiftBY0 = 2*(ny+1)+nx-1;

    for (int i=0; i<nx-1; i++)
    {
        BD_AY[0][i+1] = BC[i+ShiftAY0](2);
        BD_BY[0][i+1] = BC[i+ShiftBY0](2);
    }

    Set_BC_Active();

    return;

    //---------------------------------------------------

    //---------------------------------------------------

    //          Method 2: Vel FF -> Neumann BC

    // X-arrays. These represent the first 2*(ny+2) values.

    // Arrays X = const-> Neumann BC corresponds to dPsi/dX = u_y


    for (int i=0; i<ny+1; i++)
    {
        BD_AX[0][i] = -BC[i](1);
        BD_BX[0][i] = BC[i+ny+1](1);
    }

    // Y-arrays. Remember "edge" points
    // Arrays Y = const-> Neumann BC corresponds to dPsi/dy = u_x

    BD_AY[0][0] = BC[0](0);
    BD_AY[0][nx] = BC[ny+1](0);

    BD_BY[0][0] = -BC[ny](0);
    BD_BY[0][nx] = -BC[2*ny+1](0);

    int ShiftAY = 2*(ny+1);
    int ShiftBY = 2*(ny+1)+nx-1;


    for (int i=0; i<nx-1; i++)
    {
        BD_AY[0][i+1] = BC[i+ShiftAY](0);
        BD_BY[0][i+1] = -BC[i+ShiftBY](0);
    }

    Set_BC_Active();

    //---------------------------------------------------
}

void Eul_Grid_2D::Get_Boundary_Nodes(StateVector &Bs)
{
    // Extracts BC nodes, then converts them to a statevec. This way the original nodes are
    // not influences

//    Bs.insert(Bs.end(),BD_AX_Nds.begin(),BD_AX_Nds.end());
//    Bs.insert(Bs.end(),BD_BX_Nds.begin(),BD_BX_Nds.end());
//    Bs.insert(Bs.end(),BD_AY_Nds.begin()+1,BD_AY_Nds.end()-1);
//    Bs.insert(Bs.end(),BD_BY_Nds.begin()+1,BD_BY_Nds.end()-1);

    StdAppend(Bs,BD_Nodes);
}

//--- Poisson JL Functions ---

void Eul_Grid_2D::Set_Boundary_Gradient()
{
    // Within the context of the James-Lackner algorithm this function calculates the normal derivative on
    // the surface of the domain

    // We use a higher order derivative (third order) to capture the derivate on the boundary
    // as this is important for the single layer portion of the code

    // We can reduce the number of calcs here as, by design the values of the gradient at the
    // limits will be zero

    //    BG.setZero();

    int NShift;

    // X Faces
    NShift = 0;
    for (int j=1; j<ny; j++)
    {
        int IDJ = j + NShift;

        // Gradient term face 1
        Cart_ID CID1(0,j,0);
        BG(IDJ,2) = -D_DX_3(CID1,Psi[2]);

        // Gradient term face 2
        Cart_ID CID2(nx,j,0);
        BG(IDJ+N_FaceX,2) = D_DX_3(CID2,Psi[2]);
    }

    // Y Faces
    NShift = 2*N_FaceX;
    for (int i=1; i<nx; i++)
    {
        int IDJ = i + NShift;

        // Gradient term face 1
        Cart_ID CID1(i,0,0);
        BG(IDJ,2) = -D_DY_3(CID1,Psi[2]);

        // Gradient term face 2
        Cart_ID CID2(i,ny,0);
        BG(IDJ+N_FaceY,2) = D_DY_3(CID2,Psi[2]);
    }
}

void Eul_Grid_2D::Set_Boundary_Psi0()
{
    // In the case that the Poisson JL solver is being used, the values of Psi0 (including overlap)
    // have to be stored in an array for later use. This is the BPsi0 array.

    int DB = Grid_Vars->OL_OG-Grid_Vars->OL_PG;
    int NX = (nx+1-2*DB);
    int NY = (ny+1-2*DB);

    int NS_FaceX = NY;
    int NS_FaceY = NX;

    // X Faces
    int NShift = 0;
    for (int j=DB; j<ny+1-DB; j++)
    {
        int IDMKL1 = ID_MKL(DB,j);
        int BDA = j-DB;
        BPsi0(BDA,2) = Psi[2][IDMKL1];

        int IDMKL2 = ID_MKL(nx-DB,j);
        int BDB = BDA + NS_FaceX;
        BPsi0(BDB,2) = Psi[2][IDMKL2];
    }

    //--- BDY Faces
    NShift = 2*NS_FaceX;
    for (int i=DB; i<nx+1-DB; i++)
    {
        int IDMKL1 = ID_MKL(i,DB);
        int BDA = NShift + i-DB;
        BPsi0(BDA,2) = Psi[2][IDMKL1];

        int IDMKL2 = ID_MKL(i,ny-DB);
        int BDB = BDA + NS_FaceY;
        BPsi0(BDB,2) = Psi[2][IDMKL2];
    }
}

void Eul_Grid_2D::PoissonJL_Compact_Support()
{
    // The values of vorticity directly on the boundary are stored during the entire process within the Omega array,
    // The value of the temporary Omega (Psi) array for the solver input is set to zero at the boundary and also at the
    // first layer to guarantee compact support of the vorticity within the domain

    // If the overlap is large enough, by nature of the vorticity mapping procedure the omega grid
    // will automatically have compact suppport.

//    int OL = Grid_Vars->OL_OG;
//    int CSBarrier = Grid_Vars->Mapping_Order+1;
//    if (OL > CSBarrier) return;

//    Nullify_Psi(Cart_ID(0,0,0),         Cart_ID(OL,ny+1,nz+1));       // Face 1
//    Nullify_Psi(Cart_ID(nx+1-OL,0,0),   Cart_ID(OL,ny+1,nz+1));       // Face 2
//    Nullify_Psi(Cart_ID(0,0,0),         Cart_ID(nx+1,OL,nz+1));       // Face 3
//    Nullify_Psi(Cart_ID(0,ny+1-OL,0),   Cart_ID(nx+1,OL,nz+1));       // Face 4
//    Nullify_Psi(Cart_ID(0,0,0),         Cart_ID(nx+1,ny+1,OL));       // Face 5
//    Nullify_Psi(Cart_ID(0,0,nz+1-OL),   Cart_ID(nx+1,ny+1,OL));       // Face 6
}

//--- Activity

void    Eul_Grid_2D::Set_Cell_Active(const Cart_ID &CID)
{
    // This is called so often, that I implement a version here for easier readability
    Cart_ID CIDL = CID_Loc(CID);
    uint IDCell = CIDL(0)*ny + CIDL(1);    // Local ID!
    if (Active_Cells[IDCell]) return;
    Active_Cells[IDCell] = true;
    Active_Cells_ID.push_back(CIDL);
}

void    Eul_Grid_2D::Mark_Surr_Node_Activity(const Cart_ID &CID)
{
    // We take the given node and ensure the nodes around it are marked as active
    for (int i=-1; i<=1; i++)
    {
        int CIDX = CID(0)+i;
        if (CIDX<0 || CIDX>nx) continue;
        for (int j=-1; j<=1; j++)
        {
            int CIDY = CID(1)+j;
            if (CIDY<0 || CIDY>ny) continue;
            int IDMKL = ID_MKL(CIDX,CIDY);
            if (!Active_Nodes[IDMKL])
            {
                Active_Nodes[IDMKL]=true;
                Active_Node_ID.push_back(Cart_ID(CIDX,CIDY,0));
            }
        }
    }
}

void    Eul_Grid_2D::Set_Direct_Node_Activity()
{
    // Only used for remeshing
    for (int i=0; i<Active_Cells_ID.size(); i++) Set_Node_Active(Active_Cells_ID[i](0),Active_Cells_ID[i](1),0);
}

void    Eul_Grid_2D::Set_Node_Activity(const GRID_MAP &MAP)
{
    // This specifies node activity based on cell activity

    //    qDebug() << ID_Super(0) << ID_Super(1) << ID_Super(2) << "Eul_Grid::Set_Node_Activity. N Active Cells = " << Active_Cells_ID.size();

    if (MAP==M2)
    {
        for (int i=0; i<Active_Cells_ID.size(); i++)
        {
            Cart_ID CID = Active_Cells_ID[i];
            Set_Node_Active(CID(0)  ,CID(1)  ,0);
            Set_Node_Active(CID(0)  ,CID(1)+1,0);
            Set_Node_Active(CID(0)+1,CID(1)  ,0);
            Set_Node_Active(CID(0)+1,CID(1)+1,0);
        }
    }

    if (MAP==M4D)
    {
        for (int i=0; i<Active_Cells_ID.size(); i++)
        {
            Cart_ID CID = Active_Cells_ID[i];
            Set_Node_Active(CID(0)-1,CID(1)-1,0);
            Set_Node_Active(CID(0)-1,CID(1)  ,0);
            Set_Node_Active(CID(0)-1,CID(1)+1,0);
            Set_Node_Active(CID(0)-1,CID(1)+2,0);

            Set_Node_Active(CID(0)  ,CID(1)-1,0);
            Set_Node_Active(CID(0)  ,CID(1)  ,0);
            Set_Node_Active(CID(0)  ,CID(1)+1,0);
            Set_Node_Active(CID(0)  ,CID(1)+2,0);

            Set_Node_Active(CID(0)+1,CID(1)-1,0);
            Set_Node_Active(CID(0)+1,CID(1)  ,0);
            Set_Node_Active(CID(0)+1,CID(1)+1,0);
            Set_Node_Active(CID(0)+1,CID(1)+2,0);

            Set_Node_Active(CID(0)+2,CID(1)-1,0);
            Set_Node_Active(CID(0)+2,CID(1)  ,0);
            Set_Node_Active(CID(0)+2,CID(1)+1,0);
            Set_Node_Active(CID(0)+2,CID(1)+2,0);
        }
    }
}

void    Eul_Grid_2D::Set_Node_Active(const int &IDX,const int &IDY,const int &IDZ)
{
    // This is called so often, I implement a version here for easier readability
    int IDMKL = ID_MKL(IDX,IDY);
    if (Active_Nodes[IDMKL]) return;
    Active_Nodes[IDMKL] = true;
    Active_Node_ID.push_back(Cart_ID(IDX,IDY,0));
}

//---Post processing

void Eul_Grid_2D::Calc_Finite_Differences()
{
    for (int i=0; i<Active_Node_ID.size(); i++) Set_U_Node(Active_Node_ID[i]);
    Set_U_Active();
    for (int i=0; i<Active_Node_ID.size(); i++) Set_Diffusion_Node(Active_Node_ID[i]);
    Set_LapOmega_Active();
}

void Eul_Grid_2D::Set_U_Node(const Cart_ID &CID)
{
    // Set the value of the Laplacian for this node and stores in the laplacian array.

    // Set the value of the velocity components
    // Currently assuming first order.

    U[0][ID_MKL(CID(0),CID(1))] = -0.5*InvH*(Psi[0][ID_MKL(CID(0),CID(1)+1)] - Psi[0][ID_MKL(CID(0),CID(1)-1)]);
    U[1][ID_MKL(CID(0),CID(1))] =  0.5*InvH*(Psi[0][ID_MKL(CID(0)+1,CID(1))] - Psi[0][ID_MKL(CID(0)-1,CID(1))]);
}

void Eul_Grid_2D::Set_Diffusion_Node(const Cart_ID &CID)
{
    // Set the value of the Laplacian for this node and stores in the laplacian array.
    if (CID(0)<0 || CID(0)>nx) return;
    if (CID(1)<0 || CID(1)>ny) return;

    Real Lp = 0.0;
    Lp += LAP_ISO_2D[0]*Omega[0][ID_MKL(CID(0)-1,CID(1)-1)];
    Lp += LAP_ISO_2D[1]*Omega[0][ID_MKL(CID(0)-1,CID(1))];
    Lp += LAP_ISO_2D[2]*Omega[0][ID_MKL(CID(0)-1,CID(1)+1)];
    Lp += LAP_ISO_2D[3]*Omega[0][ID_MKL(CID(0),CID(1)-1)];
    Lp += LAP_ISO_2D[4]*Omega[0][ID_MKL(CID(0),CID(1))];
    Lp += LAP_ISO_2D[5]*Omega[0][ID_MKL(CID(0),CID(1)+1)];
    Lp += LAP_ISO_2D[6]*Omega[0][ID_MKL(CID(0)+1,CID(1)-1)];
    Lp += LAP_ISO_2D[7]*Omega[0][ID_MKL(CID(0)+1,CID(1))];
    Lp += LAP_ISO_2D[8]*Omega[0][ID_MKL(CID(0)+1,CID(1)+1)];

    DAlphaDt[0][ID_MKL(CID(0),CID(1))] = -InvH2*Lp;  // Rememeber: Omega actually stores -Omega!
}

//--- Finite difference (higher order for Poisson JL method)

Real    Eul_Grid_2D::D_DX_3(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(0)==0)          return      InvH*(  -25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  )]
                                                +4.0*       A[ID_MKL(CID(0)+1,CID(1)  )]
                                               -3.0*       A[ID_MKL(CID(0)+2,CID(1)  )]
                                               +4.0/3.0*   A[ID_MKL(CID(0)+3,CID(1)  )]
                                               -1.0/4.0*   A[ID_MKL(CID(0)+4,CID(1)  )]);
    else if (CID(0)==nx)    return      InvH*(  +25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  )]
                                                -4.0*       A[ID_MKL(CID(0)-1,CID(1)  )]
                                               +3.0*       A[ID_MKL(CID(0)-2,CID(1)  )]
                                               -4.0/3.0*   A[ID_MKL(CID(0)-3,CID(1)  )]
                                               +1.0/4.0*   A[ID_MKL(CID(0)-4,CID(1)  )]);
}

Real    Eul_Grid_2D::D_DY_3(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(1)==0)          return      InvH*(  -25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  )]
                                               +4.0*       A[ID_MKL(CID(0)  ,CID(1)+1)]
                                               -3.0*       A[ID_MKL(CID(0)  ,CID(1)+2)]
                                               +4.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)+3)]
                                               -1.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)+4)]);
    else if (CID(1)==ny)    return      InvH*(  +25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  )]
                                               -4.0*       A[ID_MKL(CID(0)  ,CID(1)-1)]
                                               +3.0*       A[ID_MKL(CID(0)  ,CID(1)-2)]
                                               -4.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)-3)]
                                               +1.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)-4)]);
}

//--- Extraction

void    Eul_Grid_2D::Extract_Grid(Real *Grid, const Cart_ID &C, const Cart_ID &W, StateVector &M)
{
    // We need to superpose the values of Psi on the grids to ensure the solution
    // is continuous. For this we extract a grid with the given dimensions.

    // Note that the value of Psi here is the value still in memory from the Psi_0 (free space solution)
    // solve. This is transferred to the Psi value of the sub grid temporarily for specification of the
    // correct boundary condition during the next solve step.

    int X0 = C(0);
    int Y0 = C(1);

    int WX = W(0);
    int WY = W(1);

    M.assign(WX*WY,Vector3::Zero());

    for (int i=0; i<WX; i++)
    {
        for (int j=0; j<WY; j++)
        {
            int IDM = i*WY + j;
            int IDMKL = ID_MKL(X0+i,Y0+j);

//            M[IDM](0) = Grid[0][IDMKL];
//            M[IDM](1) = Grid[1][IDMKL];
            M[IDM](2) = Grid[IDMKL];
        }
    }
}

void    Eul_Grid_2D::Superpose_Grid(Real *Grid, const Cart_ID &C, const Cart_ID &W, StateVector &M)
{
    // This is the complimentary step to that shown above. The extracted values of Psi0
    // are transferred over and temporarily stored in the output array before they are called during
    // specification of the boundary condition (in function Set_BC_JL()).

    int X0 = C(0);
    int Y0 = C(1);

    int WX = W(0);
    int WY = W(1);

    for (int i=0; i<WX; i++)
    {
        for (int j=0; j<WY; j++)
        {
            int IDM = i*WY + j;
            int IDMKL = ID_MKL(X0+i,Y0+j);
            Grid[IDMKL] += M[IDM](2);
        }
    }

}

void    Eul_Grid_2D::Nullify_Array(const Cart_ID &C, const Cart_ID &W, Real **A)
{
    // This function is called during remeshing. We deliberately set certain section to have zero omega
    // to avoid overlap

    int X0 = C(0);
    int Y0 = C(1);

    int WX = W(0);
    int WY = W(1);

    for (int i=0; i<WX; i++)
    {
        for (int j=0; j<WY; j++)
        {
            int IDMKL = ID_MKL(X0+i,Y0+j);
            A[2][IDMKL] = 0.0;
        }
    }
}

//--- Debugging ---

void Eul_Grid_2D::Debug_BC()
{
//    for (int i=0; i<ny+1; i++) qDebug() << i << BD_AX_Nds[i](0) << BD_AX_Nds[i](1) << BD_AX_Nds[i](2) << BD_AX[0][i];
//    for (int i=0; i<ny+1; i++) qDebug() << i << BD_BX_Nds[i](0) << BD_BX_Nds[i](1) << BD_BX_Nds[i](2) << BD_BX[0][i];
//    for (int i=0; i<nx+1; i++) qDebug() << i << BD_AY_Nds[i](0) << BD_AY_Nds[i](1) << BD_AY_Nds[i](2) << BD_AY[0][i];
//    for (int i=0; i<nx+1; i++) qDebug() << i << BD_BY_Nds[i](0) << BD_BY_Nds[i](1) << BD_BY_Nds[i](2) << BD_BY[0][i];
//    for (int i=0; i<ny+1; i++) qDebug() <<"[ "<< i << " , " << BD_AX_Nds[i](0) << " , " << BD_AX_Nds[i](1) << " , " << BD_AX_Nds[i](2) << " , " << BD_AX[0][i]<< "], ";
//    for (int i=0; i<ny+1; i++) qDebug() <<"[ "<< i << " , " << BD_BX_Nds[i](0) << " , " << BD_BX_Nds[i](1) << " , " << BD_BX_Nds[i](2) << " , " << BD_BX[0][i]<< "], ";
//    for (int i=0; i<nx+1; i++) qDebug() <<"[ "<< i << " , " << BD_AY_Nds[i](0) << " , " << BD_AY_Nds[i](1) << " , " << BD_AY_Nds[i](2) << " , " << BD_AY[0][i]<< "], ";
//    for (int i=0; i<nx+1; i++) qDebug() <<"[ "<< i << " , " << BD_BY_Nds[i](0) << " , " << BD_BY_Nds[i](1) << " , " << BD_BY_Nds[i](2) << " , " << BD_BY[0][i]<< "], ";
}

//--------------------------------------------------------------
//----------------------3D UNBOUND------------------------------
//--------------------------------------------------------------

//--- Initialize grid vars

void Eul_Grid_3D::Init_Grid()
{
    // Initialize 3D grid

    // Boundary positions

    ax = X_L*Grid_Vars->H_Grid;
    bx = (X_L+nx)*Grid_Vars->H_Grid;

    ay = Y_L*Grid_Vars->H_Grid;
    by = (Y_L+ny)*Grid_Vars->H_Grid;

    az = Z_L*Grid_Vars->H_Grid;
    bz = (Z_L+nz)*Grid_Vars->H_Grid;

    // Allocate memory blocks -- spar, f, bd_a..... ( Internal var Helmholtz Solver, RHS, BCs)

    spar=(Real*)malloc((13*(nx+ny)/2+9)*sizeof(Real));
//    f=(Real*)calloc((nx+1)*(ny+1)*(nz+1)*sizeof(Real),sizeof(Real));
//    f0=(Real*)calloc((nx+1)*(ny+1)*(nz+1)*sizeof(Real),sizeof(Real));

    N_Points = (nx+1)*(ny+1)*(nz+1);
    N_FaceX = (ny+1)*(nz+1);
    N_FaceY = (nx+1)*(nz+1);
    N_FaceZ = (nx+1)*(ny+1);
    N_BPoints = 2*N_FaceX + 2*N_FaceY + 2*N_FaceZ;
    N_Cells = nx*ny*nz;

    BD_AX = new Real*[3];
    BD_BX = new Real*[3];
    BD_AY = new Real*[3];
    BD_BY = new Real*[3];
    BD_AZ = new Real*[3];
    BD_BZ = new Real*[3];

    for (int i=0; i<3; i++)
    {
        BD_AX[i] = (Real*)malloc(N_FaceX*sizeof(Real));
        BD_BX[i] = (Real*)malloc(N_FaceX*sizeof(Real));
        BD_AY[i] = (Real*)malloc(N_FaceY*sizeof(Real));
        BD_BY[i] = (Real*)malloc(N_FaceY*sizeof(Real));
        BD_AZ[i] = (Real*)malloc(N_FaceZ*sizeof(Real));
        BD_BZ[i] = (Real*)malloc(N_FaceZ*sizeof(Real));
    }

    // Set finite diff vals
    H2 = Grid_Vars->H_Grid*Grid_Vars->H_Grid;
    H3 = Grid_Vars->H_Grid*Grid_Vars->H_Grid*Grid_Vars->H_Grid;
    InvH = 1.0/Grid_Vars->H_Grid;
    InvH2 = InvH*InvH;

//    qDebug() << H2 << InvH2;
//    for (int i=0; i<27; i++)  qDebug() << LAP_ISO_3D[i];

    // Set q value (0 for Poisson problem)
    q = 0;

    // Initializing ipar array to make it free from garbage
    for (int i=0;i<128;i++)  ipar[i]=0;

    // Allocate arrays

    Psi = new Real*[3];
    for (int i=0; i<3; i++) Psi[i] = (Real*)malloc(N_Points*sizeof(Real));

    Omega = new Real*[3];
    for (int i=0; i<3; i++) Omega[i] = (Real*)malloc(N_Points*sizeof(Real));
  
    Active_Nodes = new bool[N_Points];          // Allocate activity array
    Active_Cells = new bool[N_Cells];           // Allocate activity array
    Active_Volume_Cells = new bool[N_Cells];    // Allocate activity array

    // Additional steps if the JL algorithms being used
//    if (Vars->Solver==PoissonJL) BG = MatrixRM::Zero(N_BPoints,3);

    Set_BC_Positions();                 // Set initial BC vals

    // Reset initial arrays

    Reset_Array3D(BD_AX,N_FaceX);
    Reset_Array3D(BD_BX,N_FaceX);
    Reset_Array3D(BD_AY,N_FaceY);
    Reset_Array3D(BD_BY,N_FaceY);
    Reset_Array3D(BD_AZ,N_FaceZ);
    Reset_Array3D(BD_BZ,N_FaceZ);
    Reset_Array3D(Psi,N_Points);
    Reset_Array3D(Omega,N_Points);
    memset(Active_Nodes, false, N_Points*sizeof(bool));
    memset(Active_Cells, false, N_Cells*sizeof(bool));
    memset(Active_Volume_Cells, false, N_Cells*sizeof(bool));

//    memset(ipar, 0, 128*sizeof(MKL_INT));       // Clear integer params setting
//    Init_Solver(1);           // Init Solver
}

void Eul_Grid_3D::Allocate_Output_Arrays()
{
    // This function is sepcified as depending on the method of solution,
    // we may not need to allocate output arrays at all. For the Poisson JL algorithm,
    // we only define the values of Psi and Omega over the volume for the primary grid (free space solution)
    // The sub grid however requires storage of the output arrays for the Finite diff calcs.

    U = new Real*[3];
    for (int i=0; i<3; i++) U[i] = (Real*)malloc(N_Points*sizeof(Real));

    DAlphaDt = new Real*[3];
    for (int i=0; i<3; i++) DAlphaDt[i] = (Real*)malloc(N_Points*sizeof(Real));

    if (Vars->Turb_Model==SMG)
    {
        LapOmega = new Real*[3];
        for (int i=0; i<3; i++) LapOmega[i] = (Real*)malloc(N_Points*sizeof(Real));
    }

    if (Vars->Turb_Model==RVM)
    {
        Omega_SS = new Real*[3];
        for (int i=0; i<3; i++) Omega_SS[i] = (Real*)malloc(N_Points*sizeof(Real));
        Omega_SS2 = new Real*[3];
        for (int i=0; i<3; i++) Omega_SS2[i] = (Real*)malloc(N_Points*sizeof(Real));
        Nu_SGS = (Real*)malloc(N_Points*sizeof(Real));
    }

    // Set arrays to be zero

    Reset_Array3D(U,N_Points);             U_Active = false;
    Reset_Array3D(DAlphaDt,N_Points);      DAlphaDt_Active = false;
    if (Vars->Turb_Model==SMG)
    {
        Reset_Array3D(LapOmega,N_Points);      LapOmega_Active = false;
    }
    if (Vars->Turb_Model==RVM)
    {
        Reset_Array(Nu_SGS,N_Points);      Nu_SGS_Active = false;
        Reset_Array3D(Omega_SS,N_Points);  Reset_Array3D(Omega_SS2,N_Points); Omega_SS_Active = false;
    }

    OutputArrays = true;
}

//void Eul_Grid_3D::DeAllocate_Output_Arrays()
//{
//    // In case a region is no longer being used, the output arrays are
//    // cleared in order to decrease memory overhead.

//    if ((Vars->Timestep-Last_PA_Timestep)<20)   return;
//    if (!OutputArrays)                          return; // Don't bother is output arrays are not allocated

//    for (int i=0; i<3; i++)
//    {
//        free(U[i]);
//        free(DAlphaDt[i]);
//        if (Vars->Turb_Model==SMG)  free(LapOmega[i]);
//        if (Vars->Turb_Model==RVM)  free(Omega_SS[i]);
//        if (Vars->Turb_Model==RVM)  free(Omega_SS2[i]);
//    }
//    if (Vars->Turb_Model==RVM)  free(Nu_SGS);

////    Vars->N_Cleared_Arrays++;
//    OutputArrays = false;
//}

//--- Solver

void Eul_Grid_3D::Solve_Poisson(const int &Dim)
{
    // Solves the Poisson equation in the given dimension

    // This executes the fast poisson Solver using the in-built library in the Intel Math Kernel Library
    // As the library is not set up to Solve 3D inputs, we must Solve equation for each Vars->Dimension.

    // Trial flups

//    int  axis      = 0;              // aligned along the first dimension
//    int  lda       = 1;              // scalar field
//    int  nglob[3]  = {64, 128, 64};  // global size of 64x64x64
//    int  nproc[3]  = {2, 1, 3};      // 6 procs; 2 x 1 x 3
//    bool isComplex = false;          // real data

//    // no specific alignement => we put a value of 1
//    FLUPS_Topology *topo = flups_topo_new(axis, lda, nglob, nproc, isComplex, NULL, 1, MPI_COMM_WORLD);

//    // define additional quantities
//    double L[3] = {1.0, 2.0, 1.0};
//    double h[3] = {L[0] / nglob[0], L[1] / nglob[1], L[2] / nglob[2]};

    // Notes:   "commit" modifies f, ipar, spar
    //          "helmholtz" modifies only f

    #ifdef MKLFastPoiss
        memset(ipar, 0, 128*sizeof(MKL_INT));       // Clear integer params setting
    #else
        memset(ipar, 0, 128*sizeof(uint));       // Clear integer params setting
    #endif
    Init_Solver(Dim);           // Init Solver
//    ipar[24] = 16;              // Set number of threads // Leave this alone....
    Commit_Arrays(Dim);         // Commit arrays to carry out checks
    Execute_Solver(Dim);        // Execute Solver
    Set_Psi_Active();           // Set Psi grid active
}

void Eul_Grid_3D::Init_Solver(const int &Dim)
{
    #ifdef MKLFastPoiss
        #ifdef SinglePrec
            s_init_Helmholtz_3D(&ax, &bx, &ay, &by, &az, &bz, &nx, &ny, &nz, BCtype, &q, ipar, spar, &stat);
        #endif

        #ifdef DoublePrec
            d_init_Helmholtz_3D(&ax, &bx, &ay, &by, &az, &bz, &nx, &ny, &nz, BCtype, &q, ipar, spar, &stat);
        #endif

        if (stat!=0)
        {
            qDebug() << "Eul_Grid_3D::Init_Solver init has failed.";
            return;
        }
    #endif
}

void Eul_Grid_3D::Commit_Arrays(const int &Dim)
{
    #ifdef MKLFastPoiss
    //    qDebug() << ipar[24] << mkl_get_max_threads();    // Requires mkl.h header
        #ifdef SinglePrec
            s_commit_Helmholtz_3D(Psi[Dim], BD_AX[Dim], BD_BX[Dim], BD_AY[Dim], BD_BY[Dim], BD_AZ[Dim], BD_BZ[Dim], &xhandle, &yhandle, ipar, spar, &stat);
        #endif

        #ifdef DoublePrec
            d_commit_Helmholtz_3D(Psi[Dim], BD_AX[Dim], BD_BX[Dim], BD_AY[Dim], BD_BY[Dim], BD_AZ[Dim], BD_BZ[Dim], &xhandle, &yhandle, ipar, spar, &stat);
        #endif

    //    ipar[2]=0;                  // Do not output error message for Neuemann BC solver!

        if (stat!=0)
        {
            qDebug() << "Eul_Grid_3D::Commit_Arrays commit has failed.";
            return;
        }
    #endif
}

void Eul_Grid_3D::Execute_Solver(const int &Dim)
{
    #ifdef MKLFastPoiss
        #ifdef SinglePrec
            s_Helmholtz_3D(Psi[Dim], BD_AX[Dim], BD_BX[Dim], BD_AY[Dim], BD_BY[Dim], BD_AZ[Dim], BD_BZ[Dim], &xhandle, &yhandle, ipar, spar, &stat);
        #endif

        #ifdef DoublePrec
            d_Helmholtz_3D(Psi[Dim], BD_AX[Dim], BD_BX[Dim], BD_AY[Dim], BD_BY[Dim], BD_AZ[Dim], BD_BZ[Dim], &xhandle, &yhandle, ipar, spar, &stat);
        #endif

        if (stat!=0 && ipar[2]!=0)
        {
            qDebug() << "Eul_Grid_3D::Execute_Solver Solve has failed.";
            return;
        }
    #endif
}

//--- Set Omega

void Eul_Grid_3D::Increment_Omega(const Cart_ID &CID, const Matrix &Src, const GRID_MAP &Map)
{
    // This is a general mapping function. depending on the choice of mapping,
    // the nodes grids are incremeneted appropriately
    // CID is the neighboring particle ID, Src is the interpolating matrix.
    // This is calculated elsewhere, as sometimes we need to map to multiple grids

    Cart_ID S = CID_Loc(CID);

    switch (Map)
    {
        case (M0):
        {
            int IDMKL = ID_MKL(S(0),S(1),S(2));
            Omega[0][IDMKL] += Src(0);
            Omega[1][IDMKL] += Src(1);
            Omega[2][IDMKL] += Src(2);
            break;
        }
        case (M2):
        {
            for (int i=0; i<2; i++)
            {
                int IDXL = S(0)+MPS02X[i];
                for (int j=0; j<2; j++)
                {
                    int IDYL = S(1)+MPS02X[j];
                    for (int k=0; k<2; k++)
                    {
                        int IDZL = S(2)+MPS02X[k];
                        int IDMKL = ID_MKL(IDXL,IDYL,IDZL);
                        int ID_B = 4*i+2*j+k;
                        Omega[0][IDMKL] += Src(ID_B,0);
                        Omega[1][IDMKL] += Src(ID_B,1);
                        Omega[2][IDMKL] += Src(ID_B,2);
                    }
                }
            }
            break;
        }
        case (M4D):
        {
            for (int i=0; i<4; i++)
            {
                int IDXL = S(0)+MPS12X[i];
                for (int j=0; j<4; j++)
                {
                    int IDYL = S(1)+MPS12X[j];
                    for (int k=0; k<4; k++)
                    {
                        int IDZL = S(2)+MPS12X[k];
                        int IDMKL = ID_MKL(IDXL,IDYL,IDZL);
                        int ID_B = 16*i+4*j+k;
                        Omega[0][IDMKL] += Src(ID_B,0);
                        Omega[1][IDMKL] += Src(ID_B,1);
                        Omega[2][IDMKL] += Src(ID_B,2);
                    }
                }
            }
            break;
        }
        default: break;
    }
}

//--- BC Spec

void Eul_Grid_3D::Set_BC_Positions()
{
    // We set BC positions here for the 3D case

    // BDX Faces
    std::vector<Vector>  BD_AX_Nds, BD_BX_Nds;

    for (int j=0; j<ny+1; j++)
    {
        for (int k=0; k<nz+1; k++)
        {
            BD_AX_Nds.push_back(Node_Pos_Global(Origin_Glob + Cart_ID(0,j,k)));
            BD_BX_Nds.push_back(Node_Pos_Global(Origin_Glob + Cart_ID(nx,j,k)));
        }
    }

    // BDY Faces
    std::vector<Vector>  BD_AY_Nds, BD_BY_Nds;

    for (int i=0; i<nx+1; i++)
    {
        for (int k=0; k<nz+1; k++)
        {
            BD_AY_Nds.push_back(Node_Pos_Global(Origin_Glob + Cart_ID(i,0,k)));
            BD_BY_Nds.push_back(Node_Pos_Global(Origin_Glob + Cart_ID(i,ny,k)));
        }
    }

    // BDZ Faces
    std::vector<Vector>  BD_AZ_Nds, BD_BZ_Nds;

    for (int i=0; i<nx+1; i++)
    {
        for (int j=0; j<ny+1; j++)
        {
            BD_AZ_Nds.push_back(Node_Pos_Global(Origin_Glob + Cart_ID(i,j,0)));
            BD_BZ_Nds.push_back(Node_Pos_Global(Origin_Glob + Cart_ID(i,j,nz)));
        }
    }

//    qDebug() << "BC Arrays";

//    for (int i=0; i<N_FaceX; i++)   qDebug() << BD_AX_Nds[i](0) << BD_AX_Nds[i](1) << BD_AX_Nds[i](2);
//    for (int i=0; i<N_FaceX; i++)   qDebug() << BD_BX_Nds[i](0) << BD_BX_Nds[i](1) << BD_BX_Nds[i](2);
//    for (int i=0; i<N_FaceY; i++)   qDebug() << BD_AY_Nds[i](0) << BD_AY_Nds[i](1) << BD_AY_Nds[i](2);
//    for (int i=0; i<N_FaceY; i++)   qDebug() << BD_BY_Nds[i](0) << BD_BY_Nds[i](1) << BD_BY_Nds[i](2);
//    for (int i=0; i<N_FaceZ; i++)   qDebug() << BD_AZ_Nds[i](0) << BD_AZ_Nds[i](1) << BD_AZ_Nds[i](2);
//    for (int i=0; i<N_FaceZ; i++)   qDebug() << BD_BZ_Nds[i](0) << BD_BZ_Nds[i](1) << BD_BZ_Nds[i](2);

    StdAppend(BD_Nodes,BD_AX_Nds);
    StdAppend(BD_Nodes,BD_BX_Nds);
    StdAppend(BD_Nodes,BD_AY_Nds);
    StdAppend(BD_Nodes,BD_BY_Nds);
    StdAppend(BD_Nodes,BD_AZ_Nds);
    StdAppend(BD_Nodes,BD_BZ_Nds);
}

void Eul_Grid_3D::Get_Boundary_Nodes(StateVector &Bs)
{
    // Extracts BC nodes, then converts them to a statevec. This way the original nodes are
    // not influences

    StdAppend(Bs,BD_Nodes);

//    StdAppend(Bs,BD_AX_Nds);
//    StdAppend(Bs,BD_BX_Nds);
//    StdAppend(Bs,BD_AY_Nds);
//    StdAppend(Bs,BD_BY_Nds);
//    StdAppend(Bs,BD_AZ_Nds);
//    StdAppend(Bs,BD_BZ_Nds);
}

void Eul_Grid_3D::Set_BC(const StateVector &BC)
{
    // This function imports the boundary values based on the BC vector.
    // The vector is compressed to avoid repetition of points.
    // Keep in mind the solver for intel MKL: Need to set value as negative Psi...

    int NShift;
    Real H = Grid_Vars->H_Grid;

    //--- BDX Faces

    // X Faces
    NShift = 0;

//    qDebug() << "FaceX";

//    OpenMPfor
    for (int j=0; j<ny+1; j++)
    {
        for (int k=0; k<nz+1; k++)
        {
            int IDMKL = j+k*(ny+1);
            int BDA = j*(nz+1) + k;

            BD_AX[0][IDMKL] = BC[BDA](0);
            BD_AX[1][IDMKL] = BC[BDA](1);
            BD_AX[2][IDMKL] = BC[BDA](2);

            int BDB = BDA + N_FaceX;
            BD_BX[0][IDMKL] = BC[BDB](0);
            BD_BX[1][IDMKL] = BC[BDB](1);
            BD_BX[2][IDMKL] = BC[BDB](2);

//            qDebug() << 0.0 << j*H << k*H << BD_AX[0][IDMKL] << BD_AX[1][IDMKL] << BD_AX[2][IDMKL];
//            qDebug() << nx*H << j*H << k*H << BD_BX[0][IDMKL] << BD_BX[1][IDMKL] << BD_BX[2][IDMKL];
        }
    }

//    qDebug() << "FaceY";

    //--- BDY Faces
    NShift = 2*N_FaceX;

//    OpenMPfor
    for (int i=0; i<nx+1; i++)
    {
        for (int k=0; k<nz+1; k++)
        {
            int IDMKL = i+k*(nx+1);
            int BDA = NShift + i*(nz+1) + k;

            BD_AY[0][IDMKL] = BC[BDA](0);
            BD_AY[1][IDMKL] = BC[BDA](1);
            BD_AY[2][IDMKL] = BC[BDA](2);

            int BDB = BDA + N_FaceY;
            BD_BY[0][IDMKL] = BC[BDB](0);
            BD_BY[1][IDMKL] = BC[BDB](1);
            BD_BY[2][IDMKL] = BC[BDB](2);

//            if (i==nx/2)
//            {
//                qDebug() << i*H << 0.0 << k*H << BD_AY[0][IDMKL] << BD_AY[1][IDMKL] << BD_AY[2][IDMKL];
//                qDebug() << i*H << ny*H << k*H << BD_BY[0][IDMKL] << BD_BY[1][IDMKL] << BD_BY[2][IDMKL];
//            }

//            qDebug() << i*H << 0.0 << k*H << BD_AY[0][IDMKL] << BD_AY[1][IDMKL] << BD_AY[2][IDMKL];
//            qDebug() << i*H << ny*H << k*H << BD_BY[0][IDMKL] << BD_BY[1][IDMKL] << BD_BY[2][IDMKL];
        }
    }

//    qDebug() << "FaceZ";

    //--- BDZ Faces
    NShift = 2*N_FaceX+2*N_FaceY;

//    OpenMPfor
    for (int i=0; i<nx+1; i++)
    {
        for (int j=0; j<ny+1; j++)
        {
            int IDMKL = i+j*(nx+1);
            int BDA = NShift + i*(ny+1) + j;

            BD_AZ[0][IDMKL] = BC[BDA](0);
            BD_AZ[1][IDMKL] = BC[BDA](1);
            BD_AZ[2][IDMKL] = BC[BDA](2);

            int BDB = BDA + N_FaceZ;
            BD_BZ[0][IDMKL] = BC[BDB](0);
            BD_BZ[1][IDMKL] = BC[BDB](1);
            BD_BZ[2][IDMKL] = BC[BDB](2);

//            qDebug() << i*H << j*H << 0.0 << BD_AZ[0][IDMKL] << BD_AZ[1][IDMKL] << BD_AZ[2][IDMKL];
//            qDebug() << i*H << j*H << nz*H << BD_BZ[0][IDMKL] << BD_BZ[1][IDMKL] << BD_BZ[2][IDMKL];
        }
    }

    Set_BC_Active();
}

void Eul_Grid_3D::Set_BC_Monolithic()
{
    // I have hijacked the Psi grid for the monlithic grid solve. This is then used to fill the BC arrays.

    Real H = Grid_Vars->H_Grid;

    //--- BDX Faces

//    qDebug() << ny+1 << nz+1 << (nz+1)*(ny+1);

//    qDebug() << "FaceX";

//    qDebug() << BCtype[0] << BCtype[1] << BCtype[2] << BCtype[3] << BCtype[4] << BCtype[5];

    OpenMPfor
    for (int j=0; j<ny+1; j++)
    {
        for (int k=0; k<nz+1; k++)
        {
            int IDMKL = j+k*(ny+1);

            int ID_MKLBD1 = ID_MKL(0,j,k);
            BD_AX[0][IDMKL] = Psi[0][ID_MKLBD1];
            BD_AX[1][IDMKL] = Psi[1][ID_MKLBD1];
            BD_AX[2][IDMKL] = Psi[2][ID_MKLBD1];

            int ID_MKLBD2 = ID_MKL(nx,j,k);
            BD_BX[0][IDMKL] = Psi[0][ID_MKLBD2];
            BD_BX[1][IDMKL] = Psi[1][ID_MKLBD2];
            BD_BX[2][IDMKL] = Psi[2][ID_MKLBD2];

//             qDebug() << (X_L+0)*H << (Y_L+j)*H << (Z_L+k)*H << BD_AX[0][IDMKL] << BD_AX[1][IDMKL] << BD_AX[2][IDMKL];
//             qDebug() << (X_L+nx)*H << (Y_L+j)*H << (Z_L+k)*H << BD_BX[0][IDMKL] << BD_BX[1][IDMKL] << BD_BX[2][IDMKL];
        }
    }

    //--- BDY Faces

//    qDebug() << "FaceY";

    OpenMPfor
    for (int i=0; i<nx+1; i++)
    {
        for (int k=0; k<nz+1; k++)
        {
            int IDMKL = i+k*(nx+1);

            int ID_MKLBD1 = ID_MKL(i,0,k);
            BD_AY[0][IDMKL] = Psi[0][ID_MKLBD1];
            BD_AY[1][IDMKL] = Psi[1][ID_MKLBD1];
            BD_AY[2][IDMKL] = Psi[2][ID_MKLBD1];

            int ID_MKLBD2 = ID_MKL(i,ny,k);
            BD_BY[0][IDMKL] = Psi[0][ID_MKLBD2];
            BD_BY[1][IDMKL] = Psi[1][ID_MKLBD2];
            BD_BY[2][IDMKL] = Psi[2][ID_MKLBD2];

//            qDebug() << (X_L+i)*H << (Y_L+0)*H << (Z_L+k)*H << BD_AY[0][IDMKL] << BD_AY[1][IDMKL] << BD_AY[2][IDMKL];
//            qDebug() << (X_L+i)*H << (Y_L+ny)*H << (Z_L+k)*H << BD_BY[0][IDMKL] << BD_BY[1][IDMKL] << BD_BY[2][IDMKL];
        }
    }

    //--- BDZ Faces

//    qDebug() << "FaceZ";

    OpenMPfor
    for (int i=0; i<nx+1; i++)
    {
        for (int j=0; j<ny+1; j++)
        {
            int IDMKL = i+j*(nx+1);

            int ID_MKLBD1 = ID_MKL(i,j,0);
            BD_AZ[0][IDMKL] = Psi[0][ID_MKLBD1];
            BD_AZ[1][IDMKL] = Psi[1][ID_MKLBD1];
            BD_AZ[2][IDMKL] = Psi[2][ID_MKLBD1];

            int ID_MKLBD2 = ID_MKL(i,j,nz);
            BD_BZ[0][IDMKL] = Psi[0][ID_MKLBD2];
            BD_BZ[1][IDMKL] = Psi[1][ID_MKLBD2];
            BD_BZ[2][IDMKL] = Psi[2][ID_MKLBD2];

//            qDebug() << (X_L+i)*H << (Y_L+j)*H << (Z_L+0)*H << BD_AZ[0][IDMKL] << BD_AZ[1][IDMKL] << BD_AZ[2][IDMKL];
//            qDebug() << (X_L+i)*H << (Y_L+j)*H << (Z_L+nz)*H << BD_BZ[0][IDMKL] << BD_BZ[1][IDMKL] << BD_BZ[2][IDMKL];
        }
    }

    Set_BC_Active();
}

//--- Poisson JL Functions ---

void Eul_Grid_3D::Set_Boundary_Gradient()
{
    // Within the context of the James-Lackner algorithm this function calculates the normal derivative on
    // the surface of the domain

    // We use a higher order derivative (third order) to capture the derivate on the boundary
    // as this is important for the single layer portion of the code

    // We can reduce the number of calcs here as, by design the values of the gradient at the
    // limits will be zero

//    BG.setZero();

    int NShift;

    // X Faces
    NShift = 0;
    for (int j=1; j<ny; j++)
    {
        for (int k=1; k<nz; k++)
        {
            int IDJ = j*(nz+1)+k + NShift;

            // Gradient term face 1
            Cart_ID CID1(0,j,k);
            BG(IDJ,0) = -D_DX_3(CID1,Psi[0]);
            BG(IDJ,1) = -D_DX_3(CID1,Psi[1]);
            BG(IDJ,2) = -D_DX_3(CID1,Psi[2]);

            // Gradient term face 2
            Cart_ID CID2(nx,j,k);
            BG(IDJ+N_FaceX,0) = D_DX_3(CID2,Psi[0]);
            BG(IDJ+N_FaceX,1) = D_DX_3(CID2,Psi[1]);
            BG(IDJ+N_FaceX,2) = D_DX_3(CID2,Psi[2]);
        }
    }

    // Y Faces
    NShift = 2*N_FaceX;
    for (int i=1; i<nx; i++)
    {
        for (int k=1; k<nz; k++)
        {

            int IDJ = i*(nz+1)+k + NShift;

            // Gradient term face 1
            Cart_ID CID1(i,0,k);
            BG(IDJ,0) = -D_DY_3(CID1,Psi[0]);
            BG(IDJ,1) = -D_DY_3(CID1,Psi[1]);
            BG(IDJ,2) = -D_DY_3(CID1,Psi[2]);

            // Gradient term face 2
            Cart_ID CID2(i,ny,k);
            BG(IDJ+N_FaceY,0) = D_DY_3(CID2,Psi[0]);
            BG(IDJ+N_FaceY,1) = D_DY_3(CID2,Psi[1]);
            BG(IDJ+N_FaceY,2) = D_DY_3(CID2,Psi[2]);
        }
    }

    // Z Faces
    NShift = 2*N_FaceX + 2*N_FaceY;
    for (int i=1; i<nx; i++)
    {
        for (int j=1; j<ny; j++)
        {
            int IDJ = i*(ny+1)+j + NShift;

            // Gradient term face 1
            Cart_ID CID1(i,j,0);
            BG(IDJ,0) = -D_DZ_3(CID1,Psi[0]);
            BG(IDJ,1) = -D_DZ_3(CID1,Psi[1]);
            BG(IDJ,2) = -D_DZ_3(CID1,Psi[2]);

            // Gradient term face 2
            Cart_ID CID2(i,j,nz);
            BG(IDJ+N_FaceZ,0) = D_DZ_3(CID2,Psi[0]);
            BG(IDJ+N_FaceZ,1) = D_DZ_3(CID2,Psi[1]);
            BG(IDJ+N_FaceZ,2) = D_DZ_3(CID2,Psi[2]);
        }
    }
}

void Eul_Grid_3D::Set_Boundary_Psi0()
{
    // In the case that the Poisson JL solver is being used, the values of Psi0 (including overlap)
    // have to be stored in an array for later use. This is the BPsi0 array.

    int DB = Grid_Vars->OL_OG-Grid_Vars->OL_PG;

    int NX = (nx+1-2*DB);
    int NY = (ny+1-2*DB);
    int NZ = (nz+1-2*DB);

    int NS_FaceX = NY*NZ;
    int NS_FaceY = NX*NZ;
    int NS_FaceZ = NX*NY;

    // X Faces
    int NShift = 0;
    for (int j=DB; j<ny+1-DB; j++)
    {
        for (int k=DB; k<nz+1-DB; k++)
        {
            int IDMKL1 = ID_MKL(DB,j,k);
            int BDA = (j-DB)*NZ + (k-DB);

            BPsi0(BDA,0) = Psi[0][IDMKL1];
            BPsi0(BDA,1) = Psi[1][IDMKL1];
            BPsi0(BDA,2) = Psi[2][IDMKL1];

            int IDMKL2 = ID_MKL(nx-DB,j,k);
            int BDB = BDA + NS_FaceX;
            BPsi0(BDB,0) = Psi[0][IDMKL2];
            BPsi0(BDB,1) = Psi[1][IDMKL2];
            BPsi0(BDB,2) = Psi[2][IDMKL2];
        }
    }

    //--- BDY Faces
    NShift = 2*NS_FaceX;
    for (int i=DB; i<nx+1-DB; i++)
    {
        for (int k=DB; k<nz+1-DB; k++)
        {
            int IDMKL1 = ID_MKL(i,DB,k);
            int BDA = NShift + (i-DB)*NZ + (k-DB);

            BPsi0(BDA,0) = Psi[0][IDMKL1];
            BPsi0(BDA,1) = Psi[1][IDMKL1];
            BPsi0(BDA,2) = Psi[2][IDMKL1];

            int IDMKL2 = ID_MKL(i,ny-DB,k);
            int BDB = BDA + NS_FaceY;
            BPsi0(BDB,0) = Psi[0][IDMKL2];
            BPsi0(BDB,1) = Psi[1][IDMKL2];
            BPsi0(BDB,2) = Psi[2][IDMKL2];
        }
    }

    //--- BDZ Faces
    NShift = 2*NS_FaceX+2*NS_FaceY;
    for (int i=DB; i<nx+1-DB; i++)
    {
        for (int j=DB; j<ny+1-DB; j++)
        {
            int IDMKL1 = ID_MKL(i,j,DB);
            int BDA = NShift + (i-DB)*NY + (j-DB);

            BPsi0(BDA,0) = Psi[0][IDMKL1];
            BPsi0(BDA,1) = Psi[1][IDMKL1];
            BPsi0(BDA,2) = Psi[2][IDMKL1];

            int IDMKL2 = ID_MKL(i,j,nz-DB);
            int BDB = BDA + NS_FaceZ;
            BPsi0(BDB,0) = Psi[0][IDMKL2];
            BPsi0(BDB,1) = Psi[1][IDMKL2];
            BPsi0(BDB,2) = Psi[2][IDMKL2];
        }
    }
}

void Eul_Grid_3D::PoissonJL_Compact_Support()
{
    // The values of vorticity directly on the boundary are stored during the entire process within the Omega array,
    // The value of the temporary Omega (Psi) array for the solver input is set to zero at the boundary and also at the
    // first layer to guarantee compact support of the vorticity within the domain

    // If the overlap is large enough, by nature of the vorticity mapping procedure the omega grid
    // will automatically have compact suppport.

    int OL = Grid_Vars->OL_OG - Grid_Vars->OL_PG;
//    int CSBarrier = Grid_Vars->Mapping;
    if (OL>1) return;

    Nullify_Psi(Cart_ID(0,0,0),         Cart_ID(OL,ny+1,nz+1));       // Face 1
    Nullify_Psi(Cart_ID(nx+1-OL,0,0),   Cart_ID(OL,ny+1,nz+1));       // Face 2
    Nullify_Psi(Cart_ID(0,0,0),         Cart_ID(nx+1,OL,nz+1));       // Face 3
    Nullify_Psi(Cart_ID(0,ny+1-OL,0),   Cart_ID(nx+1,OL,nz+1));       // Face 4
    Nullify_Psi(Cart_ID(0,0,0),         Cart_ID(nx+1,ny+1,OL));       // Face 5
    Nullify_Psi(Cart_ID(0,0,nz+1-OL),   Cart_ID(nx+1,ny+1,OL));       // Face 6
}

//--- Monolithic grid

void Eul_Grid_3D::Increment_Omega_Node(const Cart_ID &CID, const Vector3 &Src)
{
    int IDL = ID_MKL(CID);
    Omega[0][IDL] += Src(0);
    Omega[1][IDL] += Src(1);
    Omega[2][IDL] += Src(2);
}

void Eul_Grid_3D::Set_Omega_Node(const int &i, const Vector3 &Src)
{
    Omega[0][i] = Src(0);
    Omega[1][i] = Src(1);
    Omega[2][i] = Src(2);
}

#ifdef OPENCL
void Eul_Grid_3D::Set_Omega_Grid(std::vector<cl_float3> &Omega_Grid)
{
    // This function transfers the grid from the OpenCL output
    // to the Eulerian grid.

//    qDebug() << "set omega grid " << N_Points;
//    bool NONZRO=false;
//    for (int i=0; i<Omega_Grid.size(); i++)
//    {
//        Vector3 O(Omega_Grid[i].x,Omega_Grid[i].y,Omega_Grid[i].z);
//        if (O.norm()!=0) NONZRO = true;
//    }
//    if (NONZRO) qDebug() << "Vector has nonzero element";

    OpenMPfor
    for (int i=0; i<N_Points; i++)
    {
        Omega[0][i] = Omega_Grid[i].x;
        Omega[1][i] = Omega_Grid[i].y;
        Omega[2][i] = Omega_Grid[i].z;
    }

    Set_Omega_Active();

//    if (Vars->Periodic)         // Correction for other case...
//    {
//        for (int j=0; j<ny+1; j++)
//        {
//            for (int k=0; k<nz+1; k++)
//            {
//                int IDMKL1 = ID_MKL(0,j,k);
//                int IDMKL2 = ID_MKL(nx,j,k);
//                Omega[0][IDMKL2] = Omega[0][IDMKL2];
//                Omega[1][IDMKL2] = Omega[1][IDMKL2];
//                Omega[2][IDMKL2] = Omega[2][IDMKL2];
//            }
//        }
//    }
}
#endif

void Eul_Grid_3D::Increment_Psi_Node(const Cart_ID &CID, const Vector3 &Src)
{
    int IDL = ID_MKL(CID);
    Psi[0][IDL] = Src(0);
    Psi[1][IDL] = Src(1);
    Psi[2][IDL] = Src(2);
}

//--- Projection vorticity free

void Eul_Grid_3D::Calc_Omega_Div()
{
    // This calculates the divergence of vorticity on the grid.

    memset(Psi[0],    0, N_Points*sizeof(Real));        // Clear array 1
    Div_Om_Max = 0;

    // Loop over particles
    for (int i=0; i<Active_Node_ID.size(); i++)
    {
        int IDMKL = ID_MKL(Active_Node_ID[i]);
        Real DivOm =    D_DX(Active_Node_ID[i],Omega[0]) +  \
                        D_DY(Active_Node_ID[i],Omega[1]) +  \
                        D_DZ(Active_Node_ID[i],Omega[2]);

        // Negative is taken here as the Poisson solver actually solve del psi = - omega...
        Psi[0][IDMKL] = -DivOm;

        if (fabs(DivOm)>Div_Om_Max) Div_Om_Max = fabs(DivOm);
    }

//    StdVector S; S.assign(Psi[0],Psi[0]+N_Points);
//    std::vector<Real>::iterator MaxEl = std::max_element(S.begin(), S.end(), abs_compare);
//    int El =  std::distance(S.begin(), MaxEl);
//    return MaxDiv;
}

void Eul_Grid_3D::Project_Omega()
{
    // This carries out the process of calculating the divergence o f Omega on the grid.
    // This will allow us to project the vorticity back onto a divergence free basis.

    // Specify the diverge of the vorticity field.

    // Solve Poisson (Only necessary in a single dimension)
//    Solve_Poisson(0);

    // Augment Omega

//    for (int i=0; i<Active_Node_ID.size(); i++)
//    {
//        int IDMKL = ID_MKL(Active_Node_ID[i]);
//        Omega[0][IDMKL] -= D_DX(Active_Node_ID[i],Psi[0]);
//        Omega[1][IDMKL] -= D_DY(Active_Node_ID[i],Psi[0]);
//        Omega[2][IDMKL] -= D_DZ(Active_Node_ID[i],Psi[0]);
//    }

    // Specify modification on grid

    // Modify Omega
    memset(Omega[0], 0, N_Points*sizeof(Real));
    memset(Omega[1], 0, N_Points*sizeof(Real));
    memset(Omega[2], 0, N_Points*sizeof(Real));

    for (int i=0; i<Active_Node_ID.size(); i++)
    {
        int IDMKL = ID_MKL(Active_Node_ID[i]);
        Omega[0][IDMKL] = D_DX(Active_Node_ID[i],Psi[0]);
        Omega[1][IDMKL] = D_DY(Active_Node_ID[i],Psi[0]);
        Omega[2][IDMKL] = D_DZ(Active_Node_ID[i],Psi[0]);
    }

    // Reset Psi once more
//    memset(Psi[0], 0, N_Points*sizeof(Real));
}

//--- Activity

void    Eul_Grid_3D::Set_Cell_Active(const Cart_ID &CID)
{
    // This is called so often, that I implement a version here for easier readability
    Cart_ID CIDL = CID_Loc(CID);
    uint IDCell = ID_CELL(CIDL);
    if (Active_Cells[IDCell]) return;
    Active_Cells[IDCell] = true;
    Active_Cells_ID.push_back(CIDL);
}

void    Eul_Grid_3D::Mark_Surr_Node_Activity(const Cart_ID &CID)
{
    // We take the given node and ensure the nodes around it are marked as active
    for (int i=-1; i<=1; i++)
    {
        int CIDX = CID(0)+i;
        if (CIDX<0 || CIDX>nx) continue;
        for (int j=-1; j<=1; j++)
        {
            int CIDY = CID(1)+j;
            if (CIDY<0 || CIDY>ny) continue;
            for (int k=-1; k<=1; k++)
            {
                int CIDZ = CID(2)+k;
                if (CIDZ<0 || CIDZ>nz) continue;
                int IDMKL = ID_MKL(CIDX,CIDY,CIDZ);
                if (!Active_Nodes[IDMKL])
                {
                    Active_Nodes[IDMKL]=true;
                    Active_Node_ID.push_back(Cart_ID(CIDX,CIDY,CIDZ));
                }
            }
        }
    }
}

void    Eul_Grid_3D::Set_Direct_Node_Activity()
{
    // Only used for remeshing
    for (int i=0; i<Active_Cells_ID.size(); i++) Set_Node_Active(Active_Cells_ID[i](0),Active_Cells_ID[i](1),Active_Cells_ID[i](2));
}

void    Eul_Grid_3D::Set_Node_Activity(const GRID_MAP &MAP)
{
    // This specifies node activity based on cell activity

    //    qDebug() << ID_Super(0) << ID_Super(1) << ID_Super(2) << "Eul_Grid::Set_Node_Activity. N Active Cells = " << Active_Cells_ID.size();

    switch (MAP)
    {
        case M0:
        {
            for (int i=0; i<Active_Cells_ID.size(); i++)
            {
                Cart_ID CID = Active_Cells_ID[i];
                Set_Node_Active(CID(0)  ,CID(1)  ,CID(2)  );
            }
            break;
        }
        case M2:
        {
            for (int i=0; i<Active_Cells_ID.size(); i++)
            {
                Cart_ID CID = Active_Cells_ID[i];
                Set_Node_Active(CID(0)  ,CID(1)  ,CID(2)  );
                Set_Node_Active(CID(0)  ,CID(1)  ,CID(2)+1);
                Set_Node_Active(CID(0)  ,CID(1)+1,CID(2)  );
                Set_Node_Active(CID(0)  ,CID(1)+1,CID(2)+1);
                Set_Node_Active(CID(0)+1,CID(1)  ,CID(2)  );
                Set_Node_Active(CID(0)+1,CID(1)  ,CID(2)+1);
                Set_Node_Active(CID(0)+1,CID(1)+1,CID(2)  );
                Set_Node_Active(CID(0)+1,CID(1)+1,CID(2)+1);
            }
            break;
        }
        case M4D:
        {
            for (int i=0; i<Active_Cells_ID.size(); i++)
            {
                Cart_ID CID = Active_Cells_ID[i];
                Set_Node_Active(CID(0)-1,CID(1)-1,CID(2)-1);
                Set_Node_Active(CID(0)-1,CID(1)-1,CID(2)  );
                Set_Node_Active(CID(0)-1,CID(1)-1,CID(2)+1);
                Set_Node_Active(CID(0)-1,CID(1)-1,CID(2)+2);
                Set_Node_Active(CID(0)-1,CID(1)  ,CID(2)-1);
                Set_Node_Active(CID(0)-1,CID(1)  ,CID(2)  );
                Set_Node_Active(CID(0)-1,CID(1)  ,CID(2)+1);
                Set_Node_Active(CID(0)-1,CID(1)  ,CID(2)+2);
                Set_Node_Active(CID(0)-1,CID(1)+1,CID(2)-1);
                Set_Node_Active(CID(0)-1,CID(1)+1,CID(2)  );
                Set_Node_Active(CID(0)-1,CID(1)+1,CID(2)+1);
                Set_Node_Active(CID(0)-1,CID(1)+1,CID(2)+2);
                Set_Node_Active(CID(0)-1,CID(1)+2,CID(2)-1);
                Set_Node_Active(CID(0)-1,CID(1)+2,CID(2)  );
                Set_Node_Active(CID(0)-1,CID(1)+2,CID(2)+1);
                Set_Node_Active(CID(0)-1,CID(1)+2,CID(2)+2);

                Set_Node_Active(CID(0)  ,CID(1)-1,CID(2)-1);
                Set_Node_Active(CID(0)  ,CID(1)-1,CID(2)  );
                Set_Node_Active(CID(0)  ,CID(1)-1,CID(2)+1);
                Set_Node_Active(CID(0)  ,CID(1)-1,CID(2)+2);
                Set_Node_Active(CID(0)  ,CID(1)  ,CID(2)-1);
                Set_Node_Active(CID(0)  ,CID(1)  ,CID(2)  );
                Set_Node_Active(CID(0)  ,CID(1)  ,CID(2)+1);
                Set_Node_Active(CID(0)  ,CID(1)  ,CID(2)+2);
                Set_Node_Active(CID(0)  ,CID(1)+1,CID(2)-1);
                Set_Node_Active(CID(0)  ,CID(1)+1,CID(2)  );
                Set_Node_Active(CID(0)  ,CID(1)+1,CID(2)+1);
                Set_Node_Active(CID(0)  ,CID(1)+1,CID(2)+2);
                Set_Node_Active(CID(0)  ,CID(1)+2,CID(2)-1);
                Set_Node_Active(CID(0)  ,CID(1)+2,CID(2)  );
                Set_Node_Active(CID(0)  ,CID(1)+2,CID(2)+1);
                Set_Node_Active(CID(0)  ,CID(1)+2,CID(2)+2);

                Set_Node_Active(CID(0)+1,CID(1)-1,CID(2)-1);
                Set_Node_Active(CID(0)+1,CID(1)-1,CID(2)  );
                Set_Node_Active(CID(0)+1,CID(1)-1,CID(2)+1);
                Set_Node_Active(CID(0)+1,CID(1)-1,CID(2)+2);
                Set_Node_Active(CID(0)+1,CID(1)  ,CID(2)-1);
                Set_Node_Active(CID(0)+1,CID(1)  ,CID(2)  );
                Set_Node_Active(CID(0)+1,CID(1)  ,CID(2)+1);
                Set_Node_Active(CID(0)+1,CID(1)  ,CID(2)+2);
                Set_Node_Active(CID(0)+1,CID(1)+1,CID(2)-1);
                Set_Node_Active(CID(0)+1,CID(1)+1,CID(2)  );
                Set_Node_Active(CID(0)+1,CID(1)+1,CID(2)+1);
                Set_Node_Active(CID(0)+1,CID(1)+1,CID(2)+2);
                Set_Node_Active(CID(0)+1,CID(1)+2,CID(2)-1);
                Set_Node_Active(CID(0)+1,CID(1)+2,CID(2)  );
                Set_Node_Active(CID(0)+1,CID(1)+2,CID(2)+1);
                Set_Node_Active(CID(0)+1,CID(1)+2,CID(2)+2);

                Set_Node_Active(CID(0)+2,CID(1)-1,CID(2)-1);
                Set_Node_Active(CID(0)+2,CID(1)-1,CID(2)  );
                Set_Node_Active(CID(0)+2,CID(1)-1,CID(2)+1);
                Set_Node_Active(CID(0)+2,CID(1)-1,CID(2)+2);
                Set_Node_Active(CID(0)+2,CID(1)  ,CID(2)-1);
                Set_Node_Active(CID(0)+2,CID(1)  ,CID(2)  );
                Set_Node_Active(CID(0)+2,CID(1)  ,CID(2)+1);
                Set_Node_Active(CID(0)+2,CID(1)  ,CID(2)+2);
                Set_Node_Active(CID(0)+2,CID(1)+1,CID(2)-1);
                Set_Node_Active(CID(0)+2,CID(1)+1,CID(2)  );
                Set_Node_Active(CID(0)+2,CID(1)+1,CID(2)+1);
                Set_Node_Active(CID(0)+2,CID(1)+1,CID(2)+2);
                Set_Node_Active(CID(0)+2,CID(1)+2,CID(2)-1);
                Set_Node_Active(CID(0)+2,CID(1)+2,CID(2)  );
                Set_Node_Active(CID(0)+2,CID(1)+2,CID(2)+1);
                Set_Node_Active(CID(0)+2,CID(1)+2,CID(2)+2);
            }
            break;
        }
        default: break;
    }
}

void    Eul_Grid_3D::Set_Node_Active(const int &IDX,const int &IDY,const int &IDZ)
{
    // This is called so often, I implement a version here for easier readability
    int IDMKL = ID_MKL(IDX,IDY,IDZ);
    if (Active_Nodes[IDMKL]) return;
    Active_Nodes[IDMKL] = true;
    Active_Node_ID.push_back(Cart_ID(IDX,IDY,IDZ));
}

//---Post processing

void Eul_Grid_3D::Calc_Finite_Differences()
{
    // Depending on the analysis desired, we loop over the active nodes here and specify
    // where the derivates are to be calculated.

    // Note: Active Omega Nodes have been specified during transfer of the vorticity to the grid.
    // We shall here specify the active nodes by looping over the active vorticity nodes.
    // Everywhere Omega was specified we require Velocity, this is handled automatically by the U node variable
    // In order to calculate velocity derivatives on the grid we need to expand the activity by one in each direction
    // for the fin difference scheme. We shall therefore add the active omega nodes to the active nodes grid and
    // then expand by one in each direction-> We will then calculate the U and U gradient there.
    if (!OutputArrays)  Allocate_Output_Arrays();
    Last_PA_Timestep = Vars->Timestep;              // Set this timestep. It will be used later for deallocation

    Set_Node_Activity(Grid_Vars->Mapping);

    int NAct = Active_Node_ID.size();
    for (int i=0; i<NAct; i++) Mark_Surr_Node_Activity(Active_Node_ID[i]);      // Necessary to increase stencil footprint to resolve velocities

    for (int i=0; i<Active_Node_ID.size(); i++) Set_U_Node(Active_Node_ID[i]);
    for (int i=0; i<Active_Node_ID.size(); i++) Set_Stretching_Node(Active_Node_ID[i]);
    for (int i=0; i<Active_Node_ID.size(); i++) Set_Diffusion_Node(Active_Node_ID[i]);

    if (Vars->Turb_Model==SMG)  Calc_Turb_Stress_SGM();
    if (Vars->Turb_Model==RVM)  Calc_Turb_Stress_RVM();

    // Set arrays to be active (for clearing later)
    Set_U_Active();
    Set_DADt_Active();
    if (Vars->Turb_Model==SMG)
    {
        Set_LapOmega_Active();
    }
    if (Vars->Turb_Model==RVM)
    {
        Set_Omega_SS_Active();
        Set_Nu_SGS_Active();
    }
    // Now convert rate of change of of vorticity to rate of change of circulation (*H3)
    // Multiply by char. volume

    for (int i=0; i<Active_Node_ID.size(); i++)
    {
        Cart_ID CID = Active_Node_ID[i];
        uint IDMKL = ID_MKL(CID(0),CID(1),CID(2));

        DAlphaDt[0][IDMKL] *= H3;
        DAlphaDt[1][IDMKL] *= H3;
        DAlphaDt[2][IDMKL] *= H3;
    }
}

void Eul_Grid_3D::Set_U_Node(const Cart_ID &CID)
{
    // First order fin diff

    Vector3 dPsidX = D_DX(CID,Psi);
    Vector3 dPsidY = D_DY(CID,Psi);
    Vector3 dPsidZ = D_DZ(CID,Psi);

    uint IDC = ID_MKL(CID(0),CID(1),CID(2));
    U[0][IDC] = dPsidY(2)-dPsidZ(1);
    U[1][IDC] = dPsidZ(0)-dPsidX(2);
    U[2][IDC] = dPsidX(1)-dPsidY(0);
}

void Eul_Grid_3D::Set_Stretching_Node(const Cart_ID &CID)
{
    // This sets the stretching component of the node
    // We shall calculate the stretching component as described by Cocle 2008

    uint IDC = ID_MKL(CID(0),CID(1),CID(2));

    // Specify the necessary U values.
    // Remember Omega stores -Omega!

    // Conservative form

//    DAlphaDt[0][IDC] = (D_DX(CID,U[0],Omega[0]) + D_DY(CID,U[1],Omega[0]) + D_DZ(CID,U[2],Omega[0]));
//    DAlphaDt[1][IDC] = (D_DX(CID,U[0],Omega[1]) + D_DY(CID,U[1],Omega[1]) + D_DZ(CID,U[2],Omega[1]));
//    DAlphaDt[2][IDC] = (D_DX(CID,U[0],Omega[2]) + D_DY(CID,U[1],Omega[2]) + D_DZ(CID,U[2],Omega[2]));

    // Non conservative form (appears to work much better?)

    Matrix3 NablaU;
    NablaU.row(0) = D_DX(CID,U);
    NablaU.row(1) = D_DY(CID,U);
    NablaU.row(2) = D_DZ(CID,U);

//    NablaU(0,0) = DUDX[0][IDC];
//    NablaU(0,1) = DUDX[1][IDC];
//    NablaU(0,2) = DUDX[2][IDC];

//    NablaU(1,0) = DUDY[0][IDC];
//    NablaU(1,1) = DUDY[1][IDC];
//    NablaU(1,2) = DUDY[2][IDC];

//    NablaU(2,0) = DUDZ[0][IDC];
//    NablaU(2,1) = DUDZ[1][IDC];
//    NablaU(2,2) = DUDZ[2][IDC];

    Vector3 S = NablaU*Comp_Vec(CID, Omega);

    DAlphaDt[0][IDC] = S(0);
    DAlphaDt[1][IDC] = S(1);
    DAlphaDt[2][IDC] = S(2);

    // Non conservative isotropic
//    Matrix3 NablaU;
//    bool BDRY = false;
//    if (CID(0)<1 || CID(0)>nx-1) BDRY=true;
//    if (CID(1)<1 || CID(1)>ny-1) BDRY=true;
//    if (CID(2)<1 || CID(2)>nz-1) BDRY=true;
//    if (BDRY)
//    {
//        NablaU.row(0) = D_DX(CID,U);
//        NablaU.row(1) = D_DY(CID,U);
//        NablaU.row(2) = D_DZ(CID,U);
//    }
//    else
//    {
//        // Isotropic stencil
//        int I = CID(0);
//        int J = CID(1);
//        int K = CID(2);

//        // X-Y-Z
//        Vector3 DDX =  16.0/36*(Comp_Vec(I+1,J  ,K  ,U) - Comp_Vec(I-1,J  ,K  ,U))
//                      + 4.0/36*(Comp_Vec(I+1,J  ,K+1,U) + Comp_Vec(I+1,J  ,K-1,U) +  Comp_Vec(I+1,J+1,K  ,U) + Comp_Vec(I+1,J-1,K  ,U))
//                      - 4.0/36*(Comp_Vec(I-1,J  ,K+1,U) + Comp_Vec(I-1,J  ,K-1,U) +  Comp_Vec(I-1,J+1,K  ,U) + Comp_Vec(I-1,J-1,K  ,U))
//                      + 1.0/36*(Comp_Vec(I+1,J+1,K+1,U) + Comp_Vec(I+1,J-1,K+1,U) +  Comp_Vec(I+1,J+1,K-1,U) + Comp_Vec(I+1,J-1,K-1,U))
//                      - 1.0/36*(Comp_Vec(I-1,J+1,K+1,U) + Comp_Vec(I-1,J-1,K+1,U) +  Comp_Vec(I-1,J+1,K-1,U) + Comp_Vec(I-1,J-1,K-1,U));

//        // Switch Y and X
//        Vector3 DDY =  16.0/36*(Comp_Vec(I  ,J+1,K  ,U) - Comp_Vec(I  ,J-1,K  ,U))
//                      + 4.0/36*(Comp_Vec(I  ,J+1,K+1,U) + Comp_Vec(I  ,J+1,K-1,U) +  Comp_Vec(I+1,J+1,K  ,U) + Comp_Vec(I-1,J+1,K  ,U))
//                      - 4.0/36*(Comp_Vec(I  ,J-1,K+1,U) + Comp_Vec(I  ,J-1,K-1,U) +  Comp_Vec(I+1,J-1,K  ,U) + Comp_Vec(I-1,J-1,K  ,U))
//                      + 1.0/36*(Comp_Vec(I+1,J+1,K+1,U) + Comp_Vec(I-1,J+1,K+1,U) +  Comp_Vec(I+1,J+1,K-1,U) + Comp_Vec(I-1,J+1,K-1,U))
//                      - 1.0/36*(Comp_Vec(I+1,J-1,K+1,U) + Comp_Vec(I-1,J-1,K+1,U) +  Comp_Vec(I+1,J-1,K-1,U) + Comp_Vec(I-1,J-1,K-1,U));

//        // Switch Z and X
//        Vector3 DDZ =  16.0/36*(Comp_Vec(I  ,J  ,K+1,U) - Comp_Vec(I  ,J  ,K-1,U))
//                      + 4.0/36*(Comp_Vec(I+1,J  ,K+1,U) + Comp_Vec(I-1,J  ,K+1,U) +  Comp_Vec(I  ,J+1,K+1,U) + Comp_Vec(I  ,J-1,K+1,U))
//                      - 4.0/36*(Comp_Vec(I+1,J  ,K-1,U) + Comp_Vec(I-1,J  ,K-1,U) +  Comp_Vec(I  ,J+1,K-1,U) + Comp_Vec(I  ,J-1,K-1,U))
//                      + 1.0/36*(Comp_Vec(I+1,J+1,K+1,U) + Comp_Vec(I+1,J-1,K+1,U) +  Comp_Vec(I-1,J+1,K+1,U) + Comp_Vec(I-1,J-1,K+1,U))
//                      - 1.0/36*(Comp_Vec(I+1,J+1,K-1,U) + Comp_Vec(I+1,J-1,K-1,U) +  Comp_Vec(I-1,J+1,K-1,U) + Comp_Vec(I-1,J-1,K-1,U));

//        NablaU.col(0) = DDX;
//        NablaU.col(1) = DDY;
//        NablaU.col(2) = DDZ;
//        NablaU *= 0.5*InvH;
//    }

//    Vector3 S = Comp_Vec(Cart_ID(CID(0),CID(1),CID(2)), Omega).transpose()*NablaU.transpose();
//    Vector3 S = Comp_Vec(CID, Omega).transpose()*NablaU.transpose();

//    Vector3 S = NablaU*Comp_Vec(CID, Omega);

//    // Add effect of stretching
//    DAlphaDt[0][IDC] += S(0);
//    DAlphaDt[1][IDC] += S(1);
//    DAlphaDt[2][IDC] += S(2);

    // Hack to check validity of solver by comparing omega to nabla cross u
    // This is a hack which changes the output....
//    if (Vars->Check_Omega)
//    {
////        qDebug() << Omega[0][IDC] << NablaU(1,2)-NablaU(2,1);

//        Omega[0][IDC] -= NablaU(1,2)-NablaU(2,1);
//        Omega[1][IDC] -= NablaU(2,0)-NablaU(0,2);
//        Omega[2][IDC] -= NablaU(0,1)-NablaU(1,0);
//    }

    // Calculate subgrid viscosity

    if (Vars->Turb_Model == RVM)
    {
        Matrix3 Sij = 0.5*(NablaU + NablaU.transpose());
        Nu_SGS[IDC] = Vars->C_n*H2*sqrt(2.0*Sij.squaredNorm());
    }
}

void Eul_Grid_3D::Set_Diffusion_Node(const Cart_ID &CID)
{
    // Set the value of the Laplacian for this node and stores in the laplacian array.

    // NOTE: This can be accelerated by using a stencil with equivalnet error, however with
    // lower node count. See:
    // Katrunnen, M., Stencils with isotropic discretization error for differential operators

    uint IDMKL = ID_MKL(CID(0),CID(1),CID(2));

    Vector3 L = Vector3::Zero();

    if (CID(0)==0 || CID(1)==0 || CID(2)==0  || CID(0)==nx || CID(1)==ny || CID(2)==nz)     // One sided!
    {
        L(0) = D2_DX2(CID,Omega[0]);
        L(1) = D2_DY2(CID,Omega[1]);
        L(2) = D2_DZ2(CID,Omega[2]);
    }
    else                                                                // Isotropic!
    {
        L += LAP_ISO_3D[0]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)-1), Omega);
        L += LAP_ISO_3D[1]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)  ), Omega);
        L += LAP_ISO_3D[2]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)+1), Omega);
        L += LAP_ISO_3D[3]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)-1), Omega);
        L += LAP_ISO_3D[4]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)  ), Omega);
        L += LAP_ISO_3D[5]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)+1), Omega);
        L += LAP_ISO_3D[6]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)-1), Omega);
        L += LAP_ISO_3D[7]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)  ), Omega);
        L += LAP_ISO_3D[8]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)+1), Omega);

        L += LAP_ISO_3D[9 ]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)-1), Omega);
        L += LAP_ISO_3D[10]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)  ), Omega);
        L += LAP_ISO_3D[11]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)+1), Omega);
        L += LAP_ISO_3D[12]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)-1), Omega);
        L += LAP_ISO_3D[13]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)  ), Omega);
        L += LAP_ISO_3D[14]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)+1), Omega);
        L += LAP_ISO_3D[15]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)-1), Omega);
        L += LAP_ISO_3D[16]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)  ), Omega);
        L += LAP_ISO_3D[17]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)+1), Omega);

        L += LAP_ISO_3D[18]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)-1), Omega);
        L += LAP_ISO_3D[19]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)  ), Omega);
        L += LAP_ISO_3D[20]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)+1), Omega);
        L += LAP_ISO_3D[21]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)-1), Omega);
        L += LAP_ISO_3D[22]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)  ), Omega);
        L += LAP_ISO_3D[23]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)+1), Omega);
        L += LAP_ISO_3D[24]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)-1), Omega);
        L += LAP_ISO_3D[25]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)  ), Omega);
        L += LAP_ISO_3D[26]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)+1), Omega);

        L *= InvH2;        // Finite difference
    }

    // Add effect of viscosity
    DAlphaDt[0][IDMKL] += Vars->Kin_Visc*L(0);
    DAlphaDt[1][IDMKL] += Vars->Kin_Visc*L(1);
    DAlphaDt[2][IDMKL] += Vars->Kin_Visc*L(2);
//    DAlphaDt[0][IDMKL]  = L(0);
//    DAlphaDt[1][IDMKL] = L(1);
//    DAlphaDt[2][IDMKL] = L(2);

    if (Vars->Turb_Model==SMG)
    {
        LapOmega[0][IDMKL] = L(0);
        LapOmega[1][IDMKL] = L(1);
        LapOmega[2][IDMKL] = L(2);
    }
}

void Eul_Grid_3D::Calc_Turb_Stress_SGM()
{
    // Set the value of the Laplacian for this node and stores in the laplacian array.

    // NOTE: This can be accelerated by using a stencil with equivalnet error, however with
    // lower node count. See:
    // Katrunnen, M., Stencils with isotropic discretization error for differential operators

    for (int i=0; i<Active_Node_ID.size(); i++)
    {
        Cart_ID CID = Active_Node_ID[i];
        uint IDMKL = ID_MKL(CID(0),CID(1),CID(2));
        Vector3 BiLap = Vector3::Zero();

        if (CID(0)==0 || CID(1)==0 || CID(2)==0  || CID(0)==nx || CID(1)==ny || CID(2)==nz)     // One sided!
        {
            //        uint IDMKL = ID_MKL(CID(0),CID(1),CID(2));
            BiLap(0) = D2_DX2(CID,LapOmega[0]);
            BiLap(1) = D2_DY2(CID,LapOmega[1]);
            BiLap(2) = D2_DZ2(CID,LapOmega[2]);
        }
        else                                                                // Isotropic!
        {
            BiLap += LAP_ISO_3D[0]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[1]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[2]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)+1), LapOmega);
            BiLap += LAP_ISO_3D[3]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[4]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[5]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)+1), LapOmega);
            BiLap += LAP_ISO_3D[6]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[7]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[8]*Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)+1), LapOmega);

            BiLap += LAP_ISO_3D[9 ]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[10]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[11]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)+1), LapOmega);
            BiLap += LAP_ISO_3D[12]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[13]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[14]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)+1), LapOmega);
            BiLap += LAP_ISO_3D[15]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[16]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[17]*Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)+1), LapOmega);

            BiLap += LAP_ISO_3D[18]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[19]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[20]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)+1), LapOmega);
            BiLap += LAP_ISO_3D[21]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[22]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[23]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)+1), LapOmega);
            BiLap += LAP_ISO_3D[24]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)-1), LapOmega);
            BiLap += LAP_ISO_3D[25]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)  ), LapOmega);
            BiLap += LAP_ISO_3D[26]*Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)+1), LapOmega);

            BiLap *= InvH2;        // Finite difference
        }

        // Add effect of hyperviscosity
        DAlphaDt[0][IDMKL] -= Vars->SMG_Fac*BiLap(0);
        DAlphaDt[1][IDMKL] -= Vars->SMG_Fac*BiLap(1);
        DAlphaDt[2][IDMKL] -= Vars->SMG_Fac*BiLap(2);
    }
}

void Eul_Grid_3D::Calc_Turb_Stress_RVM()
{
    //--- This calculates the contribution due to the RVM model

    //--- Calculate filtered field.

    // Order 1;
    if (Vars->rvm_n==1)
    {
//        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], Omega);

        // Carry out single sweeps over the arrays
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 3, Omega, Omega_SS2);        // Z sweep
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 2, Omega_SS2, Omega_SS);     // Y sweep
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 1, Omega_SS, Omega_SS2);     // X sweep
    }

    // Order 2:
    if (Vars->rvm_n==2)
    {
        // We require a larger Omega footprint if using a higher order small-scale filtering

        //-- Expand activity

        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 3, Omega, Omega_SS);         // Z sweep
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 3, Omega_SS, Omega_SS2);     // Z sweep
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 2, Omega_SS2, Omega_SS);     // Y sweep
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 2, Omega_SS, Omega_SS2);     // Y sweep
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 1, Omega_SS2, Omega_SS);     // X sweep
        for (int i=0; i<Active_Node_ID.size(); i++) SG_Disc_Filter_Node(Active_Node_ID[i], 1, Omega_SS, Omega_SS2);     // X sweep
        return;
    }

    //--- Now store as small-scale vorticity
    for (int i=0; i<Active_Node_ID.size(); i++)
    {
        Cart_ID CID = Active_Node_ID[i];
        uint IDC = ID_MKL(CID(0),CID(1),CID(2));
        Omega_SS[0][IDC] = Omega[0][IDC] - Omega_SS2[0][IDC];
        Omega_SS[1][IDC] = Omega[1][IDC] - Omega_SS2[1][IDC];
        Omega_SS[2][IDC] = Omega[2][IDC] - Omega_SS2[2][IDC];
    }

    // Now carry out Laplacian on this field

    for (int i=0; i<Active_Node_ID.size(); i++)
    {
        Cart_ID CID = Active_Node_ID[i];
        uint IDC = ID_MKL(CID(0),CID(1),CID(2));
        Real VSGS[27];
        Real VSGSC = Nu_SGS[IDC];
        VSGS[0] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)-1,CID(2)-1)] + VSGSC);
        VSGS[1] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)-1,CID(2)  )] + VSGSC);
        VSGS[2] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)-1,CID(2)+1)] + VSGSC);
        VSGS[3] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)  ,CID(2)-1)] + VSGSC);
        VSGS[4] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )] + VSGSC);
        VSGS[5] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)  ,CID(2)+1)] + VSGSC);
        VSGS[6] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)+1,CID(2)-1)] + VSGSC);
        VSGS[7] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)+1,CID(2)  )] + VSGSC);
        VSGS[8] = 0.5*(Nu_SGS[ID_MKL(CID(0)-1,CID(1)+1,CID(2)+1)] + VSGSC);

        VSGS[9 ] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)-1,CID(2)-1)] + VSGSC);
        VSGS[10] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )] + VSGSC);
        VSGS[11] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)-1,CID(2)+1)] + VSGSC);
        VSGS[12] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)] + VSGSC);
        VSGS[13] = 0;
        VSGS[14] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)] + VSGSC);
        VSGS[15] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)+1,CID(2)-1)] + VSGSC);
        VSGS[16] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )] + VSGSC);
        VSGS[17] = 0.5*(Nu_SGS[ID_MKL(CID(0)  ,CID(1)+1,CID(2)+1)] + VSGSC);

        VSGS[18] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)-1,CID(2)-1)] + VSGSC);
        VSGS[19] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)-1,CID(2)  )] + VSGSC);
        VSGS[20] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)-1,CID(2)+1)] + VSGSC);
        VSGS[21] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)  ,CID(2)-1)] + VSGSC);
        VSGS[22] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )] + VSGSC);
        VSGS[23] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)  ,CID(2)+1)] + VSGSC);
        VSGS[24] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)+1,CID(2)-1)] + VSGSC);
        VSGS[25] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)+1,CID(2)  )] + VSGSC);
        VSGS[26] = 0.5*(Nu_SGS[ID_MKL(CID(0)+1,CID(1)+1,CID(2)+1)] + VSGSC);

        Vector3 OSSC = Comp_Vec(CID, Omega_SS);
        Vector3 L = Vector3::Zero();
        L += LAP_ISO_3D[0]*VSGS[0]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[1]*VSGS[1]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[2]*VSGS[2]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)-1,CID(2)+1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[3]*VSGS[3]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[4]*VSGS[4]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[5]*VSGS[5]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)  ,CID(2)+1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[6]*VSGS[6]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[7]*VSGS[7]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[8]*VSGS[8]*(Comp_Vec(Cart_ID(CID(0)-1,CID(1)+1,CID(2)+1), Omega_SS) - OSSC);

        L += LAP_ISO_3D[9 ]*VSGS[9]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[10]*VSGS[10]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[11]*VSGS[11]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)-1,CID(2)+1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[12]*VSGS[12]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)-1), Omega_SS) - OSSC);
//        L += LAP_ISO_3D[13]*VSGS[]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[14]*VSGS[14]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)  ,CID(2)+1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[15]*VSGS[15]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[16]*VSGS[16]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[17]*VSGS[17]*(Comp_Vec(Cart_ID(CID(0)  ,CID(1)+1,CID(2)+1), Omega_SS) - OSSC);

        L += LAP_ISO_3D[18]*VSGS[18]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[19]*VSGS[19]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[20]*VSGS[20]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)-1,CID(2)+1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[21]*VSGS[21]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[22]*VSGS[22]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[23]*VSGS[23]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)  ,CID(2)+1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[24]*VSGS[24]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)-1), Omega_SS) - OSSC);
        L += LAP_ISO_3D[25]*VSGS[25]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)  ), Omega_SS) - OSSC);
        L += LAP_ISO_3D[26]*VSGS[26]*(Comp_Vec(Cart_ID(CID(0)+1,CID(1)+1,CID(2)+1), Omega_SS) - OSSC);

        L *= InvH2;        // Finite difference

        // Add effect due to turbulence
        DAlphaDt[0][IDC] += L(0);
        DAlphaDt[1][IDC] += L(1);
        DAlphaDt[2][IDC] += L(2);
    }
}

void    Eul_Grid_3D::SG_Disc_Filter_Node(const Cart_ID &CID, const int &D, Real **I, Real **O)
{
    // This carries out the sug grid discrete filter described in Jeanmart 2007. The filter is set up to complete in
    // each direction, as the filtering process is carried out one dimension at a time. The

    uint IDMKL = ID_MKL(CID(0),CID(1),CID(2));

    switch (D)
    {
        case 1:
        {
            O[0][IDMKL] = I[0][IDMKL] + H2*0.25*D2_DX2(CID, I[0]);
            O[1][IDMKL] = I[1][IDMKL] + H2*0.25*D2_DX2(CID, I[1]);
            O[2][IDMKL] = I[2][IDMKL] + H2*0.25*D2_DX2(CID, I[2]);
            break;
        }
        case 2:
        {
            O[0][IDMKL] = I[0][IDMKL] + H2*0.25*D2_DY2(CID, I[0]);
            O[1][IDMKL] = I[1][IDMKL] + H2*0.25*D2_DY2(CID, I[1]);
            O[2][IDMKL] = I[2][IDMKL] + H2*0.25*D2_DY2(CID, I[2]);
            break;
        }
        case 3:
        {
            O[0][IDMKL] = I[0][IDMKL] + H2*0.25*D2_DZ2(CID, I[0]);
            O[1][IDMKL] = I[1][IDMKL] + H2*0.25*D2_DZ2(CID, I[1]);
            O[2][IDMKL] = I[2][IDMKL] + H2*0.25*D2_DZ2(CID, I[2]);
            break;
        }
        default: break;
    }
}

Real    Eul_Grid_3D::D_DX(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(0)==0)          return      InvH*(  -1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +2.0*A[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]
                                                -0.5*A[ID_MKL(CID(0)+2,CID(1)  ,CID(2)  )]);
    else if (CID(0)==nx)    return      InvH*(   0.5*A[ID_MKL(CID(0)-2,CID(1)  ,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]
                                                +1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]);
    else                    return 0.5* InvH*(       A[ID_MKL(CID(0)+1,CID(1)  ,CID(2))]
                                                -    A[ID_MKL(CID(0)-1,CID(1)  ,CID(2))]);
}

Real    Eul_Grid_3D::D_DX_2(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(0)==0)          return      InvH*(  -11.0/6.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +3.0*       A[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]
                                                -3.0/2.0*   A[ID_MKL(CID(0)+2,CID(1)  ,CID(2)  )]
                                                +1.0/3.0*   A[ID_MKL(CID(0)+3,CID(1)  ,CID(2)  )]);
    else if (CID(0)==nx)    return      InvH*(  +11.0/6.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -3.0*       A[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]
                                                +3.0/2.0*   A[ID_MKL(CID(0)-2,CID(1)  ,CID(2)  )]
                                                -1.0/3.0*   A[ID_MKL(CID(0)-3,CID(1)  ,CID(2)  )]);
}

Real    Eul_Grid_3D::D_DX_3(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(0)==0)          return      InvH*(  -25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +4.0*       A[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]
                                                -3.0*       A[ID_MKL(CID(0)+2,CID(1)  ,CID(2)  )]
                                                +4.0/3.0*   A[ID_MKL(CID(0)+3,CID(1)  ,CID(2)  )]
                                                -1.0/4.0*   A[ID_MKL(CID(0)+4,CID(1)  ,CID(2)  )]);
    else if (CID(0)==nx)    return      InvH*(  +25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -4.0*       A[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]
                                                +3.0*       A[ID_MKL(CID(0)-2,CID(1)  ,CID(2)  )]
                                                -4.0/3.0*   A[ID_MKL(CID(0)-3,CID(1)  ,CID(2)  )]
                                                +1.0/4.0*   A[ID_MKL(CID(0)-4,CID(1)  ,CID(2)  )]);
}

Real    Eul_Grid_3D::D_DX_4(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(0)==0)          return      InvH*( -137.0/60.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                               +5.0*       A[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]
                                               -5.0*       A[ID_MKL(CID(0)+2,CID(1)  ,CID(2)  )]
                                               +10.0/3.0*  A[ID_MKL(CID(0)+3,CID(1)  ,CID(2)  )]
                                               -5.0/4.0*   A[ID_MKL(CID(0)+4,CID(1)  ,CID(2)  )]
                                               +1.0/5.0*   A[ID_MKL(CID(0)+5,CID(1)  ,CID(2)  )]);
    else if (CID(0)==nx)    return      InvH*( +137.0/60.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                               -5.0*       A[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]
                                               +5.0*       A[ID_MKL(CID(0)-2,CID(1)  ,CID(2)  )]
                                               -10.0/3.0*  A[ID_MKL(CID(0)-3,CID(1)  ,CID(2)  )]
                                               +5.0/4.0*   A[ID_MKL(CID(0)-4,CID(1)  ,CID(2)  )]
                                               -1.0/5.0*   A[ID_MKL(CID(0)-5,CID(1)  ,CID(2)  )]);
}

Real    Eul_Grid_3D::D_DX(const Cart_ID &CID, Real *A, Real *B)
{
    if (CID(0)==0)          return      InvH*(  -1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +2.0*A[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]
                                                -0.5*A[ID_MKL(CID(0)+2,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)+2,CID(1)  ,CID(2)  )]);
    else if (CID(0)==nx)    return      InvH*(   0.5*A[ID_MKL(CID(0)-2,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)-2,CID(1)  ,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]
                                                +1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]);
    else                    return 0.5* InvH*(  A[     ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)+1,CID(1)  ,CID(2))]
                                               -A[     ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)-1,CID(1)  ,CID(2))]);
}

Vector3 Eul_Grid_3D::D_DX(const Cart_ID &CID, Real **A) {return Vector3(D_DX(CID,A[0]),D_DX(CID,A[1]),D_DX(CID,A[2]));}

Real    Eul_Grid_3D::D_DY(const Cart_ID &CID, Real *A)
{
    if (CID(1)==0)          return      InvH*(  -1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +2.0*A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]
                                                -0.5*A[ID_MKL(CID(0)  ,CID(1)+2,CID(2)  )]);
    else if (CID(1)==ny)    return      InvH*(   0.5*A[ID_MKL(CID(0)  ,CID(1)-2,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]
                                                +1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]);
    else                    return  0.5*InvH*(       A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]
                                                    -A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]);
}

Real    Eul_Grid_3D::D_DY_2(const Cart_ID &CID, Real *A)
{
    if (CID(1)==0)          return      InvH*(  -11.0/6.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +3.0*       A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]
                                                -3.0/2.0*   A[ID_MKL(CID(0)  ,CID(1)+2,CID(2)  )]
                                                +1.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)+3,CID(2)  )]);
    else if (CID(1)==ny)    return      InvH*(  +11.0/6.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -3.0*       A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]
                                                +3.0/2.0*   A[ID_MKL(CID(0)  ,CID(1)-2,CID(2)  )]
                                                -1.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)-3,CID(2)  )]);
}

Real    Eul_Grid_3D::D_DY_3(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(1)==0)          return      InvH*(  -25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +4.0*       A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]
                                                -3.0*       A[ID_MKL(CID(0)  ,CID(1)+2,CID(2)  )]
                                                +4.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)+3,CID(2)  )]
                                                -1.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)+4,CID(2)  )]);
    else if (CID(1)==ny)    return      InvH*(  +25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -4.0*       A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]
                                                +3.0*       A[ID_MKL(CID(0)  ,CID(1)-2,CID(2)  )]
                                                -4.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)-3,CID(2)  )]
                                                +1.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)-4,CID(2)  )]);
}

Real    Eul_Grid_3D::D_DY_4(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(1)==0)          return      InvH*( -137.0/60.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                               +5.0*       A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]
                                               -5.0*       A[ID_MKL(CID(0)  ,CID(1)+2,CID(2)  )]
                                               +10.0/3.0*  A[ID_MKL(CID(0)  ,CID(1)+3,CID(2)  )]
                                               -5.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)+4,CID(2)  )]
                                               +1.0/5.0*   A[ID_MKL(CID(0)  ,CID(1)+5,CID(2)  )]);
    else if (CID(1)==ny)    return      InvH*( +137.0/60.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                               -5.0*       A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]
                                               +5.0*       A[ID_MKL(CID(0)  ,CID(1)-2,CID(2)  )]
                                               -10.0/3.0*  A[ID_MKL(CID(0)  ,CID(1)-3,CID(2)  )]
                                               +5.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)-4,CID(2)  )]
                                               -1.0/5.0*   A[ID_MKL(CID(0)  ,CID(1)-5,CID(2)  )]);
}

Real    Eul_Grid_3D::D_DY(const Cart_ID &CID, Real *A, Real *B)
{
    if (CID(1)==0)          return      InvH*(  -1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +2.0*A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]
                                                -0.5*A[ID_MKL(CID(0)  ,CID(1)+2,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)+2,CID(2)  )]);
    else if (CID(1)==ny)    return      InvH*(   0.5*A[ID_MKL(CID(0)  ,CID(1)-2,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)-2,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]
                                                +1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]);
    else                    return 0.5* InvH*(       A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]*B[ID_MKL(CID(0),  CID(1)+1,CID(2)  )]
                                                    -A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]);
}

Vector3 Eul_Grid_3D::D_DY(const Cart_ID &CID, Real **A) {return Vector3(D_DY(CID,A[0]),D_DY(CID,A[1]),D_DY(CID,A[2]));}

Real    Eul_Grid_3D::D_DZ(const Cart_ID &CID, Real *A)
{
    if (CID(2)==0)          return      InvH*(  -1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]
                                                -0.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+2)]);
    else if (CID(2)==nz)    return      InvH*(   0.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-2)]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]
                                                +1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]);
    else                    return  0.5*InvH*(       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]
                                                    -A[ID_MKL(CID(0),  CID(1)  ,CID(2)-1)]);
}

Real    Eul_Grid_3D::D_DZ_2(const Cart_ID &CID, Real *A)
{
    if (CID(2)==0)          return      InvH*(  -11.0/6.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +3.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]
                                                -3.0/2.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+2)]
                                                +1.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+3)]);
    else if (CID(2)==nz)    return      InvH*(  +11.0/6.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -3.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]
                                                +3.0/2.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-2)]
                                                -1.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-3)]);
}

Real    Eul_Grid_3D::D_DZ_3(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(2)==0)          return      InvH*(  -25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +4.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]
                                                -3.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+2)]
                                                +4.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+3)]
                                                -1.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+4)]);
    else if (CID(2)==nz)    return      InvH*(  +25.0/12.0* A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -4.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]
                                                +3.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-2)]
                                                -4.0/3.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-3)]
                                                +1.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-4)]);
}

Real    Eul_Grid_3D::D_DZ_4(const Cart_ID &CID, Real *A)
{
    // Ensure that a finite difference with error proportional to H^2 is being calculated
    if (CID(2)==0)          return      InvH*( -137.0/60.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                               +5.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]
                                               -5.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+2)]
                                               +10.0/3.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+3)]
                                               -5.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+4)]
                                               +1.0/5.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+5)]);
    else if (CID(2)==nz)    return      InvH*( +137.0/60.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                               -5.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]
                                               +5.0*       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-2)]
                                               -10.0/3.0*  A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-3)]
                                               +5.0/4.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-4)]
                                               -1.0/5.0*   A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-5)]);
}

Real    Eul_Grid_3D::D_DZ(const Cart_ID &CID, Real *A, Real *B)
{
    if (CID(2)==0)          return      InvH*(  -1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]
                                                -0.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+2)]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+2)]);
    else if (CID(2)==nz)    return      InvH*(   0.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-2)]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-2)]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]
                                                +1.5*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]);
    else                    return 0.5* InvH*(       A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]*B[ID_MKL(CID(0),  CID(1)  ,CID(2)+1)]
                                                    -A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]*B[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]);
}

Vector3 Eul_Grid_3D::D_DZ(const Cart_ID &CID, Real **A) {return Vector3(D_DZ(CID,A[0]),D_DZ(CID,A[1]),D_DZ(CID,A[2]));}

Real    Eul_Grid_3D::D2_DX2(const Cart_ID &CID, Real *A)
{
    if (CID(0)==0)          return      InvH2*( +1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]
                                                +1.0*A[ID_MKL(CID(0)+2,CID(1)  ,CID(2)  )]   );
    else if (CID(0)==nx)    return      InvH2*( +1.0*A[ID_MKL(CID(0)-2,CID(1)  ,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]
                                                +1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]   );
    else                    return      InvH2*( +1.0*A[ID_MKL(CID(0)-1,CID(1)  ,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +1.0*A[ID_MKL(CID(0)+1,CID(1)  ,CID(2)  )]   );
}

Real    Eul_Grid_3D::D2_DY2(const Cart_ID &CID, Real *A)
{
    if (CID(1)==0)          return      InvH2*(  1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]
                                                +1.0*A[ID_MKL(CID(0)  ,CID(1)+2,CID(2)  )]   );
    else if (CID(1)==ny)    return      InvH2*(  1.0*A[ID_MKL(CID(0)  ,CID(1)-2,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]
                                                +1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]   );
    else                    return      InvH2*(  1.0*A[ID_MKL(CID(0)  ,CID(1)-1,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +1.0*A[ID_MKL(CID(0)  ,CID(1)+1,CID(2)  )]   );
}

Real    Eul_Grid_3D::D2_DZ2(const Cart_ID &CID, Real *A)
{
    if (CID(2)==0)          return      InvH2*(  1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]
                                                +1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+2)]   );
    else if (CID(2)==nz)    return      InvH2*(  1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-2)]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]
                                                +1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]   );
    else                    return      InvH2*(  1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)-1)]
                                                -2.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)  )]
                                                +1.0*A[ID_MKL(CID(0)  ,CID(1)  ,CID(2)+1)]   );
}

//--- Extraction

void    Eul_Grid_3D::Extract_Grid(Real **Grid, const Cart_ID &C, const Cart_ID &W, StateVector &M)
{
    // We need to superpose the values of Psi on the grids to ensure the solution
    // is continuous. For this we extract a grid with the given dimensions.

    // Note that the value of Psi here is the value still in memory from the Psi_0 (free space solution)
    // solve. This is transferred to the Psi value of the sub grid temporarily for specification of the
    // correct boundary condition during the next solve step.

    int X0 = C(0);
    int Y0 = C(1);
    int Z0 = C(2);

    int WX = W(0);
    int WY = W(1);
    int WZ = W(2);

    M.assign(WX*WY*WZ,Vector3::Zero());

    for (int i=0; i<WX; i++)
    {
        for (int j=0; j<WY; j++)
        {
            for (int k=0; k<WZ; k++)
            {
                int IDM = i*WY*WZ + j*WZ + k;
                int IDMKL = ID_MKL(X0+i,Y0+j,Z0+k);

                M[IDM](0) = Grid[0][IDMKL];
                M[IDM](1) = Grid[1][IDMKL];
                M[IDM](2) = Grid[2][IDMKL];
            }
        }
    }
}

void    Eul_Grid_3D::Superpose_Grid(Real **Grid, const Cart_ID &C, const Cart_ID &W, StateVector &M)
{
    // This is the complimentary step to that shown above. The extracted values of Psi0
    // are transferred over and temporarily stored in the output array before they are called during
    // specification of the boundary condition (in function Set_BC_JL()).

    int X0 = C(0);
    int Y0 = C(1);
    int Z0 = C(2);

    int WX = W(0);
    int WY = W(1);
    int WZ = W(2);

    for (int i=0; i<WX; i++)
    {
        for (int j=0; j<WY; j++)
        {
            for (int k=0; k<WZ; k++)
            {
                int IDM = i*WY*WZ + j*WZ + k;
                int IDMKL = ID_MKL(X0+i,Y0+j,Z0+k);
                Grid[0][IDMKL] += M[IDM](0);
                Grid[1][IDMKL] += M[IDM](1);
                Grid[2][IDMKL] += M[IDM](2);
            }
        }
    }

}

void    Eul_Grid_3D::Nullify_Array(const Cart_ID &C, const Cart_ID &W, Real **A)
{
    // This function is called during remeshing. We deliberately set certain section to have zero omega
    // to avoid overlap

    int X0 = C(0);
    int Y0 = C(1);
    int Z0 = C(2);

    int WX = W(0);
    int WY = W(1);
    int WZ = W(2);

    for (int i=0; i<WX; i++)
    {
        for (int j=0; j<WY; j++)
        {
            for (int k=0; k<WZ; k++)
            {
                int IDMKL = ID_MKL(X0+i,Y0+j,Z0+k);
                A[0][IDMKL] = 0.0;
                A[1][IDMKL] = 0.0;
                A[2][IDMKL] = 0.0;
            }
        }
    }
}

Vector3 Eul_Grid_3D::Comp_Vec(const Cart_ID &CID, Real **Array)
{
    // Extracts the array vector for this CID
    uint IDMKL = ID_MKL(CID(0),CID(1),CID(2));
    return Vector3(Array[0][IDMKL],Array[1][IDMKL],Array[2][IDMKL]);
}

//--- Clean up

void Eul_Grid_3D::Reset_Arrays()
{
    // This is a general function which checks if an arrays has been used, and clears if necessary
    // This clear all activity data also

    //--- Reset BCs
    if (BC_Active)
    {
        Reset_Array3D(BD_AX,N_FaceX);
        Reset_Array3D(BD_BX,N_FaceX);
        Reset_Array3D(BD_AY,N_FaceY);
        Reset_Array3D(BD_BY,N_FaceY);
        Reset_Array3D(BD_AZ,N_FaceZ);
        Reset_Array3D(BD_BZ,N_FaceZ);
        BC_Active = false;
    }

    //--- Reset Input arrays
    if (Omega_Active)       {Reset_Array3D(Omega,N_Points);         Omega_Active = false;       }

    //--- Reset Output arrays
    if (Psi_Active)         {Reset_Array3D(Psi,N_Points);           Psi_Active = false;         }
    if (U_Active)           {Reset_Array3D(U,N_Points);             U_Active = false;           }
    if (DAlphaDt_Active)    {Reset_Array3D(DAlphaDt,N_Points);      DAlphaDt_Active = false;    }
    if (LapOmega_Active)    {Reset_Array3D(LapOmega,N_Points);      LapOmega_Active = false;    }

    if (Vars->Turb_Model==RVM)
    {
        if (Nu_SGS_Active)      {Reset_Array(Nu_SGS,N_Points);      Nu_SGS_Active = false;      }
        if (Omega_SS_Active)    {Reset_Array3D(Omega_SS,N_Points);  Reset_Array3D(Omega_SS2,N_Points); Omega_SS_Active = false;    }
    }

    //--- Reset node activity
    if (!Active_Node_ID.empty())
    {
        memset(Active_Nodes, false, N_Points*sizeof(bool));
        Active_Node_ID.clear();
    }

    if (!Active_Cells_ID.empty())
    {
        memset(Active_Cells, false,         N_Cells*sizeof(bool));
        memset(Active_Volume_Cells, false,  N_Cells*sizeof(bool));
        Active_Cells_ID.clear();
    }
}

//--------------------------------------------------------------
//----------------------3D Neumann------------------------------
//--------------------------------------------------------------

//--- Post processing

void  Eul_Grid_3D_N::Calc_Finite_Differences()
{
    // The velocity at each position is calculated... as Omega = 0,
    // we can tranfer the results directly

    if (!OutputArrays)  Allocate_Output_Arrays();

    for (int i=0; i<nx+1; i++)
    {
        for (int j=0; j<ny+1; j++)
        {
            for (int k=0; k<nz+1; k++)
            {
                Cart_ID CID(i,j,k);
                int IDMKL = ID_MKL(i, j, k);
                U[0][IDMKL] = D_DX(CID,Psi[0]);
                U[1][IDMKL] = D_DY(CID,Psi[0]);
                U[2][IDMKL] = D_DZ(CID,Psi[0]);
//                qDebug() << U[0][IDMKL] << U[1][IDMKL]<< U[2][IDMKL];
            }
        }
    }

    U_Active = true;
}

void Eul_Grid_3D_N::Allocate_Output_Arrays()
{
    // This function is sepcified as depending on the method of solution,
    // we may not need to allocate output arrays at all. For the Poisson JL algorithm,
    // we only define the values of Psi and Omega over the volume for the primary grid (free space solution)
    // The sub grid however requires storage of the output arrays for the Finite diff calcs.

    U = new Real*[3];
    for (int i=0; i<3; i++) U[i] = (Real*)malloc(N_Points*sizeof(Real));
    Reset_Array3D(U,N_Points);             U_Active = false;

//    DAlphaDt = new Real*[3];
//    for (int i=0; i<3; i++) DAlphaDt[i] = (Real*)malloc(N_Points*sizeof(Real));
//    Reset_Array3D(DAlphaDt,N_Points);       DAlphaDt_Active = false;

    OutputArrays = true;
}

////--- Eul_Grid_3D_N Poisson Test

//Grid_Vars->EGrid = NXNYNZ;
//Grid_Vars->N_ML = 16;
//Cart_ID CIDLow = Cart_ID::Zero();
//Cart_ID CIDN; CIDN  << 6,6,6;
//Cart_ID CIDUp; CIDUp << 5,5,5;
//Create_Monolithic_Poisson_Domain(CIDLow,CIDUp,CIDN);

//StateVector BC;
//int n = Grid_Vars->N_ML*6;
//int nw = (n+1)*(n+1);
//BC.assign(6*nw,Vector3::Zero());
//int n1 = 5;
//int n2 = 20;
//for (int i=n1; i<n2+1; i++)
//{
//    for (int j=n1; j<n2+1; j++)
//    {
//        BC[4*nw+    i*(n+1)         +j](0) = -1.0;
//        BC[5*nw+    (n+1-i)*(n+1)   +(n+1-j)](0) = 1.0;
//    }
//}

//EGrid_Mono->Set_BC(BC);
//EGrid_Mono->Solve_Poisson(0);
//Update_Scene();
}
