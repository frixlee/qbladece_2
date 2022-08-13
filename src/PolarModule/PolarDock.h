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

#ifndef POLARDOCK_H
#define POLARDOCK_H

#include <QPushButton>
#include <QThread>
#include <QProgressDialog>
#include <QMutex>

#include "src/GUI/NumberEdit.h"

#include "src/CurveStyleBox.h"
#include "../ScrolledDock.h"

class PolarModule;
class QProgressDialog;

class PolarDock : public ScrolledDock
{
    Q_OBJECT

public:
    PolarDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, PolarModule *module);

    CurveStyleBox *m_curveStyleBox;
    bool m_stopRequested;
    QVector<Polar*> m_newPolarList;
    NumberEdit *m_start, *m_end, *m_delta;
    QCheckBox *m_showOp;
    QProgressDialog *m_progressDialog;
    int m_progress;

private:

    PolarModule *m_module;


    QPushButton *m_renameButton, *m_editCopyButton, *m_deleteButton, *m_newButton, *m_analysisButton;
    QCheckBox *m_storeOpPoint;

public slots:
    void adjustShowCheckBox();
    void onHighlightChanged();
    void onOpPointChanged();
    void onUpdateProgress();
    void onProgressCanceled();
    void onProgressFinished();

private slots:
    void onRenameButtonClicked();
    void onDeleteButtonClicked();
    void onDeleteCurrentAirfoilPolars();
    void onDeleteAllPolars();
    void onNewButtonClicked();
    void onEditButtonClicked();
    void onLineButtonClicked();
    void onShowCheckBoxCanged();
    void onShowPointsCheckBoxCanged();
    void onShowCurveCheckBoxCanged();
    void onXFoilAnalysis();
    void onXFoilBatchAnalysis();

};

#endif // POLARDOCK_H
