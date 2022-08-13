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

#ifndef VPML_TYPES_H
#define VPML_TYPES_H

#include <Eigen/Dense>        // Eigen dense data types
#include <Eigen/Sparse>       // Eigen sparse data types
#include <tr1/cmath>          // Special functions
#include <memory>             // Shared ptr.

#define OpenMPfor _Pragma("omp parallel for")

#define SinglePrec;

#ifdef SinglePrec

//--------Single Precision----------

typedef float                       Real;
typedef std::complex<float>         CReal;
typedef Eigen::MatrixXf             Matrix;
typedef Eigen::Matrix<float,\
                Eigen::Dynamic,\
                Eigen::Dynamic,\
                Eigen::RowMajor>    MatrixRM;
typedef Eigen::SparseMatrix<float>  SparseMatrix;
typedef Eigen::MatrixXcf            CMatrix;
typedef Eigen::Matrix3f             Matrix3;
typedef Eigen::VectorXf             Vector;
typedef Eigen::RowVectorXf          RowVector;
typedef Eigen::Matrix<float, 1, 1>  Vector1;
typedef Eigen::Matrix<float, 3, 1>  Vector3;
typedef Eigen::Matrix<float, 6, 1>  Vector6;
typedef Eigen::Matrix<float, 8, 1>  Vector8;
typedef Eigen::Quaternionf          Quat;
typedef Eigen::MatrixXf::Index      Matrix_ID;
typedef std::vector<float>          StdVector;

#endif

#ifdef DoublePrec

//--------Double Precision----------

typedef double                      Real;
typedef std::complex<double>        CReal;
typedef Eigen::MatrixXd             Matrix;
typedef Eigen::Matrix<double,\
                Eigen::Dynamic,\
                Eigen::Dynamic,\
                Eigen::RowMajor>    MatrixRM;
typedef Eigen::SparseMatrix<double> SparseMatrix;
typedef Eigen::MatrixXcd            CMatrix;
typedef Eigen::Matrix3d             Matrix3;
typedef Eigen::VectorXd             Vector;
typedef Eigen::RowVectorXd          RowVector;
typedef Eigen::Matrix<double, 1, 1> Vector1;
typedef Eigen::Matrix<double, 3, 1> Vector3;
typedef Eigen::Matrix<double, 6, 1> Vector6;
typedef Eigen::Matrix<double, 8, 1> Vector8;
typedef Eigen::Quaterniond          Quat;
typedef Eigen::MatrixXd::Index      Matrix_ID;
typedef std::vector<double>         StdVector;

#endif

//-------- Cartesian ID types

typedef Eigen::Vector2i             Cart_ID2;
typedef Eigen::Vector3i             Cart_ID;
typedef Eigen::Vector4i             Cart_ID4;

//typedef std::shared_ptr<PNode> SPNode;

// Thread-safe vector!!
//  https://codereview.stackexchange.com/questions/214927/implementation-of-thread-safe-vector
// https://software.intel.com/en-us/node/506203     Intel concurrent vector
// https://www.threadingbuildingblocks.org/docs/doxygen/a00046.html#details TBB intel thread

//-------- Additional std::vector variabels and capabilities

typedef std::vector<int>        IntVector;
typedef std::vector<Vector>     StateVector;
static  StateVector             Empty_SV;

template <class T>          // Hack to avoid using insert everytime
static void StdAppend(std::vector<T>& lhs, const std::vector<T>& rhs)       {lhs.insert(lhs.end(),rhs.begin(), rhs.end());}

template <class T>          // Hack to avoid using insert everytime
static void StdAppend(std::vector<T>& lhs, const std::vector<T>& rhs, const int &P1, const int &P2)     {lhs.insert(lhs.end(),rhs.begin()+P1, rhs.begin()+P2);}

template <class T>
static void StdClearAppend(std::vector<T>& lhs, const std::vector<T>& rhs)  {lhs.clear(); StdAppend(lhs,rhs);}

static Matrix Conv_SVec_to_Mat(const StateVector &V)
{
    int NR = V.size();
    int NC = V[0].size();
    Matrix M = Matrix::Zero(NR,NC);
    for (int i=0; i<NR; i++) M.row(i) = V[i];
    return M;
}

static StateVector Conv_Mat_to_SVec(const Matrix &M)
{
    int NR = M.rows();
    int NC = M.cols();
    StateVector S;
    S.assign(NR,Vector::Zero(NC));
    for (int i=0; i<NR; i++) S[i] = M.row(i);
    return S;
}

//---- Mathematical Constants

static Real const  EUL          =  0.5772156649;
static Real const  Sqrt2        =  sqrt(2.0);
static Real const  PI           =  3.14159265358979;
static Real const  PIinv        =  1.0/PI;
static Real const  TwoPI        =  2.0*PI;
static Real const  TwoPIinv     =  1.0/TwoPI;
static Real const  FourPI       =  4.0*PI;
static Real const  FourPIinv    =  1.0/FourPI;   //   0.079577471545948
static Real const  Rt2oPi       =  sqrt(2.0/PI);
static Real const  RtTwoPIinv   = sqrt(TwoPIinv);
static Real const  Inv2PISq     = TwoPIinv/PI;
static Real const  RPMtoOmega   = TwoPI/60.0;
static Real const  OmegatoRPM   = 1.0/RPMtoOmega;

//static Real const  FourPIinv    = 0.0795774715;
static Real const  D2R          =  PI/180;
static Real const  R2D          =  180/PI;

//---- Kinematic parameters

static Real const  P_atm          = 101325;
static Real const  Rho_wat        = 1025;
static Real const  Rho_air        = 1.225;
static Real const  Gravity        = 9.81;
static Real const  Kin_Visc_wat   = 1.004e-6;
static Real const  Kin_Visc_air   = 1.5571e-5;


//---- Vector Padding

inline Vector8 Vec8(const Vector3 &P, const Vector3 &A, const Real &S, const Real &V) {Vector8 Vout; Vout << P(0),P(1),P(2),A(0),A(1),A(2),S,V; return Vout;}

inline Vector6 Vec3toVec6(const Vector3 &V1, const Vector3 &V2) {Vector6 Vout; Vout << V1(0), V1(1), V1(2), V2(0), V2(1), V2(2); return Vout;}

#endif // VPML_TYPES_H
