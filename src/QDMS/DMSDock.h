/**********************************************************************

    Copyright (C) 2013 David Marten <david.marten@qblade.org>

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

#ifndef DMSDOCK_H
#define DMSDOCK_H


#include <QObject>
#include <QMainWindow>
#include "../ScrolledDock.h"
/*
#include <QtWidgets>
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressDialog>
#include <QThread>
#include <QDebug>
#include <QStackedWidget>
#include "src/GLWidget.h"
#include "../XWidgets.h"
#include "../GUI/NumberEdit.h"
#include "../GUI/CurveCbBox.h"
#include "../GUI/CurveButton.h"
#include "../GUI/CurveDelegate.h"
*/

class DMSDock : public ScrolledDock
{
	Q_OBJECT

public:
	DMSDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags);
};



#endif // DMSDOCK_H
