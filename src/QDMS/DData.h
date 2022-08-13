/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

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

#ifndef DDATA_H
#define DDATA_H


#include <QList>
#include <QColor>
#include "../QBEM/Blade.h"
#include "../QBEM/Polar360.h"
#include "../Graph/ShowAsGraphInterface.h"


class DData : public ShowAsGraphInterface
{
    friend class QDMS;
    friend class MainFrame;
    friend class DMSData;

public:
    virtual NewCurve* newCurve (QString /*xAxis*/, QString /*yAxis*/, NewGraph::GraphType /*graphType*/) { return NULL; }
    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType, int heightIndex);

    QStringList getAvailableVariables (NewGraph::GraphType graphType);
    QString getObjectName () { return QString(m_DMSName + " " + m_windspeedString); }
	bool isDrawPoints ();
	bool isDrawCurve ();
    void initializeOutputVectors();

    double elements;
    double epsilon;
    double iterations;
    double relax;
    double rho;
    double visc;
    bool bPowerLaw;
    bool bConstant;
    bool bLogarithmic;
    double exponent;
    double roughness;

    double Toff;
    double windspeed;
    QString m_windspeedString;
    bool m_bTipLoss;
    bool m_bVariable;
    bool m_bAspectRatio;

	DData(QString dmsName);
    virtual ~DData();

	void serialize();
	static DData* newBySerialize();
	void Init(CBlade *pWing, double lambda, double pitch);
    void OnDMS(CBlade *pBlade);

//private:
    QString m_WingName;
    QString m_DMSName;
    QString m_lambdaString;

    double sweptArea;
    double lambda_global;
    double max_radius;
    double max_r_pos;
    double max_r_count;
    double positions;
    int m_sections; //number of sections
    double h_top, h_bottom, height;
    double delta;
    double cp, cp1, cp2;//power coefficient
    double cm, cm1, cm2;// torque coefficient
    double w;// rotational speed
    double power, torque, thrust;//turbine power output and torque
    double ct, ct1, ct2; // thrust coefficient
    double blades;
    bool m_bIsInverted;

    QVector <float> deltas;

    // one value per height position
    QVector <float> m_pos;           //local height
    QVector <float> m_zeta;          //local relative height
    QVector <float> m_c_local;       //local chord
    QVector <float> m_t_local;       //local thickness
    QVector <float> m_delta;         //local inclination angle
    QVector <float> m_radius_local;  //local radius
    QVector <float> m_theta_local;   //local azi angle
    QVector <float> m_eta;           //local relative radius
    QVector <float> m_lambda_up;     //local upwind tip speed ratio
    QVector <float> m_lambda_down;   //local downwind tip speed ratio
    QVector <float> m_velocity_inf;  //local inflow velocity
    QVector <float> m_velocity_up;   //local upwind induced velocity
    QVector <float> m_velocity_equil;//local equilibrium-induced velocity
    QVector <float> m_velocity_down; //local downwind induced velocity
    QVector <float> m_velocity_wake; //local wake velocity
    QVector <float> m_u_up;          //local (average) upwind interference factor
    QVector <float> m_u_down;        //local (average) downwind induction factor
    QVector <float> m_a_up;          //local upwind induction factor
    QVector <float> m_a_down;        //local downwind interference factor
    QVector <float> m_area;          //projection area for swept area
    QVector <float> m_Ftip_up;       //local upwind tiploss factor
    QVector <float> m_Ftip_dw;       //local downwind tiploss factor
    QVector <float> m_it_up;         //iterations for convergence upwind
    QVector <float> m_it_dw;         //iterations for convergence downwind
    QVector <float> m_twist;         //twist angle

    // one value per azimuthal position
    QVector <float> m_theta;         //azimuthal angle
    QVector <float> m_theta_deg;     //azimuthal angle in degrees
    QVector <float> m_theta_plot;    //azimuthal angle in degrees +90deg
    QVector <float> m_alpha_eq;      //equatorial local angle of attack
    QVector <float> m_Re_eq;         //equatorial local Reynolds Number
    QVector <float> m_vrel_eq;       //equatorial local relative velocity
    QVector <float> m_CD_eq;         //equatorial drag coeff
    QVector <float> m_CL_eq;         //equatorial drag coeff
    QVector <float> m_LD_eq;         //equatorial lift to drag coeff
    QVector <float> m_Cn_eq;         //equatorial normal force coefficient
    QVector <float> m_Ct_eq;         //equatorial tangential force coefficient
    QVector <float> m_CFN_eq;        //blade element normal force coefficient
    QVector <float> m_CFT_eq;        //blade element tangential force coefficient
    QVector <float> m_CF_length;     //lengthwise force coefficient for one blade
    QVector <float> m_CF_cross;      //crosswise force coefficient for one blade
    QVector <float> m_CF_length_tot; //lengthwise force coefficient for all blades
    QVector <float> m_CF_cross_tot;  //crosswise force coefficient for all blades
    QVector <float> m_FN;            //normal force distribution, one blade
    QVector <float> m_FT;            //tangential force distribution, one blade
    QVector <float> m_T;             //torque distribution, one blade
    QVector <float> m_F_length_tot;	 //lengthwise force districution, one blade
    QVector <float> m_F_cross_tot;	 //crosswise force districution, one blade
    QVector <float> m_F_length;		 //lengthwise force districution, all blades
    QVector <float> m_F_cross;		 //crosswise force districution, all blades
    QVector <float> m_FN_tot;        //total normal force distribution, all blades
    QVector <float> m_FT_tot;        //total tangential force distribution, all blades
    QVector <float> m_T_tot;         //total torque distribution, all blades

    // azimuthal arrays per height position
    QVector<QVector<float> > m_iterations;  //local angle of attack
    QVector<QVector<float> > m_alpha;       //local angle of attack
    QVector<QVector<float> > m_alpha_deg;   //local angle of attack in degrees
    QVector<QVector<float> > m_Ftip;        //local angle of attack in degrees
    QVector<QVector<float> > m_u;           //local interference factor
    QVector<QVector<float> > m_V;           //local induced velocity
    QVector<QVector<float> > m_Re;          //local Reynolds Number
    QVector<QVector<float> > m_DeltaRe;     //delta Reynolds Number
    QVector<QVector<float> > m_vrel;        //local relative velocity
    QVector<QVector<float> > m_CD;          //drag coeff
    QVector<QVector<float> > m_CM;          //moment coeff
    QVector<QVector<float> > m_CL;          //lift coeff
    QVector<QVector<float> > m_LD;          //lift to drag ratio
    QVector<QVector<float> > m_Cn;          //normal force coefficient
    QVector<QVector<float> > m_Ct;          //tangential force coefficient

    bool m_bBackflow;

    QStringList m_availableBladeVariables, m_availableAziVariables;


private:
	virtual QPen doGetPen (int curveIndex, int highlightedIndex, bool forTheDot);
};

#endif // DDATA_H
