/**********************************************************************

    Copyright (C) 2020 Joseph Saverin <joseph.saverin@qblade.org>

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

#ifndef VPML_GLOBAL_VARS_H
#define VPML_GLOBAL_VARS_H

#include "Math_Types.h"
#include "Debug.h"

namespace VPML {

enum DIMENSION      {SIM_2D, SIM_3D};
enum SOL_METHOD     {GREEN, POISSON, POISSON_JL, POISSON_MONO, POISSON_UNB, POISSON_CYL};            // Which method of solution are we using?
enum SPATIAL_INT    {NOINT, DIRECT, DIRECT_OCL, MULTILEVEL};                   // How are we carrying out the spatial integration?
enum REMESHING      {FULL, CIRC_FILT, STRETCH_FILT};                    // How is remeshing being carried out?
enum GRID_MAP       {M0, M2, M4D};                                      // How are particles being meshed to the grid?
enum TEMPORAL_INT   {NOTIMEINT, EF, RK2, RK3, RK4, AB2, AB2LF};         // What is the timestepping scheme being used?
enum REG            {   SINGULAR, LOA_2D, HOA_2D, GAUSS2_2D,
                        LOA_3D, HOA_3D, GAUSS_3D, SUPGAUSS_3D, SININT_3D};   // Which regularisation are we using?
enum STRETCH        {CLASSIC, TRANSPOSE, MIXED};
enum TURB_MODEL     {NOTURB, SMG, RVM};                         // Which turbulence model is being used?
enum MLKERNEL       {BS2D, PSI2D, BS3D, PSI3D};
enum OUTVAR         {VEL, PSI, OMEGA, NUSGS};
enum BINNING        {TREE, BLOCK, GPU};
enum EUL_GRID       {DXDY, PXPY, DXDYDZ, PXDYDZ, NXNYNZ};         // What type of Eulerian grids are we generating?
enum Plane          {XPlane, YPlane, ZPlane};

//enum GRIDPROC       {GRID_CPU, GRID_GPU};

enum Status_Type
{
    kSuccess,               // No problems
    kPoissonInsuff,         // The choice of polynomial approximation for the ML method is too low. Must be greater than 1.
    kPoissonOverlap,        // Use of the James-Lackner algorithm requires that the Omeag Grid is larger than the Psi grid. OL_OG is too low.
    kPoissonJLOverlapSmall  // OLOG too small
};

struct Sim_Variables
{
    //---Solver status---
    Status_Type Status;

    //--- Options
    SOL_METHOD Solver;                      // Remove!

    //---Sim vars---
    DIMENSION  Dim = SIM_3D;                 // Is the sim 2D or 3D?
    QString SIM_NAME;

    //---Environmental vars---
    Real Kin_Visc = 1.0;                        // Kinematic viscosity: Initialized to zero for stretch testing
    Vector3 Freestream = Vector3::Zero();       // Freestream velocity

    //--- Export vars
    int  Particle_Export_Freq = 0;              // How frequently are we exporting the grid for visualisations?
    int  VTK_Export_Freq = 0;                   // How frequently are we exporting vtk files?
    QString VTK_ID;
    QString PartOut_ID;

    //--- Remeshing variables
    int  Remesh_Freq = 0;                   // How frequently are we carrying out remeshing?
    bool Remesh_Init = false;
    REMESHING   Rmsh_Opt = FULL;            // Method for remeshing

    bool Set_Resized = true;
    bool Particle_Set_Imported = false;

    //--- Mag filtering variables
    int Mag_Filt_Freq = 0;
    Real Mag_Filt_Fac = 1e-3;               // What is the threshold?
    bool Spat_Mag_Filt = false;             // Is spatial magnitude filtering being carried out?
    Real Spat_Mag_Fac;                      // Is spatial magnitude filtering being carried out?

    //--- Position filtering variables
    int Pos_Filt_Freq = 0;

    //--- Dviergence filtering variables
    int  Div_Filtering_Freq = 0;
    Real MaxDiv = 1e-3;                     // Used to corrected divergence of Voriticity field

    //--- Modelling variables
    REG Smoothing = GAUSS_3D;
    STRETCH Stretching_Scheme = TRANSPOSE;  // What is the stretching scheme being used? (default: transpose)

    //---Turbulence modelling---
    TURB_MODEL Turb_Model = NOTURB;     // Which turbulence model are we using?
    Real C_smg;                         // Hyperviscosity coefficient
    Real T_smg;                         // Global time scale
    Real SMG_Fac;                       // Hyperviscosity factor
    int rvm_n = 2;                      // How many times shall we filter the field?
    Real C_n = 1.27*pow(0.3,3)*1.39;    // Smagorinski scale

    //---Problems with symmetry---
    bool PADDING = false;
    bool CONSTRAIN = false;
    bool SYMM_X = false;
    bool SYMM_Y = false;
    bool SYMM_Z = false;
    bool SYMM_AXIAL_BI = false;
    bool SYMM_AXIAL_TRI = false;

    //---Problems with periodicity (for now only applied in x-direction)---
    bool Periodic = false;      // Do we have a periodic simulation?
    int NB_Periodic = 0;        // This determines the number of periodic base boxes in X direction
    int N_Per_Mirr = 0;         // This specifies how many time we mirror the domain in X direction
    Real Lx_Per;                // Length in the x direction of the periodic domain

    //--- Binning option

    BINNING Grid_Option = TREE;

    //--- Method to avoid double calculation of source/probe interps
    bool Prbs_Are_Srcs = false;

    //--- Problems with a rotating wing
    Real Rot_Omega, TSR, RInner, ROuter;

    //----------Spatial integration method-------------
    MLKERNEL        ML_Kernel;
    OUTVAR          Output = VEL;

    //----------Timestepping variables-------------
    TEMPORAL_INT    Temp_Int = AB2LF;       // Default AB2LF for particle sims
    int Timestep;               // Timestep
    int Timestep_Init = 0;          // Timestep
    double dT;                  // Integration timestep
    double T;                   // Current time
    int  N_Max;                 // How many timesteps are we carrying out?
    Real T_Char;                // Char. time
    QTime Clock;                // Timing
    QElapsedTimer Wall_Clock;   // Timing

    //----------Particle generation variables-------------
    int Particle_Gen_Freq = 0;

    //----------Testing variables-------------
    Real Anal;                  // Analytical solution

    //----------Visualisation variables-------------
    bool Update_Vis = false;
};

// Struct for grid variables

struct Grid_Variables
{
    //--- Characteristic variables
    Real Vol_Char;                      // Characteristic volume / area
    Real Sigma_Char;                    // Characteristic coresize

    //--- Cartesian grid parameters
    Vector3 Origin = Vector3::Zero();   // Origin of grid CS
    Real    H_Grid;                     // Characteristic particle spacing
    GRID_MAP Mapping = M4D;
    GRID_MAP Remesh_Mapping = M4D;
    uint     Fin_Diff=1;                // Order of the finite difference method used?

    //--- Poisson grid parameters
    EUL_GRID EGrid = DXDYDZ;            // What type of Eulerian grid is being generated? Default 3D Eul Dirichlet
    int     ESX=0;                      // Supergrid length in X direction (not including overlap)
    int     ESY=0;                      // Supergrid length in Y direction (not including overlap)
    int     ESZ=0;                      // Supergrid length in Z direction (not including overlap)
    int     OL=0;                       // Grid overlap in all directions in the case that supergrid elements are cubic
    int     OL_OG=5;                    // Shifting factor for Omega Grid (where free space solution is calculated- JL)
    int     OL_ROG;

    int     OL_PG=2;                    // Shifting factor for Psi Grid (on which fin diffs are calculated)
    int     OL_RPG;                     // Shifting factor for Psi Grid (on which fin diffs are calculated)
    int     ES;

    int Block_Grid_Buffer = 1;          // In the case we are using a block grid, what is the buffer desired?

    //--- Multilevel parameters
    int     N_ML;                       // Multilevel base box size multiple
    Real    H_ML;                       // Characteristic Domain sidelength multilevel method
    uint    Poly_ML = 5;                // Polynomial order int multilevel method approximation
    uint    NF_Crit = 1;                // Nearfield critical length

    int N_IntFace = 1;                  // Number of interp per face Poisson JL
    int N_IntB = 1;                     // Number of interp nodes for boundary (Poisson JL)
    int N_Int = 1;                      // Number of interp nodes in each box (Interp nodes)
    int N_Rcv = 1;                      // Number of Receiver nodes in each box (Interp nodes*Octants)
    int N_Src = 1;                      // Number of Source nodes in a parent-near-field set
    int N_Oct = 1;                      // Number of child boxes per parent
    int N_NF = 1;                       // Number of boxes which belong in the near field of a parent object...

    //--- Multilevel global vars
    Matrix Weight_Template_1D;
    Matrix Node_Template_1D;
    Matrix Node_Template_2D;
    Matrix Node_Template_3D;

    SparseMatrix DDXS, DDYS, DDZS;  // Sparse mult mats

    MatrixRM Poisson_Boundary_Template;         // Templates poisson solver.
    MatrixRM PJL_WallSrc2Vol;                   // This interpolates the nodes onto the wall mesh.
    MatrixRM PMono_W2W;                         // This interpolates wall nodes to wall nodes

    //--- Anterp to ML-> Dense grids
    Matrix M2MLMap;                           // Map Eul grid Source to ML Grid;

    //--- Monolithic solver
    Vector3 Mono_Corner = Vector3::Zero();
    Cart_ID NBlMono = Cart_ID::Zero();
    Cart_ID NPMono = Cart_ID::Zero();
};

extern Sim_Variables    *Vars;
extern Grid_Variables   *Grid_Vars;
}

#endif // VPML_GLOBAL_VARS_H

