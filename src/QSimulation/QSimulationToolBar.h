/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#ifndef QSIMULATIONTOOLBAR_H
#define QSIMULATIONTOOLBAR_H

#include <QMainWindow>
#include <QToolBar>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include "../StoreAssociatedComboBox.h"
#include "QSimulation.h"
#include <QThread>

class QSimulationModule;

class QSimulationToolBar : public QToolBar
{

    Q_OBJECT

private:
    class QMultiReplayThread : public QThread
    {
    public:
        QSimulation *simulation;

    private:
        void run () {simulation->onStartReplay(); }
    };

public:

    QMultiReplayThread *m_QMultiReplayThread;

    QAction *TwoDView, *GLView, *DualView, *HideWidgets;

    QSimulationToolBar(QMainWindow *parent, QSimulationModule *module);
    QSimulationModule *m_module;
    QSimulationComboBox *m_simulationBox;
    QTurbineComboBox *m_turbineBox;

    QSlider *m_Timesteps;
    QLabel *m_TimeLabel, *m_TimestepLabel;
    QPushButton *m_startReplay;
    QDoubleSpinBox *m_DelayBox;

    QSimulation *m_QSim;
    QTurbine *m_QTurbine;

    bool isReplayRunning() { if (!m_QMultiReplayThread) return false; else return m_QMultiReplayThread->isRunning();}


    void CurrentSimulationChanged(QSimulation *simulation);
    void CurrentTurbineChanged(QTurbine *turbine);
    void DisableBoxes();
    void EnableBoxes();
    void setShownTimeForAllSimulations();

public slots:
    void onStartReplay();
    void onStopReplay();
    void OnTimeSliderChanged();
    void SetTimeStepSlider(int i) { m_Timesteps->setValue(i); }

};

#endif // QSIMULATIONTOOLBAR_H
