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

#ifndef PARTICLE_OPERATIONS_H
#define PARTICLE_OPERATIONS_H

#include "Math_Types.h"
#include "Debug.h"

namespace VPML {

//------------------------------------------
//--------Individual particles
//------------------------------------------

//--------Spatial checks

inline bool X_LT_Zero(const Vector &V)      { return (V(0)<0.0); }
inline bool X_GT_Zero(const Vector &V)      { return (V(0)>0.0); }
//inline bool X_GT_PerDom(const Vector &V)    { return (V(0)>Vars->Lx_Per); }
inline bool Y_LT_Zero(const Vector &V)      { return (V(1)<0.0); }
inline bool Y_GT_Zero(const Vector &V)      { return (V(1)>0.0); }
inline bool Z_LT_Zero(const Vector &V)      { return (V(2)<0.0); }
inline bool Z_GT_Zero(const Vector &V)      { return (V(2)>0.0); }
inline bool TH_NEG(const Vector &V)         { return Z_GT_Zero(V);  }

//--------Spatial comparisons

inline bool XComp(const Vector &V1, const Vector &V2)   {return (V1(0)<=V2(0));    }
inline bool YComp(const Vector &V1, const Vector &V2)   {return (V1(1)<=V2(1));    }
inline bool ZComp(const Vector &V1, const Vector &V2)   {return (V1(2)<=V2(2));    }

//--------Spatial manipulations

inline void Rotate_Vector_X_180(Vector &V)    {V(1) *= -1.0;    V(2) *= -1.0;   V(4) *= -1.0;    V(5) *= -1.0;}
inline void Rotate_Vector_X(Vector &V, const Real &TH)
{
    Real CT = cos(TH), ST = sin(TH);
    Vector VH = V;
    V(1) = CT*VH(1) - ST*VH(2); V(2) = ST*VH(1) + CT*VH(2);
    V(4) = CT*VH(4) - ST*VH(5); V(5) = ST*VH(4) + CT*VH(5);
//    Real V(1) = CT*VH(1) + ST*VH(2);V(2) *= -1.0;   V(4) *= -1.0;    V(5) *= -1.0;
}
inline void Mirror_Vector_X(Vector &V)      {V(0) *= -1.0; V(4) *= -1.0; V(5) *= -1.0;}
inline void Mirror_Vector_Y(Vector &V)      {V(1) *= -1.0; V(3) *= -1.0; V(5) *= -1.0;}

//--------Extract data

inline Real PosMag(const Vector &V)                     {return Vector3(V(0),V(1),V(2)).norm();}
inline Real VortMag(const Vector &V)                    {return Vector3(V(3),V(4),V(5)).norm();}
inline Real GamMag(const Vector &V)                     {return V(7)*VortMag(V);            }
inline bool VortComp(const Vector &V1,const Vector &V2) {return (VortMag(V1)<=VortMag(V2));  }
inline bool VelComp(const Vector &V1,const Vector &V2)  {return (PosMag(V1)<=PosMag(V2));   }
inline bool GamComp(const Vector &V1, const Vector &V2) {return (GamMag(V1)<=GamMag(V2));    }

inline bool VortNull(const Vector &V)                   {return (Vector3(V(3),V(4),V(5)).norm()==0.0);}

//------ Diagnostics

inline Vector3 Pos(const Vector &V)     {return Vector3(V(0),V(1),V(2));}
inline Vector3 Gamma(const Vector &V)   {return Vector3(V(3),V(4),V(5))*V(7);}
inline Vector3 LinImp(const Vector &V)  {return Pos(V).cross(Gamma(V))/2.0;}
inline Vector3 AngImp(const Vector &V)  {return Pos(V).cross(Pos(V).cross(Gamma(V)))/3.0;}

//------------------------------------------
//--------Particle Lists
//------------------------------------------

//----- Statistics

inline Real MinX(const StateVector &S)         {return (*std::min_element(S.begin(), S.end(), XComp))(0);}
inline Real MaxX(const StateVector &S)         {return (*std::max_element(S.begin(), S.end(), XComp))(0);}
inline Real MinY(const StateVector &S)         {return (*std::min_element(S.begin(), S.end(), YComp))(1);}
inline Real MaxY(const StateVector &S)         {return (*std::max_element(S.begin(), S.end(), YComp))(1);}
inline Real MinZ(const StateVector &S)         {return (*std::min_element(S.begin(), S.end(), ZComp))(2);}
inline Real MaxZ(const StateVector &S)         {return (*std::max_element(S.begin(), S.end(), ZComp))(2);}

inline Real MaxVort(const StateVector &S)      {return VortMag(*std::max_element(S.begin(), S.end(), VortComp));}
inline Real MaxGamma(const StateVector &S)     {return GamMag(*std::max_element(S.begin(), S.end(), GamComp));}
inline Real MaxVel(const StateVector &S)       {return PosMag(*std::max_element(S.begin(), S.end(), VelComp));}

//------ Constraints

inline void Constrain_Periodic(StateVector &P, StateVector &PM1, StateVector &DPM1, const Real &LPer)
{
//    int Count = 0;
    if (!PM1.empty())
    {
    OpenMPfor
    for (int i=0; i<P.size(); i++)
    {
        if (P[i](0)<0)          PM1[i](0) += LPer;  // Shift previous position forward (AB2-AB2LF integration schemes)
        else if (P[i](0)>=LPer) PM1[i](0) -= LPer;  // Shift previous position back (AB2-AB2LF integration schemes)
    }
    }

    OpenMPfor
    for (int i=0; i<P.size(); i++)
    {
        if (P[i](0)<0)          P[i](0) += LPer;  // Shift previous position forward (AB2-AB2LF integration schemes)
        else if (P[i](0)>=LPer) P[i](0) -= LPer;  // Shift previous position back (AB2-AB2LF integration schemes)
    }
}

inline void Constrain_XSymm(StateVector &P, StateVector &PM1, StateVector &DPM1)
{
    // Enforces X symmetry constraint
    int NP = P.size();
    OpenMPfor
    for (int i=0; i<NP; i++)
    {
        if (X_GT_Zero(P[i]))     // Flip X
        {
            Mirror_Vector_X(P[i]);
            if (!PM1.empty())   Mirror_Vector_X(PM1[i]);
            if (!DPM1.empty())  Mirror_Vector_X(DPM1[i]);
        }
    }
}

inline void Constrain_YSymm(StateVector &P, StateVector &PM1, StateVector &DPM1)
{
    // Enforces Y symmetry constraint
    int NP = P.size();
    OpenMPfor
    for (int i=0; i<NP; i++)
    {
        if (Y_GT_Zero(P[i]))     // Flip X
        {
            Mirror_Vector_Y(P[i]);
            if (!PM1.empty()) Mirror_Vector_Y(PM1[i]);
            if (!DPM1.empty()) Mirror_Vector_Y(DPM1[i]);
        }
    }
}

//------ Magnitude filtering

inline Vector Part_Equiv(const StateVector &S, const Real &Sig, const Real &dV)
{
    // This is used for the magnitude filtering.
    // I calculate total circulation, linear impulse and angular impulse of the "leftover" elements
    // and replace them with a single element ensuring these three terms are conserved.

    Vector3 GamTot = Vector3::Zero();
    Vector3 ITot = Vector3::Zero();
    Vector3 ATot = Vector3::Zero();
    for (int i=0; i<S.size(); i++){
        GamTot += Gamma(S[i]);
        ITot += LinImp(S[i]);
        ATot += AngImp(S[i]);
    }

    // We have an over specified system. Need to solve for optimum distribution
    Vector3 Om = GamTot/dV;
    Matrix RHS = Matrix::Zero(6,1);
    RHS(0) = ITot(0)/dV;                    RHS(1) = ITot(1)/dV;                    RHS(2) = ITot(2)/dV;
    RHS(3) = ATot(0)/dV;                    RHS(4) = ATot(1)/dV;                    RHS(5) = ATot(2)/dV;

    Matrix M = Matrix::Zero(6,3);
    M(0,0) = 0;                             M(0,1) = -Om(2);                        M(0,2) = Om(1);
    M(1,0) = Om(2);                         M(1,1) = 0;                             M(1,2) = -Om(0);
    M(2,0) = -Om(1);                        M(2,1) = Om(0);                         M(2,2) = 0;

    M(3,0) = -(Om(1)*Om(1)+Om(2)*Om(2));    M(3,1) = Om(0)*Om(1);                   M(3,2) = Om(0)*Om(2);
    M(4,0) = Om(0)*Om(1);                   M(4,1) = -(Om(0)*Om(0)+Om(2)*Om(2));    M(4,2) = Om(1)*Om(2);
    M(5,0) = Om(0)*Om(2);                   M(5,1) = Om(1)*Om(2);                   M(5,2) = -(Om(0)*Om(0)+Om(1)*Om(1));

    // Solve using linear least squares
    Matrix Beta = M.transpose()*M;
    Matrix MPInv = (Beta.inverse())*(M.transpose());
    Matrix Sol = (Beta.inverse())*(M.transpose())*RHS;

    Vector8 V; V << Sol(0), Sol(1), Sol(2), Om(0), Om(1), Om(2), Sig, dV;

    // Check:
    Vector3 GO = Gamma(V);
    Vector3 LIO = LinImp(V);
    Vector3 AIO = AngImp(V);
    qDebug()    << "Gamma prev: " << GamTot(0) << GamTot(1) << GamTot(2)    << "Gamma post: "   << GO(0) << GO(1) << GO(2)
                << "Lin Imp prev: " << ITot(0) << ITot(1) << ITot(2)  << "Lin Imp post: " << LIO(0) << LIO(1) << LIO(2)
                << "Ang Imp prev: " << ATot(0) << ATot(1) << ATot(2)  << "Ang Imp post: " << AIO(0) << AIO(1) << AIO(2);

    return V;
}

//------ Grid operations

inline Cart_ID Get_CID(const Vector3 &Pos, const Real &H)
{
    // Returns the relative grid ID
    int IX = int(Pos(0)/H);
    int IY = int(Pos(1)/H);
    int IZ = int(Pos(2)/H);
    return Cart_ID(IX,IY,IZ);
}

//inline Cart_ID Get_CID(const Vector3 &Pos, const Vector3 &CPos, const Real &H)
//{
//    // Returns the relative grid ID

//    Vector3 PRel = Pos-CPos;
//    int IX = int(PRel(0)/H);
//    int IY = int(PRel(1)/H);
//    int IZ = int(PRel(2)/H);
//    return Cart_ID(IX,IY,IZ);
//}

}

#endif // PARTICLE_OPERATIONS_H
