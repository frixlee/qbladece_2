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

#ifndef WAVEDOCK_H
#define WAVEDOCK_H

#include <QButtonGroup>
#include <QRadioButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QMainWindow>
#include <QThread>
#include <QDoubleSpinBox>

#include "src/GUI/NumberEdit.h"
#include "src/GUI/CurveButton.h"
#include "../CreatorDock.h"
#include "src/CurveStyleBox.h"

class WaveModule;
class LinearWave;


class WaveDock : public CreatorDock<LinearWave>
{
	Q_OBJECT

    class WaveAnmiationThread : public QThread
    {
    public:
        WaveDock *dock;

    private:
        void run () {dock->OnAnimate(); }
    };
	
public:
    WaveDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags, WaveModule *module);
	
    void setShownObject (LinearWave *newObject);

    WaveAnmiationThread *m_WaveAnimationThread;

    QGroupBox *m_plotBox;

    CurveStyleBox *m_curveStyleBox;

    QButtonGroup *m_componentGroup;
    QPushButton *m_perspective, *m_showtext, *m_showLegend, *m_showGrid, *m_showSurface, *m_showGround, *m_animation;
    NumberEdit *m_width, *m_length, *m_discw, *m_discl, *m_gravity, *m_depth, *m_time, *m_timeIncr, *m_plotStart, *m_plotEnd, *m_plotDisc;
    QGroupBox *m_vizBox, *m_timeBox, *m_boundaryConditionsBox;

    bool m_bStopAnimation;


    void OnTwoDView();
    void OnGLView();
    void adjustShowCheckBox();
    void OnAnimate();

private:
    WaveModule *m_module;

signals:
    void updateProgress();

public slots:
    void PrepareGraphData();
    void IncrementShownTime();
    void onStartAnimation();
    void onStopAnimation();
    void ReportGlChanged();
    void RedrawGl();
	void onEditCopyButtonClicked ();
	void onRenameButtonClicked ();
	void onDeleteButtonClicked ();
	void onNewButtonClicked ();
    void onLineButtonClicked();
    void onShowCheckBoxCanged();
    void onShowPointsCheckBoxCanged();
    void onShowCurveCheckBoxCanged();
};

#endif // WINDFIELDDOCK_H
