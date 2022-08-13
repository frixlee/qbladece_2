/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#include "FoilModule.h"

#include <math.h>

#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QProcess>
#include <QSettings>

#include "FoilMenu.h"
#include "FoilToolBar.h"
#include "FoilDock.h"
#include "FoilEditDlg.h"
#include "src/GlobalFunctions.h"
#include "src/ColorManager.h"

#include "../src/Globals.h"
#include "../src/FoilModule/Airfoil.h"
#include "../src/QBEM/CircularFoilDlg.h"
#include "../src/QBEM/BEM.h"
#include "../src/QDMS/DMS.h"
#include "../src/Store.h"
#include "../src/BinaryProgressDialog.h"
#include "../MainFrame.h"
#include "../TwoDWidget.h"
#include "../src/PolarModule/PolarModule.h"
#include "../src/PolarModule/PolarToolBar.h"



FoilModule::FoilModule(QMainWindow *mainWindow, QToolBar *toolbar)
{

    m_editFoil = NULL;
    m_globalModuleIndentifier = FOILMODULE;

    registrateAtToolbar(tr("Airfoil Design Module"),
                        tr("Create, modify and manage airfoil geometries"),
                        ":/images/foil.png", toolbar);

    g_mainFrame->ModuleMenu->addAction(m_activationAction);
    m_Menu = new FoilMenu (mainWindow, this);
    mainWindow->menuBar()->addMenu(m_Menu);
    m_ToolBar = new FoilToolBar(mainWindow, this);
    m_Dock = new FoilDock (tr("Airfoil Design Module"), mainWindow, 0, this);

    m_ContextMenu = new FoilCtxMenu (m_twoDWidget, this);
    m_ContextMenu->getAutoResetAction()->setVisible(false);
    m_ContextMenu->getExportAction()->setVisible(false);
    m_ContextMenu->getShowAllAction()->setText("Show All Airfoils");
    m_ContextMenu->getHideAllAction()->setText("Show Current Airfoil Only");

    m_highlightAction = new QAction("Highlight Current Airfoil");
    m_highlightAction->setCheckable(true);
    m_highlightAction->setChecked(true);
    connect(m_highlightAction,SIGNAL(triggered(bool)),this,SLOT(reloadAllGraphs()));
    m_ContextMenu->insertAction(m_ContextMenu->getResetAction(),m_highlightAction);

    disconnect(m_ContextMenu->getResetAction(),&QAction::triggered,0,0);
    connect(m_ContextMenu->getResetAction(), SIGNAL(triggered(bool)), this, SLOT(scaleFoilGraphs()));

    connect(&g_foilStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));
    connect(&g_foilStore, SIGNAL(objectListChanged(bool)), m_Dock, SLOT(FillFoilTable()));

    m_graph[0] = new NewGraph ("FoilGraphOne", this, {NewGraph::FoilGraph, "", "", false, false});
    m_graph[1] = new NewGraph ("FoilGraphTwo", this, {NewGraph::FoilGraph, "", "", false, false});
    m_graph[2] = new NewGraph ("FoilGraphThree", this, {NewGraph::FoilGraph, "", "", false, false});
    m_graph[3] = new NewGraph ("FoilGraphFour", this, {NewGraph::FoilGraph, "", "", false, false});
    m_graph[4] = new NewGraph ("FoilGraphFive", this, {NewGraph::FoilGraph, "", "", false, false});
    m_graph[5] = new NewGraph ("FoilGraphSix", this, {NewGraph::FoilGraph, "", "", false, false});
    m_graph[6] = new NewGraph ("FoilGraphSeven", this, {NewGraph::FoilGraph, "", "", false, false});
    m_graph[7] = new NewGraph ("FoilGraphEight", this, {NewGraph::FoilGraph, "", "", false, false});

    for (int i=0;i<8;i++) g_graphList.append(m_graph[i]);


    createActions();
}

void FoilModule::onFoilTableCtxMenu(const QPoint &point){

    m_ContextMenu->exec(point);

}

void FoilModule::addMainMenuEntries(){
    g_mainFrame->menuBar()->addMenu(m_Menu);
}

QList<NewCurve*> FoilModule::prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                            NewGraph::GraphType graphTypeMulti){

    QList<NewCurve*> curves;

    if (!g_foilStore.size()) g_pCurFoil = NULL;

    for (int i=0;i<g_foilStore.size();i++)
        g_foilStore.at(i)->setHighlight(false);

    if (g_pCurFoil && m_highlightAction->isChecked())
        g_pCurFoil->setHighlight(true);

    if (m_editFoil){
        NewCurve *curve = m_editFoil->newCurve("","",NewGraph::FoilGraph);
        if (curve){

            curves.append(curve);

            if (m_highlightPoint >= 0 && m_highlightPoint < m_editFoil->n){
                curve = new NewCurve();
                curve->getAssociatedObject()->pen()->setColor(QColor(0,0,0));
                curve->getAssociatedObject()->pen()->setWidth(m_editFoil->getPen().width()+4);
                curve->getAssociatedObject()->setDrawPoints(true);
                curve->addPoint(m_editFoil->x[m_highlightPoint],m_editFoil->y[m_highlightPoint]);
                curves.append(curve);
            }

            return curves;
        }
    }

    g_foilStore.addAllCurves(&curves,"","",NewGraph::FoilGraph);
    g_foilStore.addAllCurves(&curves,"MIDLINE","MIDLINE",NewGraph::FoilGraph);

    return curves;
}

QStringList FoilModule::prepareMissingObjectMessage(){

//    if (g_foilStore.isEmpty()) {

//        QStringList message;
//        if (g_mainFrame->m_iApp == FOILMODULE) {
//            message = QStringList(">>> Import or create a new Airfoil");
//        }
//        message.prepend("- No Airfoil in Database");
//        return message;
//    }
    return QStringList();

}

void FoilModule::onActivationActionTriggered() {
    ModuleBase::onActivationActionTriggered();
    showModule();
    g_mainFrame->switchToTwoDWidget();
    m_Dock->show();
    m_Dock->FillFoilTable();
    m_ToolBar->show();
    scaleFoilGraphs();

}

void FoilModule::scaleFoilGraphs(){

    for (int i=0;i<8;i++){
        if (m_graph[i]->getDrawingArea()->width() && m_graph[i]->getDrawingArea()->height()){

            m_graph[i]->setNoAutoResize(true);

            m_graph[i]->getXAxis()->setOptimalLimits(0,1.0);

            double xRange = fabs(m_graph[i]->getXAxis()->getHighLimit()-m_graph[i]->getXAxis()->getLowLimit());
            double ratio = double(m_graph[i]->getDrawingArea()->width())/double(m_graph[i]->getDrawingArea()->height());
            double yRange = xRange/ratio;

            m_graph[i]->getYAxis()->setLimits(-yRange/2.0,yRange/2.0);
        }
    }
}

void FoilModule::onModuleChanged() {
    if (g_mainFrame->getCurrentModule() == this) {
        ModuleBase::onModuleChanged();
        hideModule();
        m_Dock->hide();
        m_ToolBar->hide();
    }
}

void FoilModule::onHideDocks(bool hide) {
    m_Dock->setVisible(!hide);
}

FoilModule::~FoilModule() {
    if (m_firstView == false) {
        delete m_graph[0];
        delete m_graph[1];
        delete m_graph[2];
        delete m_graph[3];
        delete m_graph[4];
        delete m_graph[5];
        delete m_graph[6];
        delete m_graph[7];

        if(g_mainFrame->m_bSaveSettings){
            QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
            settings.setValue(QString("modules/FoilModule/graphArrangement"), getGraphArrangement());
        }
    }
}

QStringList FoilModule::getAvailableGraphVariables(bool xAxis){
    return QStringList();
}  // override from TwoDWidgetInterface

QPair<ShowAsGraphInterface*,int> FoilModule::getHighlightDot(NewGraph::GraphType graphType){
    return QPair<ShowAsGraphInterface*,int> (NULL, -1);
}

void FoilModule::showAll() {
    if (!g_foilStore.size()) return;
    g_foilStore.showAllCurves(true);
    reloadAllGraphCurves();
    m_Dock->FillFoilTable();
}

void FoilModule::hideAll() {
    if (!g_foilStore.size()) return;
    g_foilStore.showAllCurves(false,g_pCurFoil);
    reloadAllGraphCurves();
    m_Dock->FillFoilTable();
}

void FoilModule::initView(){
    if (m_firstView) {

        m_firstView = false;

        QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
        setGraphArrangement(static_cast<TwoDWidgetInterface::GraphArrangement>
                            (settings.value("modules/FoilModule/graphArrangement", TwoDWidgetInterface::Single).toInt()));

        m_Dock->FillFoilTable();

        setGraphArrangement(TwoDWidgetInterface::Single);
    }
}

void FoilModule::onPaintEvent(QPaintEvent */*event*/) {
    QPainter painter (m_twoDWidget);
    painter.fillRect(m_twoDWidget->rect(), g_mainFrame->getBackgroundColor());

    switch (getGraphArrangement()) {  // missing break intended
    case EightVertical:
    case Eight:
        m_graph[7]->drawGraph(painter);
        m_graph[6]->drawGraph(painter);
    case SixVertical:
    case Six:
        m_graph[5]->drawGraph(painter);
        m_graph[4]->drawGraph(painter);
    case QuadVertical:
    case Quad:
        m_graph[3]->drawGraph(painter);
    case Vertical3:
        m_graph[2]->drawGraph(painter);
    case Vertical:
    case Horizontal:
        m_graph[1]->drawGraph(painter);
    case Single:
        m_graph[0]->drawGraph(painter);
        break;
    }

    if (twoDAntiAliasing){
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
    }

    QRect r = m_twoDWidget->geometry();

    double width = r.width();
    double height = r.height();

    int posLarge = height / (20*1.6);
    int largeFont = height / (70*1.2);
    int pos = 1150 / 30;

    if (width > 300 && g_pCurFoil) {

            painter.setPen(g_mainFrame->m_TextColor);
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), largeFont));

            QFontMetrics metrics = painter.fontMetrics();
            QRect fontRect = metrics.boundingRect("Current Airfoil: "+g_pCurFoil->getName());
            fontRect.width();

            painter.drawText(width/2-fontRect.width()/2, pos+posLarge, "Current Airfoil: "+g_pCurFoil->getName());
    }

    if (!g_pCurFoil){
        painter.setPen(g_mainFrame->m_TextColor);
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), largeFont));

        QFontMetrics metrics = painter.fontMetrics();
        QRect fontRect = metrics.boundingRect(">>> Import or create a new Airfoil from the Menu");
        fontRect.width();

        painter.drawText(width/2-fontRect.width()/2, pos+posLarge, "No Airfoil in database!");
        painter.drawText(width/2-fontRect.width()/2, pos+3*posLarge, ">>> Import or create a new Airfoil from the Menu");
    }

}

void FoilModule::onResizeEvent() {
    const int border = 0;  // border of whole widget
    const int gap = 0;  // gap between two graphs
    int w, h;
    QRect max (m_twoDWidget->rect());

    switch (getGraphArrangement()) {
    case Single:
        w = max.width()-2*border;
        h = max.height()-2*border;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        break;
    case Vertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-gap) / 2;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h+max.height()%2));
        break;
    case Vertical3:
        w = max.width()-2*border;
        h = (max.height()-2*border-2*gap) / 3;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h+max.height()%3));
        break;
    case Horizontal:
        w = (max.width()-2*border-gap) / 2;
        h = max.height()-2*border;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        break;
    case Quad:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 2;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h+max.height()%2));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h+max.height()%2));
        break;
    case QuadVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-3*gap) / 4;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h+max.height()%4));
        break;
    case Six:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 3;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*h+gap, w, h+max.height()%3));
        m_graph[5]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+2*h+gap, w+max.width()%2, h+max.height()%3));
        break;
    case SixVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-5*gap) / 6;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+4*(h+gap), w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border, max.y()+border+5*(h+gap), w, h+max.height()%6));
        break;
    case Eight:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 4;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*h+gap, w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+2*h+gap, w+max.width()%2, h));
        m_graph[6]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*h+gap, w, h+max.height()%4));
        m_graph[7]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+3*h+gap, w+max.width()%2, h+max.height()%4));
        break;
    case EightVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-7*gap) / 8;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+4*(h+gap), w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border, max.y()+border+5*(h+gap), w, h));
        m_graph[6]->setDrawingArea(QRect(max.x()+border, max.y()+border+6*(h+gap), w, h));
        m_graph[7]->setDrawingArea(QRect(max.x()+border, max.y()+border+7*(h+gap), w, h+max.height()%8));
        break;
    }

    scaleFoilGraphs();
}

void FoilModule::createActions(){


    m_ContextMenu->addSeparator()->setText("");

    m_importFoilAction = new QAction("Import Airfoil File");
    m_ContextMenu->addAction(m_importFoilAction);
    m_Menu->addAction(m_importFoilAction);
    connect(m_importFoilAction,SIGNAL(triggered(bool)),this,SLOT(onImportFoil()));

    m_exportFoilAction = new QAction("Export Current Airfoil");
    m_ContextMenu->addAction(m_exportFoilAction);
    m_Menu->addAction(m_exportFoilAction);
    connect(m_exportFoilAction,SIGNAL(triggered(bool)),this,SLOT(onExportFoil()));

    m_renameAction = new QAction("Rename Current Airfoil");
    m_ContextMenu->addAction(m_renameAction);
    m_Menu->addAction(m_renameAction);
    connect(m_renameAction,SIGNAL(triggered(bool)),this,SLOT(onRenameFoil()));

    m_deleteAction = new QAction("Delete Current Airfoil");
    m_ContextMenu->addAction(m_deleteAction);
    m_Menu->addAction(m_deleteAction);
    connect(m_deleteAction,SIGNAL(triggered(bool)),this,SLOT(onDeleteFoil()));

    m_ContextMenu->addSeparator()->setText("");
    m_Menu->addSeparator()->setText("");

    m_normalizeFoil = new QAction("Normalize Current Airfoil");
    m_ContextMenu->addAction(m_normalizeFoil);
    m_Menu->addAction(m_normalizeFoil);
    connect(m_normalizeFoil,SIGNAL(triggered(bool)),this,SLOT(onNormalizeFoil()));

    m_duplicateAction = new QAction("Duplicate Current Airfoil");
    m_ContextMenu->addAction(m_duplicateAction);
    m_Menu->addAction(m_duplicateAction);
    connect(m_duplicateAction,SIGNAL(triggered(bool)),this,SLOT(onDuplicateFoil()));

    m_derotateFoils = new QAction("De-rotate Current Airfoil");
    m_ContextMenu->addAction(m_derotateFoils);
    m_Menu->addAction(m_derotateFoils);
    connect(m_derotateFoils,SIGNAL(triggered(bool)),this,SLOT(onDerotateFoil()));

    m_refineAction = new QAction("Repanel Current Airfoil");
    m_ContextMenu->addAction(m_refineAction);
    m_Menu->addAction(m_refineAction);
    connect(m_refineAction,SIGNAL(triggered(bool)),this,SLOT(onRefineGlobally()));

    m_TEGapAction = new QAction("Set T.E. Gap for Current Airfoil");
    m_ContextMenu->addAction(m_TEGapAction);
    m_Menu->addAction(m_TEGapAction);
    connect(m_TEGapAction,SIGNAL(triggered(bool)),this,SLOT(onTEGap()));

    m_TEFlapAction = new QAction("Set T.E. Flap for Current Airfoil");
    m_ContextMenu->addAction(m_TEFlapAction);
    m_Menu->addAction(m_TEFlapAction);
    connect(m_TEFlapAction,SIGNAL(triggered(bool)),this,SLOT(onTEFlap()));

    m_editAction = new QAction("Edit Current Airfoil Coordinates");
    m_ContextMenu->addAction(m_editAction);
    m_Menu->addAction(m_editAction);
    connect(m_editAction,SIGNAL(triggered(bool)),this,SLOT(onEditFoil()));

    m_ContextMenu->addSeparator()->setText("");
    m_Menu->addSeparator()->setText("");

    m_NACAAction = new QAction("NACA Airfoil Generator");
    m_ContextMenu->addAction(m_NACAAction);
    m_Menu->addAction(m_NACAAction);
    connect(m_NACAAction,SIGNAL(triggered(bool)),this,SLOT(onNACAFoil()));

    m_CircularFoil = new QAction("Circular Airfoil Generator");
    m_ContextMenu->addAction(m_CircularFoil);
    m_Menu->addAction(m_CircularFoil);
    connect(m_CircularFoil,SIGNAL(triggered(bool)),this,SLOT(onCircularFoil()));

    m_interpolateFoils = new QAction("Interpolate Airfoils");
    m_ContextMenu->addAction(m_interpolateFoils);
    m_Menu->addAction(m_interpolateFoils);
    connect(m_interpolateFoils,SIGNAL(triggered(bool)),this,SLOT(onInterpolateFoils()));

    m_transformFoil = new QAction("Virtual Camber Transformation");
    m_ContextMenu->addAction(m_transformFoil);
    m_Menu->addAction(m_transformFoil);
    connect(m_transformFoil,SIGNAL(triggered(bool)),this,SLOT(onTransformFoil()));

}

void FoilModule::onRenameFoil(){

    if(!g_pCurFoil) return;
    g_foilStore.rename(g_pCurFoil,g_pCurFoil->getName());
    g_mainFrame->SetSaveState(false);
}

void FoilModule::onDeleteFoil(){

    if(!g_pCurFoil) return;

    QString strong = tr("Are you sure you want to delete")  +"\n"+ g_pCurFoil->getName() +"\n";
    strong+= tr("and all associated Polars, Blades and Simulations ?");

    int resp = QMessageBox::question(g_mainFrame,tr("Question"), strong,  QMessageBox::Yes | QMessageBox::No);
    if(resp != QMessageBox::Yes) return;

    Airfoil *foil = g_pCurFoil;
    g_pCurFoil = NULL;
    g_foilStore.remove(foil);
    setFoil(NULL);

    g_mainFrame->SetSaveState(false);
}

void FoilModule::setFoil(Airfoil *foil){

    if (!foil && g_foilStore.size()) foil = g_foilStore.at(0);
    g_pCurFoil = foil;
    m_Dock->FillFoilTable();

    for (int i=0;i<g_foilStore.size();i++)
        g_foilStore.at(i)->setHighlight(false);

    if (foil && m_highlightAction->isChecked())
        foil->setHighlight(true);

    if (foil) g_polarModule->m_ToolBar->m_pctrlFoil->setCurrentObject(foil);

}

void FoilModule::onTEGap(){

    QDialog templateDiag(g_mainFrame);
    templateDiag.setWindowTitle("Set T.E. Gap");

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    templateDiag.setSizePolicy(szPolicyExpanding);

    NumberEdit valA;
    valA.setMinimum(0);
    valA.setMaximum(1);
    valA.setAutomaticPrecision(3);
    valA.setValue(0.005);
    QLabel labA("T.E Gap [%c]");


    NumberEdit valB;
    valB.setMinimum(0);
    valB.setMaximum(1);
    valB.setAutomaticPrecision(3);
    valB.setValue(0.8);
    QLabel labB("Blending Distance (from T.E.) [%c]");


    QPushButton ok("Ok");
    connect (&ok,SIGNAL(clicked()), &templateDiag,SLOT(accept()));
    QPushButton cancel("Cancel");
    connect (&cancel,SIGNAL(clicked()), &templateDiag,SLOT(reject()));

    QVBoxLayout layV;
    QHBoxLayout layH;
    QGridLayout grid;

    int gridRow = 0;

    grid.addWidget(&labA,gridRow,0);
    grid.addWidget(&valA,gridRow++,1);

    grid.addWidget(&labB,gridRow,0);
    grid.addWidget(&valB,gridRow++,1);

    layH.addWidget(&ok);
    layH.addWidget(&cancel);

    layV.addLayout(&grid);
    layV.addLayout(&layH);


    templateDiag.setLayout(&layV);

    double tegap;
    double blending;

    if (QDialog::Accepted == templateDiag.exec()){
        tegap = valA.getValue();
        blending = valB.getValue();
    }
    else{
        return;
    }

    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    QString directory = QString(g_tempPath)+QDir::separator();

    QFile XFile(directory+"xfbat.txt");

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Error, cant create xfbat.txt!!";
        return ;
    }

    onExportFoil(directory+"tmpFoil.dat");


    QTextStream out(&XFile);

    out << "PLOP\n";
    out << "G F\n";
    out << "\n";
    out << "LOAD "+directory+"tmpFoil.dat\n";
    out << "GDES\n";
    out << "TGAP\n";
    out << QString().number(tegap,'f',3)+"\n";
    out << QString().number(blending,'f',3)+"\n";
    out << "\n";
    out << "PCOP\n";
    out << "PPAR\n";
    out << "N"+QString().number(g_pCurFoil->n,'f',0)+"\n";
    out << "\n";
    out << "\n";
    out << "PANE\n";
    out << "SAVE "+directory+"tmpFoil.dat\n";
    out << "\n";
    out << "QUIT\n";

    XFile.close();

    BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("XFoil",false);
    progressDialog->setStandardInputFile(directory+"xfbat.txt");
    progressDialog->startProcess(QStringList());

//    int response = progressDialog->exec();
//    if (response == QDialog::Accepted) {
//        qDebug() << "success!!!";
//    }

    progressDialog->getProcess()->waitForFinished(1000);

    if (XFile.exists()){
        XFile.remove();
    }

    QFile FFile(directory+"tmpFoil.dat");

    if (FFile.exists()){
        onImportFoil(directory+"tmpFoil.dat");
        FFile.remove();
    }

}

void FoilModule::onTEFlap(){

    QDialog templateDiag(g_mainFrame);
    templateDiag.setWindowTitle("Set T.E. Flap");

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    templateDiag.setSizePolicy(szPolicyExpanding);

    NumberEdit valA;
    valA.setMinimum(-90);
    valA.setMaximum(90);
    valA.setAutomaticPrecision(1);
    valA.setValue(10);
    QLabel labA("Flap Angle (+ is down) [deg]");


    NumberEdit valB;
    valB.setMinimum(0);
    valB.setMaximum(1);
    valB.setAutomaticPrecision(2);
    valB.setValue(0.8);
    QLabel labB("Hinge x-Position [%c]");

    NumberEdit valC;
    valC.setMinimum(0);
    valC.setMaximum(1);
    valC.setAutomaticPrecision(2);
    valC.setValue(0.5);
    QLabel labC("Hinge y-Position [%t]");

    QPushButton ok("Ok");
    connect (&ok,SIGNAL(clicked()), &templateDiag,SLOT(accept()));
    QPushButton cancel("Cancel");
    connect (&cancel,SIGNAL(clicked()), &templateDiag,SLOT(reject()));

    QVBoxLayout layV;
    QHBoxLayout layH;
    QGridLayout grid;

    int gridRow = 0;

    grid.addWidget(&labA,gridRow,0);
    grid.addWidget(&valA,gridRow++,1);

    grid.addWidget(&labB,gridRow,0);
    grid.addWidget(&valB,gridRow++,1);

    grid.addWidget(&labC,gridRow,0);
    grid.addWidget(&valC,gridRow++,1);

    layH.addWidget(&ok);
    layH.addWidget(&cancel);

    layV.addLayout(&grid);
    layV.addLayout(&layH);


    templateDiag.setLayout(&layV);

    double angle;
    double xpos;
    double ypos;

    if (QDialog::Accepted == templateDiag.exec()){
        angle = valA.getValue();
        xpos = valB.getValue();
        ypos = valC.getValue();
    }
    else{
        return;
    }

    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    QString directory = QString(g_tempPath)+QDir::separator();


    QFile XFile(directory+"xfbat.txt");

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Error, cant create xfbat.txt!!";
        return ;
    }

    onExportFoil(directory+"tmpFoil.dat");


    QTextStream out(&XFile);

    out << "PLOP\n";
    out << "G F\n";
    out << "\n";
    out << "LOAD "+directory+"tmpFoil.dat\n";
    out << "GDES\n";
    out << "FLAP\n";
    out << QString().number(xpos,'f',3)+"\n";
    out << "999\n";
    out << QString().number(ypos,'f',3)+"\n";
    out << QString().number(angle,'f',3)+"\n";
    out << "\n";
    out << "PCOP\n";
    out << "PPAR\n";
    out << "N"+QString().number(g_pCurFoil->n,'f',0)+"\n";
    out << "\n";
    out << "\n";
    out << "PANE\n";
    out << "SAVE "+directory+"tmpFoil.dat\n";
    out << "\n";
    out << "QUIT\n";

    XFile.close();

    BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("XFoil",false);
    progressDialog->setStandardInputFile(directory+"xfbat.txt");
    progressDialog->startProcess(QStringList());

//    int response = progressDialog->exec();
//    if (response == QDialog::Accepted) {
//        qDebug() << "success!!!";
//    }

    progressDialog->getProcess()->waitForFinished(1000);

    if (XFile.exists()){
        XFile.remove();
    }

    QFile FFile(directory+"tmpFoil.dat");

    if (FFile.exists()){
        onImportFoil(directory+"tmpFoil.dat");
        FFile.remove();
    }

}

void FoilModule::onImportFoil(QString PathName){


    if (!PathName.size()) PathName = QFileDialog::getOpenFileName(g_mainFrame, tr("Open Foil File"),
                                            g_mainFrame->m_LastDirName,
                                            "Airfoil File (*.*)");
    if(!PathName.length()) return;
    int pos = PathName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = PathName.left(pos);

    Airfoil *pFoil = ImportAirfoil(PathName);

    if(pFoil)
    {
        g_foilModule->storeAirfoil(pFoil);
        g_mainFrame->SetSaveState(false);
    }
    if (!g_pCurFoil) setFoil(NULL);

}

void FoilModule::onExportFoil(QString FileName, Airfoil *foil){

    if (!foil) foil = g_pCurFoil;

    ExportAirfoil(foil, FileName);
}

void FoilModule::onNormalizeFoil(){

    if(!g_pCurFoil) return;

    Airfoil *newFoil = new Airfoil();
    newFoil->CopyFoil(g_pCurFoil);
    newFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));

    newFoil->NormalizeGeometry();
    newFoil->InitFoil();

    g_foilModule->storeAirfoil(newFoil);

}

void FoilModule::onInterpolateFoils(){

    if (!g_pCurFoil) return;

    QDialog interDiag(g_mainFrame);
    interDiag.setWindowTitle("Airfoil Interpolation");

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);
    interDiag.setSizePolicy(szPolicyExpanding);

    FoilComboBox foilBox1(&g_foilStore);
    QLabel labBox1("Airfoil A");

    FoilComboBox foilBox2(&g_foilStore);
    QLabel labBox2("Airfoil B");

    NumberEdit FRACTION;
    FRACTION.setMinimum(0);
    FRACTION.setMaximum(1);
    FRACTION.setAutomaticPrecision(2);
    FRACTION.setValue(0.5);
    QLabel labFRACTION("Interpolation");

    QPushButton ok("Ok");
    connect (&ok,SIGNAL(clicked()), &interDiag,SLOT(accept()));
    QPushButton cancel("Cancel");
    connect (&cancel,SIGNAL(clicked()), &interDiag,SLOT(reject()));

    QVBoxLayout layV;
    QHBoxLayout layH;
    QGridLayout grid;

    int gridRow = 0;

    grid.addWidget(&labBox1,gridRow,0);
    grid.addWidget(&foilBox1,gridRow++,1);

    grid.addWidget(&labBox2,gridRow,0);
    grid.addWidget(&foilBox2,gridRow++,1);

    grid.addWidget(&labFRACTION,gridRow,0);
    grid.addWidget(&FRACTION,gridRow++,1);

    layH.addWidget(&ok);
    layH.addWidget(&cancel);

    layV.addLayout(&grid);
    layV.addLayout(&layH);

    interDiag.setLayout(&layV);

    if (QDialog::Accepted != interDiag.exec())
        return;

    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    QString directory = QString(g_tempPath)+QDir::separator();

    onExportFoil(directory+"tmpFoilA.dat",foilBox1.currentObject());
    onExportFoil(directory+"tmpFoilB.dat",foilBox2.currentObject());

    QFile XFile(directory+"xfbat.txt");

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Error, cant create xfbat.txt!!";
        return ;
    }

    QTextStream out(&XFile);

    out << "PLOP\n";
    out << "G F\n";
    out << "\n";
    out << "LOAD "+directory+"tmpFoilA.dat\n";
    out << "INTE\n";
    out << "C\n";
    out << "F\n";
    out << directory+"tmpFoilB.dat\n";
    out << QString().number(FRACTION.getValue(),'f',2)+"\n";
    out << "Interpolated\n";
    out << "PCOP\n";
    out << "PPAR\n";
    out << "N"+QString().number(foilBox1.currentObject()->n*0.5+foilBox2.currentObject()->n*0.5,'f',0)+"\n";
    out << "\n";
    out << "\n";
    out << "PANE\n";
    out << "SAVE "+directory+"tmpFoil.dat\n";
    out << "\n";
    out << "QUIT\n";

    XFile.close();

    BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("XFoil",false);
    progressDialog->setStandardInputFile(directory+"xfbat.txt");
    progressDialog->startProcess(QStringList());

//    int response = progressDialog->exec();
//    if (response == QDialog::Accepted) {
//        qDebug() << "success!!!";
//    }

    progressDialog->getProcess()->waitForFinished(1000);

    if (XFile.exists())
        XFile.remove();

    QFile FFileA(directory+"tmpFoilA.dat");
    if (FFileA.exists())
        FFileA.remove();


    QFile FFileB(directory+"tmpFoilB.dat");
    if (FFileB.exists())
        FFileB.remove();

    QFile FFile(directory+"tmpFoil.dat");
    if (FFile.exists()){
        onImportFoil(directory+"tmpFoil.dat");
        FFile.remove();
    }

}

void FoilModule::onDerotateFoil(){

    if(!g_pCurFoil) return;

    Airfoil *newFoil = new Airfoil();
    newFoil->CopyFoil(g_pCurFoil);
    newFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));

    newFoil->DeRotate();
    newFoil->InitFoil();

    g_foilModule->storeAirfoil(newFoil);

}

void FoilModule::onRefineGlobally(){

    if (!g_pCurFoil) return;


    QDialog nacaDiag(g_mainFrame);
    nacaDiag.setWindowTitle("Global Repaneling");

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    nacaDiag.setSizePolicy(szPolicyExpanding);


    NumberEdit PANELS;
    PANELS.setMinimum(10);
    PANELS.setMaximum(290);
    PANELS.setAutomaticPrecision(0);
    PANELS.setValue(200);
    QLabel labPANELS("Number of Panels");


    QPushButton ok("Ok");
    connect (&ok,SIGNAL(clicked()), &nacaDiag,SLOT(accept()));
    QPushButton cancel("Cancel");
    connect (&cancel,SIGNAL(clicked()), &nacaDiag,SLOT(reject()));

    QVBoxLayout layV;
    QHBoxLayout layH;
    QGridLayout grid;

    int gridRow = 0;

    grid.addWidget(&labPANELS, gridRow, 0);
    grid.addWidget(&PANELS, gridRow++, 1);

    layH.addWidget(&ok);
    layH.addWidget(&cancel);

    layV.addLayout(&grid);
    layV.addLayout(&layH);


    nacaDiag.setLayout(&layV);

    int panels;

    if (QDialog::Accepted == nacaDiag.exec()){
        panels = PANELS.getValue()+1;
    }
    else{
        return;
    }

    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    QString directory = QString(g_tempPath)+QDir::separator();

    if (panels%2 == 0) panels+=1;


    QFile XFile(directory+"xfbat.txt");

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Error, cant create xfbat.txt!!";
        return ;
    }

    onExportFoil(directory+"tmpFoil.dat");

    QTextStream out(&XFile);

    out << "PLOP\n";
    out << "G F\n";
    out << "\n";
    out << "LOAD "+directory+"tmpFoil.dat\n";
    out << "PPAR\n";
    out << "N"+QString().number(panels,'f',0)+"\n";
    out << "\n";
    out << "\n";
    out << "PANE\n";
    out << "SAVE "+directory+"tmpFoil.dat\n";
    out << "\n";
    out << "QUIT\n";

    XFile.close();

    BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("XFoil",false);
    progressDialog->setStandardInputFile(directory+"xfbat.txt");
    progressDialog->startProcess(QStringList());

//    int response = progressDialog->exec();
//    if (response == QDialog::Accepted) {
//        qDebug() << "success!!!";
//    }

    progressDialog->getProcess()->waitForFinished(1000);

    if (XFile.exists()){
        XFile.remove();
    }

    QFile FFile(directory+"tmpFoil.dat");

    if (FFile.exists()){
        onImportFoil(directory+"tmpFoil.dat");
        FFile.remove();
    }

    QFile EFile(directory+"tmpFoil.dat");
    if (EFile.exists()){
        EFile.remove();
    }


}

void FoilModule::onCircularFoil(){

    CircularFoilDlg pCircularFoilDlg;

    if (pCircularFoilDlg.exec())
        g_qbem->LoadCylindricalFoil(pCircularFoilDlg.m_CircleName->text(), pCircularFoilDlg.m_CircularDrag->getValue(),pCircularFoilDlg.m_Reynolds->getValue());

}

void FoilModule::onNACAFoil(){


    QDialog nacaDiag(g_mainFrame);
    nacaDiag.setWindowTitle("NACA 4 Digit Airfoil Generator");

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);


    nacaDiag.setSizePolicy(szPolicyExpanding);

    NumberEdit NACA;
    NACA.setMinimum(0);
    NACA.setMaximum(9999);
    NACA.setAutomaticPrecision(0);
    NACA.setValue(20);
    QLabel labNACA("NACA 4-DIGIT");


    NumberEdit PANELS;
    PANELS.setMinimum(10);
    PANELS.setMaximum(290);
    PANELS.setAutomaticPrecision(0);
    PANELS.setValue(200);
    QLabel labPANELS("Number of Panels");


    QPushButton ok("Ok");
    connect (&ok,SIGNAL(clicked()), &nacaDiag,SLOT(accept()));
    QPushButton cancel("Cancel");
    connect (&cancel,SIGNAL(clicked()), &nacaDiag,SLOT(reject()));

    QVBoxLayout layV;
    QHBoxLayout layH;
    QGridLayout grid;

    int gridRow = 0;

    grid.addWidget(&labNACA,gridRow,0);
    grid.addWidget(&NACA,gridRow++,1);

    grid.addWidget(&labPANELS,gridRow,0);
    grid.addWidget(&PANELS,gridRow++,1);

    layH.addWidget(&ok);
    layH.addWidget(&cancel);

    layV.addLayout(&grid);
    layV.addLayout(&layH);


    nacaDiag.setLayout(&layV);

    int naca;
    int panels;

    if (QDialog::Accepted == nacaDiag.exec()){
        naca = NACA.getValue();
        panels = PANELS.getValue()+1;
    }
    else{
        return;
    }

    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    QString directory = QString(g_tempPath)+QDir::separator();

    if (panels%2 == 0) panels+=1;

    QFile XFile(directory+"xfbat.txt");

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "Error, cant create xfbat.txt!!";
        return ;
    }

    QTextStream out(&XFile);

    out << "PLOP\n";
    out << "G F\n";
    out << "\n";
    out << "NACA "+QString().number(naca,'f',0)+"\n";
    out << "PPAR\n";
    out << "N"+QString().number(panels,'f',0)+"\n";
    out << "\n";
    out << "\n";
    out << "PANE\n";
    out << "SAVE "+directory+"tmpFoil.dat\n";
    out << "\n";
    out << "QUIT\n";

    XFile.close();

    BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("XFoil",false);
    progressDialog->setStandardInputFile(directory+"xfbat.txt");
    progressDialog->startProcess(QStringList());

//    int response = progressDialog->exec();
//    if (response == QDialog::Accepted) {
//        qDebug() << "success!!!";
//    }

    progressDialog->getProcess()->waitForFinished(1000);

    if (XFile.exists()){
        XFile.remove();
    }

    QFile FFile(directory+"tmpFoil.dat");

    if (FFile.exists()){
        onImportFoil(directory+"tmpFoil.dat");
        FFile.remove();
    }

}

void FoilModule::onEditFoil(){

    if(!g_pCurFoil) return;

    m_highlightPoint = 0;
    m_editFoil = new Airfoil(g_pCurFoil->getName());
    m_editFoil->CopyFoil(g_pCurFoil);
    m_editFoil->setDrawPoints(true);

    reloadAllGraphCurves();

    FoilEditDlg dlg;

    dlg.m_pMemFoil    = g_pCurFoil;
    dlg.m_pBufferFoil = m_editFoil;
    dlg.InitDialog();

    if(QDialog::Accepted == dlg.exec())
    {
        if(g_foilModule->storeAirfoil(m_editFoil)){
            g_mainFrame->SetSaveState(false);
            m_editFoil->setDrawPoints(false);
            m_editFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
            setFoil(m_editFoil);
        }
    }
    else
    {
        delete m_editFoil;
    }

    m_editFoil = NULL;
    reloadAllGraphCurves();
}

void FoilModule::onTransformFoil()
{

    if(!g_pCurFoil) return;
    double TSR, epsilon, lamda, cR, mpx, mpy, C, D, G, virtualalpha,chordratio;

    QDialog strDiag;

    strDiag.setWindowTitle("Virtual Camber Transformation");

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    strDiag.setSizePolicy(szPolicyExpanding);

    QLabel labelName;

    labelName.setText("Transformation for: "+g_pCurFoil->getName());

    NumberEdit CR;
    NumberEdit tsr;
    QLabel lab1("Tip Speed ratio");
    QLabel lab2("Chord to Radius Ratio");

    tsr.setMinimum(0.5);
    CR.setMinimum(0.01);

    tsr.setValue(4);
    CR.setValue(0.1);

    QPushButton ok("Ok");
    QPushButton cancel("Cancel");

    connect (&ok,SIGNAL(clicked()), &strDiag,SLOT(accept()));
    connect (&cancel,SIGNAL(clicked()), &strDiag,SLOT(reject()));


    QVBoxLayout layV;
    QHBoxLayout layH;
    QGridLayout grid;

    layH.addWidget(&ok);
    layH.addWidget(&cancel);

    int gridRow = 0;

    grid.addWidget(&lab1,gridRow,0);
    grid.addWidget(&tsr,gridRow++,1);

    grid.addWidget(&lab2,gridRow,0);
    grid.addWidget(&CR,gridRow++,1);

    layV.addWidget(&labelName);
    layV.addLayout(&grid);
    layV.addLayout(&layH);

    strDiag.setLayout(&layV);

    cR = CR.getValue();
    TSR = tsr.getValue();

    if (QDialog::Accepted != strDiag.exec())
        return;


    Airfoil *pNewFoil = new Airfoil();
    pNewFoil->CopyFoil(g_pCurFoil);
    pNewFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));


    //----------------INPUT BOX-------------------//

    epsilon = 1/TSR;
    lamda = 1;
    mpx= 0.5;
    mpy = 0;
    C = sqrt(pow(epsilon,2.0)+1);
    D = atan(epsilon);

    //---------------------------------------------//

    double  x[IQX],  y[IQX];
    for (int i=0; i<pNewFoil->n; i++){
        x[i] = (pNewFoil->x[i]-mpx);   // 1st transformation, to position the mounting point(mpx,mpy) on LE of non modified profile
        y[i] = (pNewFoil->y[i]-mpy);
        pNewFoil->x[i] = x[i];
        pNewFoil->y[i] = y[i];
    }

    for(int j=0; j<pNewFoil->n; j++){

        x[j] = (pNewFoil->x[j]*cR)+epsilon;     // 2nd transformation to camber the foil using input BC
        y[j] = (pNewFoil->y[j]*cR)+1;

        G = sqrt(pow(x[j],2.0)+pow(y[j],2.0));

        x[j] = G* (atan(x[j]/y[j])-D)/cR;


        // here the orientation of the camber line is switched
        y[j] = /*(-1)**/(G-C)/cR;
    }



    for(int j=0; j<pNewFoil->n; j++){
        // here the ordering of the foil points is switched due to the change in camber line

        pNewFoil->x[j] = x[pNewFoil->n-1-j];
        pNewFoil->y[j] = y[pNewFoil->n-1-j];
    }



    int LE, TE;                              // Leading edge(LE) and Trailing edge(TE) research over the airfoil points
    double small = 1000000;
    double large = -100000;
    for(int j=0; j<pNewFoil->n; j++){
        if (pNewFoil->x[j] < small){
            small = pNewFoil->x[j];
            LE = j;
        }
        if (pNewFoil->x[j] > large){
            large = pNewFoil->x[j];
            TE = j;
        }
    }
    virtualalpha = atan2(y[TE]-y[LE],x[TE]-x[LE]);                   // virtual parameters: virtual alpha, cv/cg(ratio between virtual chord and geometric chord)
    chordratio = sqrt(pow(x[TE]-x[LE],2.0)+pow(y[TE]-y[LE],2.0));

    for(int k=0; k<pNewFoil->n; k++){
        x[k] = ((pNewFoil->x[k]*cos(virtualalpha)+pNewFoil->y[k]*sin(virtualalpha))/chordratio);          // 3rd transformation to align the two coordinate systems
        y[k] = ((pNewFoil->y[k]*cos(virtualalpha)-pNewFoil->x[k]*sin(virtualalpha))/chordratio);

        pNewFoil->x[k] = x[k];
        pNewFoil->y[k] = -y[k];
    }

    pNewFoil->DeRotate();
    pNewFoil->NormalizeGeometry();
    pNewFoil->InitFoil();

    if(g_foilModule->storeAirfoil(pNewFoil))
    {
        setFoil(pNewFoil);
        g_mainFrame->SetSaveState(false);
    }
    if (!g_pCurFoil) setFoil(NULL);

}

Airfoil* FoilModule::storeAirfoil(Airfoil *pNewFoil)
{
    // Adds the buffer foil to the Store,
    // and initializes XFoil, comboboxes and everything.

    if (!g_foilStore.add(pNewFoil)) pNewFoil = NULL;
    else setFoil(pNewFoil);

    if (pNewFoil) pNewFoil->InitFoil();
    g_pCurFoil = pNewFoil;

    g_qbem->UpdateFoils();

    return pNewFoil;// foil added
}

void FoilModule::onDuplicateFoil()
{
    if(!g_pCurFoil) return;
    Airfoil *pNewFoil = new Airfoil();
    pNewFoil->CopyFoil(g_pCurFoil);
    pNewFoil->InitFoil();
    pNewFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
    pNewFoil->setDrawPoints(false);

    if(storeAirfoil(pNewFoil))
    {
        setFoil(pNewFoil);
        g_mainFrame->SetSaveState(false);
    }
}

FoilModule *g_foilModule;
Airfoil *g_pCurFoil;

