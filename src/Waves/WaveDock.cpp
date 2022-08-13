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

#include "WaveDock.h"

#include <QMessageBox>

#include "src/GUI/CurvePickerDlg.h"
#include "src/MainFrame.h"
#include "src/Store.h"
#include "src/ParameterGrid.h"

#include "LinearWave.h"
#include "WaveCreatorDialog.h"
#include "WaveModule.h"
#include "WaveToolBar.h"


WaveDock::WaveDock (const QString & title, QMainWindow * parent, Qt::WindowFlags flags, WaveModule *module)
    : CreatorDock<LinearWave> (title, parent, flags)
{
    m_module = module;

    setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setVisible(false);

    QLabel *label;
    QGridLayout* grid;
    int gridRowCount = 0;

    m_curveStyleBox = new CurveStyleBox();

    m_contentVBox->addWidget(m_curveStyleBox->m_stylebox);

    connect(m_curveStyleBox->m_simulationLineButton, SIGNAL(clicked()), this, SLOT(onLineButtonClicked()));
    connect(m_curveStyleBox->m_showCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCheckBoxCanged()));
    connect(m_curveStyleBox->m_showCurveCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCurveCheckBoxCanged()));
    connect(m_curveStyleBox->m_showPointsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowPointsCheckBoxCanged()));
    connect(m_curveStyleBox->m_showHighlightCheckBox, SIGNAL(stateChanged(int)), m_module, SLOT(onShownWaveChanged()));

    m_plotBox = new QGroupBox (tr("Plot Range"));
    grid = new QGridLayout ();
    m_plotBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Time Start [s]:"));
    grid->addWidget(label,gridRowCount,0);
    m_plotStart = new NumberEdit();
    m_plotStart->setAutomaticPrecision(2);
    m_plotStart->setValue(0);
    grid->addWidget(m_plotStart,gridRowCount++,1);

    label = new QLabel (tr("Time End [s]:"));
    grid->addWidget(label,gridRowCount,0);
    m_plotEnd = new NumberEdit();
    m_plotEnd->setAutomaticPrecision(2);
    m_plotEnd->setValue(5000);
    grid->addWidget(m_plotEnd,gridRowCount++,1);

    label = new QLabel (tr("Timestep [s]:"));
    grid->addWidget(label,gridRowCount,0);
    m_plotDisc = new NumberEdit();
    m_plotDisc->setAutomaticPrecision(2);
    m_plotDisc->setMinimum(0.01);
    m_plotDisc->setValue(0.5);
    grid->addWidget(m_plotDisc,gridRowCount++,1);

    m_contentVBox->addWidget(m_plotBox);

    m_vizBox = new QGroupBox ("Visualization");
    m_contentVBox->addWidget(m_vizBox);
    grid = new QGridLayout;
    m_vizBox->setLayout(grid);
    gridRowCount = 0;

    m_showtext = new QPushButton(tr("Show Text"));
    m_showtext->setCheckable(true);
    m_showtext->setFlat(true);
    m_showtext->setChecked(true);
    grid->addWidget(m_showtext,gridRowCount, 0);

    m_showLegend = new QPushButton(tr("Show Legend"));
    m_showLegend->setCheckable(true);
    m_showLegend->setFlat(true);
    m_showLegend->setChecked(true);
    grid->addWidget(m_showLegend,gridRowCount++, 1);

    m_perspective = new QPushButton(tr("Perspective View"));
    m_perspective->setCheckable(true);
    m_perspective->setFlat(true);
    m_perspective->setChecked(false);
    grid->addWidget(m_perspective,gridRowCount, 0);

    m_showGrid = new QPushButton(tr("Show Grid"));
    m_showGrid->setCheckable(true);
    m_showGrid->setFlat(true);
    m_showGrid->setChecked(true);
    grid->addWidget(m_showGrid,gridRowCount++, 1);

    m_showSurface = new QPushButton(tr("Show Surface"));
    m_showSurface->setCheckable(true);
    m_showSurface->setFlat(true);
    m_showSurface->setChecked(true);
    grid->addWidget(m_showSurface,gridRowCount, 0);

    m_showGround = new QPushButton(tr("Show Ground"));
    m_showGround->setCheckable(true);
    m_showGround->setFlat(true);
    m_showGround->setChecked(true);
    grid->addWidget(m_showGround,gridRowCount++, 1);


    label = new QLabel (tr("Width [m]:"));
    grid->addWidget(label,gridRowCount,0);
    m_width = new NumberEdit();
    m_width->setMinimum(0);
    m_width->setAutomaticPrecision(2);
    m_width->setValue(400);
    grid->addWidget(m_width,gridRowCount++,1);

    label = new QLabel (tr("Length [m]:"));
    grid->addWidget(label,gridRowCount,0);
    m_length = new NumberEdit();
    m_length->setMinimum(0);
    m_length->setAutomaticPrecision(2);
    m_length->setValue(400);
    grid->addWidget(m_length,gridRowCount++,1);

    label = new QLabel (tr("Width Disc [-]:"));
    grid->addWidget(label,gridRowCount,0);
    m_discw = new NumberEdit();
    m_discw->setMinimum(1);
    m_discw->setAutomaticPrecision(0);
    m_discw->setValue(100);
    grid->addWidget(m_discw,gridRowCount++,1);

    label = new QLabel (tr("Length Disc [-]:"));
    grid->addWidget(label,gridRowCount,0);
    m_discl = new NumberEdit();
    m_discl->setMinimum(1);
    m_discl->setAutomaticPrecision(0);
    m_discl->setValue(100);
    grid->addWidget(m_discl,gridRowCount++,1);

    m_boundaryConditionsBox = new QGroupBox ("Boundary Conditions");
    m_contentVBox->addWidget(m_boundaryConditionsBox);
    grid = new QGridLayout;
    m_boundaryConditionsBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Gravity [m/s^2]:"));
    grid->addWidget(label,gridRowCount,0);
    m_gravity = new NumberEdit();
    m_gravity->setMinimum(0);
    m_gravity->setAutomaticPrecision(2);
    m_gravity->setValue(9.81);
    grid->addWidget(m_gravity,gridRowCount++,1);

    label = new QLabel (tr("Water Depth [m]:"));
    grid->addWidget(label,gridRowCount,0);
    m_depth = new NumberEdit();
    m_depth->setMinimum(0.1);
    m_depth->setAutomaticPrecision(2);
    m_depth->setValue(200);
    grid->addWidget(m_depth,gridRowCount++,1);

    m_timeBox = new QGroupBox ("Time Control");
    m_contentVBox->addWidget(m_timeBox);
    grid = new QGridLayout;
    m_timeBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Time [s]:"));
    grid->addWidget(label,gridRowCount,0);
    m_time = new NumberEdit();
    m_time->setAutomaticPrecision(4);
    m_time->setValue(0);
    grid->addWidget(m_time,gridRowCount++,1);

    label = new QLabel (tr("dT [s]:"));
    grid->addWidget(label,gridRowCount,0);
    m_timeIncr = new NumberEdit();
    m_timeIncr->setAutomaticPrecision(2);
    m_timeIncr->setValue(0.1);
    grid->addWidget(m_timeIncr,gridRowCount++,1);

    m_animation = new QPushButton(tr("Start Animation"));
    m_animation->setCheckable(false);
    grid->addWidget(m_animation,gridRowCount++, 1);

    connect(m_length, SIGNAL(valueChanged(double)), this, SLOT(RedrawGl()));
    connect(m_width, SIGNAL(valueChanged(double)), this, SLOT(RedrawGl()));
    connect(m_length, SIGNAL(valueChanged(double)), m_module, SLOT(OnCenterScene()));
    connect(m_width, SIGNAL(valueChanged(double)), m_module, SLOT(OnCenterScene()));
    connect(m_discl, SIGNAL(valueChanged(double)), this, SLOT(RedrawGl()));
    connect(m_discw, SIGNAL(valueChanged(double)), this, SLOT(RedrawGl()));
    connect(m_time, SIGNAL(valueChanged(double)), this, SLOT(RedrawGl()));
    connect(m_gravity, SIGNAL(valueChanged(double)), m_module, SLOT(UpdateDispersion()));
    connect(m_depth, SIGNAL(valueChanged(double)), m_module, SLOT(UpdateDispersion()));

    connect(m_showtext, SIGNAL(toggled(bool)), this, SLOT(ReportGlChanged()));
    connect(m_showLegend, SIGNAL(toggled(bool)), this, SLOT(ReportGlChanged()));
    connect(m_showGrid, SIGNAL(toggled(bool)), this, SLOT(RedrawGl()));
    connect(m_showSurface, SIGNAL(toggled(bool)), this, SLOT(RedrawGl()));
    connect(m_showGround, SIGNAL(toggled(bool)), this, SLOT(RedrawGl()));
    connect(m_perspective, SIGNAL(toggled(bool)), this, SLOT(RedrawGl()));

    connect(m_animation, SIGNAL(clicked()), this, SLOT(onStartAnimation()));

    connect(m_plotStart, SIGNAL(valueChanged(double)), this, SLOT(PrepareGraphData()));
    connect(m_plotEnd, SIGNAL(valueChanged(double)), this, SLOT(PrepareGraphData()));
    connect(m_plotDisc, SIGNAL(valueChanged(double)), this, SLOT(PrepareGraphData()));

	m_contentVBox->addStretch();		
	
    addScrolledDock (Qt::LeftDockWidgetArea, parent);

    setShownObject(NULL);
}

void WaveDock::OnTwoDView(){
    m_curveStyleBox->m_stylebox->show();
    m_plotBox->show();
    m_vizBox->hide();
}

void WaveDock::OnGLView(){
    m_curveStyleBox->m_stylebox->hide();
    m_plotBox->hide();
    m_vizBox->show();
}

void WaveDock::setShownObject(LinearWave *newObject) {
    CreatorDock::setShownObject(newObject);

    m_shownObject = newObject;

    m_curveStyleBox->UpdateContent(m_shownObject);

    for (int i=0;i<g_WaveStore.size();i++) g_WaveStore.at(i)->setHighlight(false);
    if (m_curveStyleBox->m_showHighlightCheckBox->isChecked() && newObject)
        newObject->setHighlight(true);

}

void WaveDock::onEditCopyButtonClicked () {

    onStopAnimation();

    WaveCreatorDialog *dialog = new WaveCreatorDialog (m_shownObject, m_module);
    dialog->exec();
    delete dialog;
}

void WaveDock::onRenameButtonClicked () {

    onStopAnimation();

    g_WaveStore.rename(m_shownObject);
}

void WaveDock::onDeleteButtonClicked () {

    if(!m_shownObject) return;

    onStopAnimation();

    if (m_shownObject->getNumChilds() != 0){
        QString strong = tr("Are you sure you want to delete")  +"\n\n"+ m_shownObject->getName() +"?";
        strong+= tr("\n\nand all associated Simulations:");
        for (int i=0;i<m_shownObject->getNumChilds();i++) strong += "\n\n" + m_shownObject->getChild(i)->getName();
        int resp = QMessageBox::question(this,tr("Question"), strong,  QMessageBox::Yes | QMessageBox::No);
        if (resp != QMessageBox::Yes) return;
    }

    int lastIndex = m_module->m_waveToolbar->m_waveComboBox->currentIndex();

    g_WaveStore.remove(m_shownObject);

    if (m_module->m_waveToolbar->m_waveComboBox->count() > lastIndex)
        m_module->m_waveToolbar->m_waveComboBox->setCurrentIndex(lastIndex);
    else if(m_module->m_waveToolbar->m_waveComboBox->count())
        m_module->m_waveToolbar->m_waveComboBox->setCurrentIndex(m_module->m_waveToolbar->m_waveComboBox->count()-1);

}

void WaveDock::onNewButtonClicked () {

    onStopAnimation();

    WaveCreatorDialog *dialog = new WaveCreatorDialog (NULL, m_module);
    dialog->exec();
    delete dialog;
}

void WaveDock::adjustShowCheckBox() {
    m_curveStyleBox->m_showCheckBox->setChecked(m_shownObject->isShownInGraph());
}

void WaveDock::onLineButtonClicked() {

    QPen pen;
    if (m_curveStyleBox->GetLinePen(pen)) m_shownObject->setPen(pen);
}

void WaveDock::onShowCheckBoxCanged () {
    m_shownObject->setShownInGraph(m_curveStyleBox->m_showCheckBox->isChecked());
    m_module->reloadAllGraphCurves();
}

void WaveDock::onShowPointsCheckBoxCanged () {
    m_shownObject->setDrawPoints(m_curveStyleBox->m_showPointsCheckBox->isChecked());
    m_module->update();
}

void WaveDock::onShowCurveCheckBoxCanged () {
    m_shownObject->setDrawCurve(m_curveStyleBox->m_showCurveCheckBox->isChecked());
    m_module->update();
}

void WaveDock::RedrawGl(){
    if(!m_module->isGlView()) return;
    m_module->UpdateView();
    m_module->reportGLChange();
}

void WaveDock::ReportGlChanged(){
    if(!m_module->isGlView()) return;
    m_module->reportGLChange();
}

void WaveDock::OnAnimate(){

    connect(this, SIGNAL(updateProgress()), this, SLOT(IncrementShownTime()), Qt::BlockingQueuedConnection);

    m_bStopAnimation = false;
    while (!m_bStopAnimation){

        emit updateProgress();
    }

    disconnect(this, SIGNAL(updateProgress()), this, SLOT(IncrementShownTime()));

}

void WaveDock::IncrementShownTime(){

    m_time->setValue(m_time->getValue()+m_timeIncr->getValue());
    m_module->reloadAllTimeGraphs();

}

void WaveDock::onStartAnimation(){

    if(!m_module->m_shownWave) return;

    disconnect(m_animation, SIGNAL(clicked()), this, SLOT(onStartAnimation()));
    connect(m_animation, SIGNAL(clicked()), this, SLOT(onStopAnimation()));

    m_animation->setText("Stop Animation");

    m_WaveAnimationThread =  new WaveAnmiationThread ();
    m_WaveAnimationThread->dock = this;
    connect(m_WaveAnimationThread, SIGNAL(finished()), m_WaveAnimationThread, SLOT(deleteLater()), Qt::QueuedConnection);
    m_WaveAnimationThread->start();

}

void WaveDock::onStopAnimation(){

    m_bStopAnimation = true;

    m_animation->setText("Start Animation");

    connect(m_animation, SIGNAL(clicked()), this, SLOT(onStartAnimation()));
    disconnect(m_animation, SIGNAL(clicked()), this, SLOT(onStopAnimation()));

    m_WaveAnimationThread = NULL;
}

void WaveDock::PrepareGraphData(){

    m_plotStart->blockSignals(true);
    m_plotEnd->blockSignals(true);
    m_plotDisc->blockSignals(true);

    if (m_plotEnd->getValue() < m_plotStart->getValue())
        m_plotEnd->setValue(m_plotStart->getValue()+5000.0);

    if ((m_plotEnd->getValue() - m_plotStart->getValue()) / m_plotDisc->getValue() > 100000)
        m_plotDisc->setValue((m_plotEnd->getValue()-m_plotStart->getValue())/100000.0);

    m_plotStart->blockSignals(false);
    m_plotEnd->blockSignals(false);
    m_plotDisc->blockSignals(false);


    for (int i=0;i<g_WaveStore.size();i++)
        g_WaveStore.at(i)->PrepareGraphData(m_plotStart->getValue(),m_plotEnd->getValue(),m_plotDisc->getValue(),m_depth->getValue());

    m_module->reloadAllGraphCurves();

}
