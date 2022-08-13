/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#ifndef IMPORTEXPORT_H
#define IMPORTEXPORT_H

#include <QString>
#include <QTextStream>
#include <src/Vec3.h>

class Polar360;
class CBlade;
class Airfoil;
class QTurbine;
class WindField;
class QSimulation;
class LinearWave;
class DynPolarSet;
class QVelocityCutPlane;

struct turbineParams{
    QString turbName, turbFile, eventFile, simFile, motionFile, loadingFile;
    double yaw, pitch, azi, rpmPrescribed;
    int relaxationSteps, prescribeRPMType, integratorType, structuralIterations, structuralSubsteps;
    Vec3 globalPosition, floaterRotation, floaterPosition;
    bool modNewton;
    QTurbine *turb;
};

WindField* ImportFromTurbSimWindInp(QString windFileName = "", bool deleteFiles = false);
WindField* ImportBinaryWindField(QString fileName = "");
QSimulation *ImportSimulationDefinition(QString fileName = "", bool skip = false, bool removeWind = true, bool searchForExisting = false, QString setName = "", bool noBCimport = false);
QTurbine *ImportTurbineDefinition(QString fileName = "", bool searchForExisting = false);
QTurbine *ImportMultiTurbineDefinition(QString fileName = "", bool searchForExisting = false);
CBlade* ImportQBladeFullBlade(QString bladeFile = "", bool searchForExisting = false);
QList<Polar360*> ImportMultiRePolarFile(QString fileName = "");
LinearWave* ImportLinearWaveDefinition(QString fileName ="", bool searchForExisting = false);
DynPolarSet* ImportDynamicPolarSet(QString fileName = "", bool searchForExisting = false);
QVelocityCutPlane* ImportVelocityCutPlane(QString fileName = "");
Airfoil *ImportAirfoil(QString fileName);


void ExportSimulationDefinition(QSimulation *sim, QList<QTurbine *> turbList, QString windName = "", QString fileName = "");
void ExportTurbineDefinition(QTurbine *turbine, QString fileName = "");
void ExportMultiTurbineDefinition(QTurbine *turbine, QString fileName = "");
void ExportQBladeFullBlade(CBlade *blade, bool isVawt, QString fileName = "");
void ExportMultiREPolarFile(QString pathName, QList<Polar360*> polarList, QString &fileName, QString uniqueID = "", bool exportAirfoils = false);
void ExportQBladeResults(QString folderName, bool hawc2bin, bool hawc2ascii, bool ascii);
void ExportBinaryWindfield(WindField *windfield, QString fileName = "");
void ExportSingleQBladeProject(QString fileName, QString folderName, bool hawc2bin, bool hawc2ascii, bool ascii, bool cut_txt, bool cut_vtu);
void ExportLinearWaveDefinition(QString fileName, LinearWave *wave);
void ExportDynamicPolarSet(DynPolarSet *set, QString fileName = "");
void ExportVelocityCutPlane(QVelocityCutPlane *plane, QString fileName = "");
bool ExportAirfoil(Airfoil *foil, QString fileName = "");


void UnloadQBladeProjectNoGUI();
bool LoadQBladeProjectNoGUI(QString PathName);
bool SaveQBladeProjectNoGUI(QString PathName);

void ExportFileHeader(QTextStream &stream);

bool sortPolarsByThickness(Polar360* p1, Polar360 *p2);
bool sortPolarsByReynolds(Polar360* p1, Polar360 *p2);

Airfoil* GenerateCircularFoil();

QList<double> ReynoldsInterpolatePolarList(double AoA, double reynolds, QList<Polar360*> *list);
QList<double> ReynoldsInterpolatePolarVector(double AoA, double reynolds, QVector<Polar360*> *vector);

Polar360* Get360Polar(QString m_FoilName,  QString PolarName);

void CreateTurbSimStream(QTextStream &stream, int seed, int gridPointsZ, int gridPointsY, double dt, double anaTime, double useTime, double hubHeight, double dimZ, double dimY,
                         double vertInf, double horInf, QString spectralModel, QString IECedition, QString tClass, QString tType, QString ETMc, QString profileType, double refHeight,
                         double windspeed, QString jetString, QString shearString, QString roughString);

#endif // IMPORTEXPORT_H
