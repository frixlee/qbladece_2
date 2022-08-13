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

#ifndef GRAPHOPTIONSDIALOG_H
#define GRAPHOPTIONSDIALOG_H

#include <QDialog>
class QListWidget;
class QLineEdit;
class QCheckBox;
class QSpinBox;

class NewGraph;
class NumberEdit;
class LineStyleButton;
class NewColorButton;


class GraphOptionsDialog : public QDialog
{
	Q_OBJECT
	
public:
	GraphOptionsDialog(NewGraph *graph);
	
private:
	void initView (bool asDefault = false);
	
	NewGraph *m_graph;
    QLineEdit *m_graphTitleEdit, *m_searchX, *m_searchY;
	QListWidget *m_xVariableList, *m_yVariableList;
	NumberEdit *m_xLowLimitEdit, *m_xHighLimitEdit, *m_yLowLimitEdit, *m_yHighLimitEdit;
	NumberEdit *m_xTickSizeEdit, *m_yTickSizeEdit;
    QCheckBox *m_xLogScale, *m_yLogScale, *m_setAllGraphs;
    QCheckBox *m_xLimitsManualCheckBox, *m_yLimitsManualCheckBox;
    QSpinBox *m_windowSizeSMA;
	LineStyleButton *m_xGridStyleButton, *m_yGridStyleButton, *m_mainAxisStyleButton, *m_borderStyleButton;
	NewColorButton *m_backgroundColorButton, *m_tickColorButton, *m_titleColorButton;
	QPushButton *m_tickFontButton, *m_titleFontButton;
	QFont m_tickFont, m_titleFont;

    void keyPressEvent(QKeyEvent *event);
	
private slots:
	void onCancelButtonClicked ();
	void onRestoreButtonClicked ();
	void onApplyButtonClicked ();
	void onOkButtonClicked ();
	void onFontButtonClicked ();  // FontButton is not even implemented...
	void setFontButtonsText (QPushButton *button, QString fontName);
	void onAxisValueChanged ();
	void onLogarithmicChanged ();
    void onSearchUpdated ();
};

#endif // GRAPHOPTIONSDIALOG_H
