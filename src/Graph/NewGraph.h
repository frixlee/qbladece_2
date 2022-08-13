/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

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

#ifndef NEWGRAPH_H
#define NEWGRAPH_H

#include <QRect>
#include <QFont>
#include <QColor>
#include <QPen>
#include <QMap>
#include <QPair>

#include "Axis.h"
class TwoDWidgetInterface;
class NewCurve;
class ShowAsGraphInterface;


/**
 * @brief The class that visualizes data as a graph
 *
 * There are different types of graphs, that visualize different portions of data. Because some of them plot all data
 * available in the according store, the functionallity is globally implemented in this class. The different types are:
 * TurbineRotor: Used in TurDms and shows a graph for any TurDmsSimulation available in the store.
 * TurbineBlade: Used in TurDms and shows a graph for any windspeed available in the currently shown TurDmsSimulation.
 */

class NewGraph : public QObject
{
	Q_OBJECT	
	
public:
    enum GraphType {None, QFEMSimulation,
                    Noise, WindfieldTimeGraph, WindfieldLegend, WaveTimeGraph, WaveLegend, MultiTimeGraph,
                    MultiBladeGraph, MultiStructTimeGraph, MultiStructBladeGraph, ControllerTimeGraph, FloaterTimeGraph, MultiLegend, QTurbineBladeGraph,
                    QTurbineRadiationIRFGraph, QTurbineDiffractionIRFGraph, QTurbineStrutGraph, QTurbineTowerGraph, SimulationEnsembleGraph, CampbellGraph,
                    QTurbineTorquetubeGraph, QTurbineLegend, QFlightTimeGraph, QFlightWingGraph, QFlightLegend, QSimulationGraph, FoilGraph, PolarGraph,
                    PolarLegend, OpPointGraph, BLPressureGraph, Polar360Graph, Polar360Legend, HAWTBladeGraph, VAWTBladeGraph, HAWTLegend, VAWTLegend, BEMRotorGraph,
                    BEMBladeGraph, TBEMRotorGraph, TBEMBladeGraph, CBEMGraph, BEMLegend, TBEMLegend, DMSRotorGraph, DMSBladeGraph, DMSAziGraph, DMSLegend, TDMSRotorGraph,
                    TDMSBladeGraph, TDMSAziGraph, TDMSLegend, CDMSGraph, PSDGraph, AllDataGraph, PropRotorGraph, PropBladeGraph, CharPropGraph, PROPLegend};

	enum ExportType {HumanReadable, CSV};
	
	struct GraphDefaults { GraphType graphType; QString xTitle, yTitle; bool xLogarithmic, yLogarithmic; };
	
	NewGraph(QString nameInSettings, TwoDWidgetInterface *twoDInterface, const GraphDefaults &defaults);
	~NewGraph();
	
	void setGraphType (GraphType graphType);  // NM TODO most of these setters could be replaced by a friend function
	void setGraphTypeMulti (GraphType graphTypeMulti) { m_graphTypeMulti = graphTypeMulti; }
	void setDrawingArea (QRect drawingArea);
	void setTitle (QString title) { m_title = title; }
	void setShownVariables (QString xVariable, QString yVariable);
	void setXLogarithmic (bool logarithmic) { m_xAxis.setLogarithmic(logarithmic); }
	void setYLogarithmic (bool logarithmic) { m_yAxis.setLogarithmic(logarithmic); }
	void setBackgroundColor (QColor color) { m_backgroundColor = color; }
	void setBorderColor (QColor color) { m_borderColor = color; }
	void setTickFont (QFont font) { m_tickFont = font; calculateGraphArea(); }
	void setTitleFont (QFont font) { m_titleFont = font; calculateGraphArea(); }
	void setMainAxesPen (QPen pen) { m_mainAxesPen = pen; }
	void setXGridPen (QPen pen) { m_xGridPen = pen; }
	void setYGridPen (QPen pen) { m_yGridPen = pen; }
	void setTitleColor (QColor color) { m_titleColor = color; }
	void setTickColor (QColor color) { m_tickColor = color; }
	void setBorderWidth (int width) { m_borderWidth = width + m_borderGapWidth; }
	void setXLimits (double lowLimit, double highLimit)	{ m_xAxis.setLimits(lowLimit, highLimit); }
	void setYLimits (double lowLimit, double highLimit) { m_yAxis.setLimits(lowLimit, highLimit); }
	void setXTickSize (double tickSize) { m_xAxis.setTickSize(tickSize); }
	void setYTickSize (double tickSize) { m_yAxis.setTickSize(tickSize); }
    void setNoAutoResize (bool autoResize) { m_noAutoResize = autoResize; }
    void setMainVar(int main){m_mainVar = main; }
    void setParam(int param){m_param = param; }
    void setWindowSize(int param){m_windowSizeSMA = param; }

    //for compatibility with BEM/DMS modules when NOT using TwoDWidgetInterface
    void reloadCurves(QList<NewCurve*> &curves);
    void addCurve(NewCurve *curve);
    void setAvaliableGraphVariables(QStringList &avaliableGraphVariables){ m_avaliableGraphVariables = avaliableGraphVariables; }
    void setHighlightDot(QPair<ShowAsGraphInterface*,int> &highlight){m_highlightDot = highlight; }
    void setHighlightIndex(int highlightIndex){m_highlightIndex = highlightIndex; }
    QStringList m_avaliableGraphVariables;
    QPair<ShowAsGraphInterface*,int> m_highlightDot;
    int m_highlightIndex;
    // end compatibility fix

	GraphType getGraphType () { return m_graphType; }
	GraphType getGraphTypeMulti () { return m_graphTypeMulti; }
	QStringList getAvailableVariables (bool xAxis);
	QString getTitle () { return m_title; }
	QString getShownXVariable () { return m_xAxisTitle; }
	QString getShownYVariable () { return m_yAxisTitle; }
	bool getXLogarithmic () { return m_xAxis.getLogarithmic(); }
	bool getYLogarithmic () { return m_yAxis.getLogarithmic(); }
	QColor getBackgroundColor () { return m_backgroundColor; }
	QColor getBorderColor () { return m_borderColor; }
	QFont getTickFont () { return m_tickFont; }
	QFont getTitleFont () { return m_titleFont; }
	QPen getMainAxesPen () { return m_mainAxesPen; }
	QPen getXGridPen () { return m_xGridPen; }
	QPen getYGridPen () { return m_yGridPen; }
	QColor getTitleColor () { return m_titleColor; }
	QColor getTickColor () { return m_tickColor; }
	int getBorderWidth () { return m_borderWidth - m_borderGapWidth; }
	double getXLowLimit () { return m_xAxis.getLowLimit(); }
	double getXHighLimit () { return m_xAxis.getHighLimit(); }
	double getYLowLimit () { return m_yAxis.getLowLimit(); }
	double getYHighLimit () { return m_yAxis.getHighLimit(); }
	double getXTickSize () { return m_xAxis.getTickSize(); }
	double getYTickSize () { return m_yAxis.getTickSize(); }
    double getXTickFactor () { return m_xAxis.getTickSizeFactor(); }
    double getYTickFactor () { return m_yAxis.getTickSizeFactor(); }
    void setXTickFactor (double factor) { m_xAxis.setTickSizeFactor(factor); m_xFactor = factor; }
    void setYTickFactor (double factor) { m_yAxis.setTickSizeFactor(factor); m_yFactor = factor; }
    bool getNoAutoResize () { return m_noAutoResize; }
    int getMainVar(){ return m_mainVar; }
    int getParam(){return m_param; }
    int getWindowSize(){return m_windowSizeSMA; }

    QString getNameInSettings(){return m_nameInSettings; }
    QRect* getDrawingArea(){return &m_innerRect; }
    Axis* getXAxis(){return &m_xAxis; }
    Axis* getYAxis(){return &m_yAxis; }

	void drawGraph (QPainter &painter);
    void exportGraph (QString fileName, ExportType type);
	bool contains (QPoint point);
	void translate (QPoint from, QPoint to);
	void zoomX (double zoom);
	void zoomY (double zoom);
	void setOptimalLimits(bool force);
	void setOptimalXLimits();
	void setOptimalYLimits();
	void reloadCurves ();
    void saveStylesToSettings ();

private:
	NewCurve* getCurve(ShowAsGraphInterface *object, int curveIndex = -1);
	void drawGrid (QPainter &painter);
	void drawTicks (QPainter &painter);
	void drawCurves (QPainter &painter);
	void drawTitles (QPainter &painter);
	void calculateGraphArea ();
	int mapXCoordinateToPixel (double xValue);
	int mapYCoordinateToPixel (double yValue);
	double mapPixelToXCoordinate (int xPixel);
	double mapPixelToYCoordinate (int yPixel);
	void removeAllCurves ();
    bool loadStylesFromSettings ();
    void performSMA(QVector<QPointF> &points);

private:
	GraphType m_graphType;
	GraphType m_graphTypeMulti;  // for the MultiModule two graph types are required
	TwoDWidgetInterface *m_twoDInterface;
	QRect m_drawingArea;  // the area for title, tick, grid and graph
    QRect m_innerRect;
	QRect m_graphArea;  // only grid and graph
	QString m_title, m_xAxisTitle, m_yAxisTitle;
	QMap<int, QPair<QString,QString> > m_formerAxisTitles;
	QMap<int, int> m_formerGraphTypeMulti;
	Axis m_xAxis, m_yAxis;
	QList<NewCurve*> m_curves;
	QString m_nameInSettings;
    double m_xFactor, m_yFactor;
    int m_windowSizeSMA;
	
	/* styles */
	QColor m_backgroundColor, m_borderColor, m_titleColor, m_tickColor;
	QFont m_tickFont, m_titleFont;
	QPen m_mainAxesPen, m_xGridPen, m_yGridPen;
	int m_borderWidth;  // the whole border. User set + borderGap
	int m_borderGapWidth;  // a part of the border with the background color. Simply looks better like this
	int m_gapWidth;  // gap between the graphArea and the labels
    bool m_noAutoResize;  // if false the graph never performes the resize action automaticly
    int m_mainVar, m_param;
};

typedef QMap<int, QPair<QString,QString> > GraphTypeMap;  // needed to avoid the comma in the macro
Q_DECLARE_METATYPE(GraphTypeMap)
typedef QMap<int, int> GraphTypeMultiMap;
Q_DECLARE_METATYPE(GraphTypeMultiMap)


#endif // NEWGRAPH_H
