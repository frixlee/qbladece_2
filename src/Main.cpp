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

#define __CL_ENABLE_EXCEPTIONS

#include <QDebug>
#include <QTimer>
#include <QTest>

#include "QBladeApplication.h"
#include "Globals.h"

int main(int argc, char *argv[]) {

    g_ChronoVersion = QString(chrono_string);
    g_VersionName = QString("QBlade CE v "+QString(version_string)+" "+QString(compiled_string));

    QBladeApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QBladeApplication app(argc, argv);

    return app.exec();
}

QRegularExpression ANY_NUMBER = QRegularExpression(QRegularExpression::anchoredPattern("^\\-?\\+?[0-9]+(\\.[0-9]+)?(e?E?\\-?\\+?[0-9]+)?$"));
QRegularExpression S_CHAR = QRegularExpression("["+QRegularExpression::escape("\\/:*?\"<>|")+"]");
QString g_ChronoVersion = "";
QString g_VersionName = "";
QString g_turbsimPath = "";
QString g_xfoilPath = "";
QString g_applicationDirectory = "";
QString g_controllerPath = "";
QString g_tempPath = "";

bool debugStruct = false;
bool debugTurbine = false;
bool debugController = false;
bool debugSimulation = false;
bool debugStores = false;
bool debugSerializer = true;
bool twoDAntiAliasing = true;
bool uintRes = false;
bool uintVortexWake = false;
bool isGUI = true;
bool isWIN = !QString(compiled_string).contains("unix");

double globalLineAlpha = 0.7;
