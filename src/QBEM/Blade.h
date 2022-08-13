/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#ifndef CBLADE_H
#define CBLADE_H

#include <QAbstractTableModel>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QList>
#include "src/QBEM/AFC.h"
#include "src/QBEM/BDamage.h"
#include "src/QDMS/Strut.h"
#include "src/VortexObjects/VortexPanel.h"
#include "../StorableObject.h"
#include "../QBEM/BladeSurface.h"
#include "../Graph/ShowAsGraphInterface.h"

class Polar360;
class Airfoil;
class QDataStream;

class CBlade : public StorableObject, public ShowAsGraphInterface
{
public:

    struct DrawOptions {
        bool advancedEdit, drawSurfaces, drawAirfoilsFill, drawAirfoilsOutline, drawBladeOutline, drawAxes;
        double outlineWidth;
    };

    //the discretization is now a member variable of the simulation object instead of the blade, this allows for parallelization
    struct BladeDiscretization{
        QList<double> TThickness, TOffsetX, TPos, TOffsetZ, TChord, TCircAngle, TFoilPAxisX, TFoilPAxisZ, TTwist, TDihedral;
        QStringList TFoilNames;
        QList< QList <Vec3> > m_PanelPoints;
        QList<BladeSurface> m_PanelSurfaces;
    };

	int m_NPanel;
    int m_blades;
    int m_NSurfaces;
    bool m_bPlaceholder;
    bool m_bIsInverted;
    double m_PlanformSpan;
    double m_HubRadius;
    double m_sweptArea;
    double m_MaxRadius;

    bool m_bisSinglePolar;
    bool m_bisSymmetric;

	static CBlade* newBySerialize ();
	CBlade (QString name = "< no name >", StorableObject *parent = NULL);  // TODO remove the first default parameter asap!
	~CBlade ();
	void serialize ();  // override from StorableObject
    void restorePointers();
    void InitializeOutputVectors();

    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);
    QString getObjectName () { return m_objectName; }

	// NM new public interface
    double getBladeParameterFromCurvedLength(double position, double *data, bool isVawt, bool normHeight);
    double getRelativeStationLengthVAWT(int station, BladeDiscretization &bladeDisc);
    double getRelativeStationLengthHAWT(int station, BladeDiscretization &bladeDisc);
    double relativeLengthToHeight(double position);
	double getRotorRadius ();
    double getRootRadius();
    int getNumberOfBlades () { return m_blades; }
	int getNumberOfNodes ();
	double getAngle (int index);
	void setSegment (int index, double length, double angle);
	double getLength (int index);
	Polar360* get360PolarAt (int position);
	QList<Polar360*> getAll360Polars ();
	double getFASTRadiusAt (int position);
	void drawCoordinateAxes ();
	void drawFoilNames ();
	void drawFoilPositions ();
    void drawSelectionHighlight(int index);
	void drawBlade (DrawOptions options);
	void shiftPanelArrays (int index, bool forward);  // [index] will be ready to use after forward shift
	void fillPanelFromNeighbors (int index);
	void prepareGlDraw ();
    double getSkew();
	
	static QStringList prepareMissingObjectMessage(bool forDMS);
    static QStringList prepareMissingObjectMessage();
	
	Airfoil* GetFoilFromStation(int k);

    void Duplicate(CBlade *pWing, bool temporaryCopy, bool isVawt);
	
    // newly added stuff for flight sim integration, TODO - CLASS NEEDS CLEANUP
    void CreateWingSurfaces(bool createPanels, BladeDiscretization &bladeDisc);
    void CalculateSweptArea(bool isVawt);
    void GLCreateGeom(BladeDiscretization &bladeDisc, int List, bool selected, bool showSurf, bool showOut, bool showPanels, bool showAirfoils = false, bool showSurfaces = false);
    void CreateWingLLTPanels(int numPanels, int discType, BladeDiscretization &bladeDisc);
    QList<double> getPanelParameters(VortexPanel *panel, double aoa = 0, double beta = 0);
    void ComputeWingGeometry(BladeDiscretization &bladeDisc);

    Vec3 rotations, translations;
    double scale;
    QString fromBladePrototype;
    int m_discType, m_numPanels;
    double m_largestY, m_smallestY;

    //end newly added stuff


    void CreateSurfaces(bool isVawt = false);  // generic surface, LLT, VLM or Panel
    void CreateLLTPanels(int discType, int numPanels, bool isVawt, bool isCounterrotating, QList<VortexNode> &nodeList, BladeDiscretization &bladeDisc);
    void InterpolateBladeStations(int discType, int numPanels, BladeDiscretization &bladeDisc);

    void ComputeGeometry();

    void ScaleTwist(double Twist);
    void ScaleSpan(double NewSpan);
    void ScaleChord(double NewChord);
    double computeHimmelskamp(double Cl, double radius, double AoA, double chord, double TSR, double slope, double alpha_zero);

    QList<double> getBladeParameters(double radius, double AoA, bool interpolate = true, double Re = 0, bool himmelskamp = false, double TSR = 0, QList<AFC *> *AFC_list = NULL, double beta = 0, int fromBlade = 0);
    QList<double> getStrutParameters(int numStrut, double AoA, double Re, double position);
    double GetChordAt(double position);
    void addAllParents();

    // NM the optimum would be, if there was a Section class that holds all data for one section instead of the arrays
    QList<Airfoil *> m_Airfoils;  // NM holds the airfoil that is selected for each section
    QList<Polar360 *> m_Polar;  // NM holds the 360 polar that is selected for each section in single polar
	QStringList m_Range;  // NM holds only the displayed string for multipolar
    //variables used to store multi polar assginment
    QList< QList<Polar360 *> > m_MultiPolars;  // NM holds which polars are selected for each foil in m_PolAssFoils for multipolar
    QList<Airfoil *> m_PolarAssociatedFoils;  // NM holds all foils that are used for this blade
	QStringList m_MinMaxReynolds;  // NM holds the string that is copied into m_Range at some point

	QColor m_WingColor, m_OutlineColor;

    double m_TChord[MAXBLADESTATIONS+1];		// Chord length at each panel side
    double m_TLength[MAXBLADESTATIONS+1];  // the length of each panel  // calculated in ComputeGeometry
    double m_TPos[MAXBLADESTATIONS+1];		// b-position of each panel end on developed surface
    double m_TCircAngle[MAXBLADESTATIONS+1];		// b-position of each panel end projected on horizontal surface
    double m_TOffsetX[MAXBLADESTATIONS+1];		// b-position of each panel end
    double m_TTwist[MAXBLADESTATIONS+1];		//Twist value of each foil (measured to the wing root)
    double m_TPAxisX[MAXBLADESTATIONS+1];    //Definition of the Pitch Axis - Offset in X Direction
    double m_TOffsetZ[MAXBLADESTATIONS+1];    //Definition of the Pitch Axis - Offset in Z Direction
    double m_TPAxisY[MAXBLADESTATIONS+1];  // Pitch Axis - Offset in Y Direction  // calculated in CreateSurfaces
    double m_TFoilPAxisX[MAXBLADESTATIONS+1];    //Definition of the Pitch Axis - Offset in X Direction
    double m_TFoilPAxisZ[MAXBLADESTATIONS+1];    //Definition of the Pitch Axis - Offset in Z Direction
    double m_TRelPos[MAXBLADESTATIONS+1];  // relative position, starting at blade root  // not used for VAWT
    double m_TDihedral[MAXBLADESTATIONS+1];		//Dihedral value of each foil (measured to the wing root)
	
    QList<BladeSurface> m_Surface ;  // no need to serialize
    QVector<Strut*> m_StrutList;
    QVector<AFC*> m_AFCList;
    QVector<BDamage*> m_BDamageList;

    QStringList m_availableHAWTVariables, m_availableVAWTVariables;

	bool m_temporaryCopy;  // when true, blade and struts are not in the store and struts have to be deleted manually

};

#endif

