/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#ifndef GLOBALFUNCTIONS_H
#define GLOBALFUNCTIONS_H

#include <QString>
#include <QTextStream>
#include <QFile>
#include "Eigen/Core"

class Airfoil;

//file streaming and i/o options
QString UnifyString(QString strong);
QString truncateQStringMiddle(QString strong, int maxlength);
QStringList FileContentToQStringList(QString filename, bool giveWarning = true);
QString FindValueInFile(QString value, QStringList File, QString *error_msg = NULL, bool setmsg = true, bool *found = NULL);
QStringList FindLineWithKeyword(QString value, QStringList File, QString *error_msg = NULL, bool setmsg = false, bool *found = NULL, bool replace = false);
void WriteStreamToFile(QString FileName, QStringList FileStream);
Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> FindMatrixInFile(QString value, QStringList File, int rows, int cols, QString *error_msg, bool setmsg = true, bool *found = NULL);
bool FindKeywordInFile(QString value, QStringList File);
QStringList FindStreamSegmentByKeyword(QString keyword, QStringList File);
QList< QStringList >  FindStringDataTable(QString value, QStringList File, int cols, QString *error_msg, bool setmsg = true, bool *found = NULL);
QList< QList<double> > FindNumericDataTable(QString value, QStringList File, int cols, QString *error_msg, bool setmsg = true, bool *found = NULL);
QList< QList<double> > FindNumericValuesInFile(int minColCount, QStringList File, QString *error_msg = NULL, QString FileName = "");
QList< QStringList > FindDlcTableInFile(int colCount, QStringList File, bool allowAuto, QString *error_msg = NULL, QString FileName = "");

void CalculatePSD(QVector<float> *data, QVector<float> &xResult, QVector<float> &yResult, double dT);
void CalculatePSD2(QVector<float> *data, QVector<float> &freq, QVector<float> &amp, QVector<float> &phase, double dT);
void ConstrainAngle_0_360_Degree(double &ang);
void ConstrainAngle_0_360_Degree(float &ang);
void ConstrainAngle_0_360_Radian(double &ang);
void ConstrainAngle_0_360_Radian(float &ang);
void ConstrainAngle_180_180_Degree(double &ang);
void ConstrainAngle_180_180_Degree(float &ang);
void ConstrainAngle_180_180_Radian(double &ang);
void ConstrainAngle_180_180_Radian(float &ang);

void ExtractWpdataFileFromStream(QStringList &parameterStream, QStringList &wpDataStream, QString &wpDataFileName);
bool ReadSimOrMotionFileFromStream(QString &fileName, QStringList &fileStream);
bool ReadControllerParameterFile(QString &fileName, QString &wpDataFileName, QStringList &parameterStream, QStringList &wpDataStream, int controllerType);
void ReadFileToStream(QString &fileName, QStringList &stream, bool updateLastDir = true);

QString UpdateLastDirName(QString fileName);

// store related functions
void serializeAllStoresPublic(QString ident = "");
void sortAllStores();
void enableAllStoreSignals();
void disableAllStoreSignals();
void clearAllStores();
Airfoil *generateNACAFoil(int digits, int panels = 200);
Airfoil *interpolateFoils(Airfoil *foil1, Airfoil *foil2, double frac);
void exportFoil(QString FileName, Airfoil *foil);

void unloadAllControllers();
QString SerializeQBladeProject(QDataStream &ar, bool isStoring, QString ident = "", bool publicFormat = false, bool echo_off = false);
void emitObjectListsChanged(bool searchForLastActive);
void printStoreState ();

QString makeNameWithHigherNumber (QString oldName);

//misc
double findAbsMinMax(QVector<float> *vector);
double findMin(QVector<float> *vector);
double findMax(QVector<float> *vector);
void removeColumnXf(Eigen::MatrixXf& matrix, unsigned int colToRemove);
void removeRowXf(Eigen::MatrixXf& matrix, unsigned int rowToRemove);

//color relates stuff
typedef struct {
    double r;       // percent
    double g;       // percent
    double b;       // percent
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // percent
    double v;       // percent
} hsv;

hsv      rgb2hsv(rgb in);
rgb      hsv2rgb(hsv in);

#endif // GLOBALFUNCTIONS_H
