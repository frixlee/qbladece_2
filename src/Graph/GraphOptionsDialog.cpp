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


#include "GraphOptionsDialog.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QColorDialog>
#include <QCheckBox>
#include <QFontDialog>
#include <QKeyEvent>
#include <QApplication>
#include <QSpinBox>
#include <qclipboard.h>

#include "NewGraph.h"
#include "ShowAsGraphInterface.h"
#include "src/TwoDWidgetInterface.h"
#include "../MainFrame.h"
#include "../TwoDWidget.h"
#include "../GUI/NumberEdit.h"
#include "../GUI/LineStyleButton.h"
#include "../GUI/NewColorButton.h"
#include "../src/Globals.h"


GraphOptionsDialog::GraphOptionsDialog(NewGraph *graph) {
	m_graph = graph;
	
    setWindowTitle("Graph Settings: "+m_graph->getNameInSettings());
	
	QVBoxLayout *dialogVBox = new QVBoxLayout ();
	setLayout(dialogVBox);
		QTabWidget *tabWidget = new QTabWidget ();
		dialogVBox->addWidget(tabWidget);
		QHBoxLayout *hBox = new QHBoxLayout ();
		dialogVBox->addLayout(hBox);
			hBox->addStretch();
			QPushButton *button = new QPushButton (tr("Restore default Style"));
			connect(button, SIGNAL(clicked(bool)), this, SLOT(onRestoreButtonClicked()));
			hBox->addWidget (button);
			button = new QPushButton (tr("Apply"));
			connect(button, SIGNAL(clicked(bool)), this, SLOT(onApplyButtonClicked()));
			hBox->addWidget (button);
			button = new QPushButton (tr("Cancel"));
			connect(button, SIGNAL(clicked(bool)), this, SLOT(onCancelButtonClicked()));
			hBox->addWidget (button);
			button = new QPushButton (tr("Ok"));
			button->setDefault(true);
			connect(button, SIGNAL(clicked(bool)), this, SLOT(onOkButtonClicked()));
			hBox->addWidget (button);
			
	/* the variables tab */
	QWidget *widget = new QWidget ();
	tabWidget->addTab(widget, "Variables");
		QGridLayout *grid = new QGridLayout ();
		widget->setLayout(grid);
			hBox = new QHBoxLayout ();
			grid->addLayout(hBox, 0, 0, 1, 2);
				QLabel *label = new QLabel (tr("Title of graph: "));
				hBox->addWidget(label);
				m_graphTitleEdit = new QLineEdit ();
				hBox-> addWidget(m_graphTitleEdit);

			label = new QLabel (tr("y-Axis"));
            grid->addWidget(label, 1, 0, 1, 1, Qt::AlignHCenter);
			label = new QLabel (tr("x-Axis"));
            grid->addWidget(label, 1, 1, 1, 1, Qt::AlignHCenter);

            hBox = new QHBoxLayout ();
            grid->addLayout(hBox, 2, 0, 1, 2);
                label = new QLabel (tr("Search: "));
                hBox->addWidget(label);
                m_searchY = new QLineEdit ();
                hBox-> addWidget(m_searchY);
                label = new QLabel (tr("Search: "));
                hBox->addWidget(label);
                m_searchX = new QLineEdit ();
                hBox-> addWidget(m_searchX);

                connect(m_searchX, SIGNAL(textChanged(QString)), this, SLOT(onSearchUpdated()));
                connect(m_searchY, SIGNAL(textChanged(QString)), this, SLOT(onSearchUpdated()));

			m_yVariableList = new QListWidget ();
			connect(m_yVariableList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onOkButtonClicked()));
            grid->addWidget(m_yVariableList, 3, 0);
			m_xVariableList = new QListWidget ();
			connect(m_xVariableList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onOkButtonClicked()));
            grid->addWidget(m_xVariableList, 3, 1);
			
	/* the styles and axes tab */
	widget = new QWidget ();
	tabWidget->addTab(widget, "Styles and Axes");
		hBox = new QHBoxLayout ();
		widget->setLayout(hBox);
			QGroupBox *groupBox = new QGroupBox (tr("Axes"));
			groupBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			hBox->addWidget(groupBox);
				grid = new QGridLayout ();
				groupBox->setLayout(grid);
					int gridRowCount = 0;
					grid->setVerticalSpacing(10);
					grid->setHorizontalSpacing(10);
					label = new QLabel(tr("y-Axis"));
					grid->addWidget(label, gridRowCount, 1, 1, 1, Qt::AlignHCenter);
					label = new QLabel(tr("x-Axis"));
					grid->addWidget(label, gridRowCount++, 2, 1, 1, Qt::AlignHCenter);
					label = new QLabel (tr("Scale:"));
					grid->addWidget(label, gridRowCount, 0);
					m_yLogScale = new QCheckBox ("logarithmic");
					connect(m_yLogScale, SIGNAL(toggled(bool)), this, SLOT(onLogarithmicChanged()));
					grid->addWidget(m_yLogScale, gridRowCount, 1);
					m_xLogScale = new QCheckBox ("logarithmic");
					connect(m_xLogScale, SIGNAL(toggled(bool)), this, SLOT(onLogarithmicChanged()));
					grid->addWidget(m_xLogScale, gridRowCount++, 2);
					label = new QLabel (tr("Automatic:"));
					grid->addWidget(label, gridRowCount, 0);
					QHBoxLayout *autoHBox = new QHBoxLayout ();
					grid->addLayout(autoHBox, gridRowCount, 1);
						m_yLimitsManualCheckBox = new QCheckBox (tr("limits"));
						autoHBox->addWidget(m_yLimitsManualCheckBox);
					autoHBox = new QHBoxLayout ();
					grid->addLayout(autoHBox, gridRowCount++, 2);
						m_xLimitsManualCheckBox = new QCheckBox (tr("limits"));
						autoHBox->addWidget(m_xLimitsManualCheckBox);
					label = new QLabel (tr("Low limit:"));
					grid->addWidget(label, gridRowCount, 0);
					m_yLowLimitEdit = new NumberEdit (NumberEdit::Scientific, 6);
					connect(m_yLowLimitEdit, SIGNAL(valueChanged(double)), this, SLOT(onAxisValueChanged()));
					grid->addWidget(m_yLowLimitEdit, gridRowCount, 1);
					m_xLowLimitEdit = new NumberEdit (NumberEdit::Scientific, 6);
					connect(m_xLowLimitEdit, SIGNAL(valueChanged(double)), this, SLOT(onAxisValueChanged()));
					grid->addWidget(m_xLowLimitEdit, gridRowCount++, 2);
					label = new QLabel (tr("High limit:"));
					grid->addWidget(label, gridRowCount, 0);
					m_yHighLimitEdit = new NumberEdit (NumberEdit::Scientific, 6);
					connect(m_yHighLimitEdit, SIGNAL(valueChanged(double)), this, SLOT(onAxisValueChanged()));
					grid->addWidget(m_yHighLimitEdit, gridRowCount, 1);
					m_xHighLimitEdit = new NumberEdit (NumberEdit::Scientific, 6);
					connect(m_xHighLimitEdit, SIGNAL(valueChanged(double)), this, SLOT(onAxisValueChanged()));
					grid->addWidget(m_xHighLimitEdit, gridRowCount++, 2);
                    label = new QLabel (tr("Number of ticks:"));
					grid->addWidget(label, gridRowCount, 0);
                    m_yTickSizeEdit = new NumberEdit (NumberEdit::Standard, 0, 1);
					connect(m_yTickSizeEdit, SIGNAL(valueChanged(double)), this, SLOT(onAxisValueChanged()));
					grid->addWidget(m_yTickSizeEdit, gridRowCount, 1);
                    m_xTickSizeEdit = new NumberEdit (NumberEdit::Standard, 0, 1);
					connect(m_xTickSizeEdit, SIGNAL(valueChanged(double)), this, SLOT(onAxisValueChanged()));
					grid->addWidget(m_xTickSizeEdit, gridRowCount++, 2);
					label = new QLabel (tr("Grid style:"));
					grid->addWidget(label, gridRowCount, 0);
					m_yGridStyleButton = new LineStyleButton ();
					grid->addWidget(m_yGridStyleButton, gridRowCount, 1);
					m_xGridStyleButton = new LineStyleButton ();
					grid->addWidget(m_xGridStyleButton, gridRowCount++, 2);
					label = new QLabel (tr("Axis style:"));
					grid->addWidget(label, gridRowCount, 0);
					m_mainAxisStyleButton = new LineStyleButton ();
                    grid->addWidget(m_mainAxisStyleButton, gridRowCount++, 1, 1, 2);

                    label = new QLabel (tr("Y-Axis SMA Window:"));
                    grid->addWidget(label, gridRowCount, 0, 1, 2);
                    m_windowSizeSMA = new QSpinBox ();
                    m_windowSizeSMA->setMinimum(1);
                    m_windowSizeSMA->setSingleStep(1);
                    m_windowSizeSMA->setMaximum(100000);
                    grid->addWidget(m_windowSizeSMA, gridRowCount++, 2);

					setTabOrder(m_yLimitsManualCheckBox, m_yLowLimitEdit);
					setTabOrder(m_yLowLimitEdit, m_yHighLimitEdit);
					setTabOrder(m_yHighLimitEdit, m_yTickSizeEdit);
					setTabOrder(m_yTickSizeEdit, m_xLimitsManualCheckBox);
					setTabOrder(m_xLimitsManualCheckBox, m_xLowLimitEdit);
					setTabOrder(m_xLowLimitEdit, m_xHighLimitEdit);
					setTabOrder(m_xHighLimitEdit, m_xTickSizeEdit);
			groupBox = new QGroupBox (tr("Styles"));
			groupBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			hBox->addWidget(groupBox);
				grid = new QGridLayout ();
				grid->setVerticalSpacing(10);
				grid->setHorizontalSpacing(10);
//				grid->setColumnMinimumWidth(1, 100);
				groupBox->setLayout(grid);
					gridRowCount = 0;
					label = new QLabel (tr("Border style:"));
					grid->addWidget(label, gridRowCount, 0);
					m_borderStyleButton = new LineStyleButton (false, true, true);
					grid->addWidget(m_borderStyleButton, gridRowCount++, 1);
					label = new QLabel (tr("Background:"));
					grid->addWidget(label, gridRowCount, 0);
					m_backgroundColorButton = new NewColorButton ();
					grid->addWidget(m_backgroundColorButton, gridRowCount++, 1);
                    label = new QLabel (tr("Label font:"));
                    grid->addWidget(label, gridRowCount, 0);
                    m_titleFontButton = new QPushButton ();
					m_titleFontButton->setFixedWidth (120);
					connect(m_titleFontButton, SIGNAL(clicked()), this, SLOT(onFontButtonClicked()));
                    grid->addWidget(m_titleFontButton, gridRowCount++, 1);
                    label = new QLabel (tr("Label color:"));
                    grid->addWidget(label, gridRowCount, 0);
                    m_titleColorButton = new NewColorButton ();
                    grid->addWidget(m_titleColorButton, gridRowCount++, 1);
                    label = new QLabel (tr("Tick font:"));
					grid->addWidget(label, gridRowCount, 0);
					m_tickFontButton = new QPushButton ();
					m_tickFontButton->setFixedWidth (120);
					connect(m_tickFontButton, SIGNAL(clicked()), this, SLOT(onFontButtonClicked()));
					grid->addWidget(m_tickFontButton, gridRowCount++, 1);
                    label = new QLabel (tr("Tick color:"));
					grid->addWidget(label, gridRowCount, 0);
					m_tickColorButton = new NewColorButton ();
					grid->addWidget(m_tickColorButton, gridRowCount++, 1);
                    label = new QLabel (tr("Apply Style to All:"));
                    grid->addWidget(label, gridRowCount, 0);
                    m_setAllGraphs = new QCheckBox ();
                    grid->addWidget(m_setAllGraphs, gridRowCount++, 1);

			hBox->addStretch();
		
	initView();
}

void GraphOptionsDialog::onSearchUpdated(){
    initView();
}

void GraphOptionsDialog::keyPressEvent(QKeyEvent *event){

     if (event->matches(QKeySequence::Copy)){
        if (m_xVariableList->isSelectionRectVisible())
            QApplication::clipboard()->setText(m_xVariableList->currentItem()->text());
        if (m_yVariableList->isSelectionRectVisible())
            QApplication::clipboard()->setText(m_yVariableList->currentItem()->text());
     }

     QDialog::keyPressEvent(event);
}

void GraphOptionsDialog::initView(bool asDefault/* = false*/) {
	NewGraph *graphToLoad;
	if (asDefault) {
		graphToLoad = new NewGraph("__default", NULL, {NewGraph::None, "", "", false, false});
	} else {
		graphToLoad = m_graph;
	}
	
	m_backgroundColorButton->setColor(graphToLoad->getBackgroundColor());
	m_titleColorButton->setColor(graphToLoad->getTitleColor());
	m_tickColorButton->setColor(graphToLoad->getTickColor());
	m_titleFont = graphToLoad->getTitleFont();
	setFontButtonsText(m_titleFontButton, m_titleFont.family());
	m_tickFont = graphToLoad->getTickFont();
	setFontButtonsText(m_tickFontButton, m_tickFont.family());
	m_mainAxisStyleButton->setPen(graphToLoad->getMainAxesPen());
	m_xGridStyleButton->setPen(graphToLoad->getXGridPen());
	m_yGridStyleButton->setPen(graphToLoad->getYGridPen());
	m_borderStyleButton->setPen(QPen(graphToLoad->getBorderColor(), graphToLoad->getBorderWidth()));
			
	if (asDefault) {
		delete graphToLoad;
	}	
	
	if (!asDefault) {  // the following parameters are not restored to default

        m_xVariableList->clear();
        m_yVariableList->clear();

        if (m_searchX->text() == ""){
            m_xVariableList->addItems(m_graph->getAvailableVariables(true));
        }

        if (m_searchY->text() == ""){
            m_yVariableList->addItems(m_graph->getAvailableVariables(false));
        }

        if (m_searchX->text() != ""){
            QStringList strings = m_searchX->text().split(" ");
            for (int i=0;i<m_graph->getAvailableVariables(true).size();i++){
                QString strong = m_graph->getAvailableVariables(true).at(i);
                bool add = true;
                for (int j=0;j< strings.size();j++) if (!strong.contains(strings.at(j), Qt::CaseInsensitive)) add = false;
                if (add) m_xVariableList->addItem(m_graph->getAvailableVariables(true).at(i));
            }
        }

        if (m_searchY->text() != ""){
            QStringList strings = m_searchY->text().split(" ");
            for (int i=0;i<m_graph->getAvailableVariables(false).size();i++){
                QString strong = m_graph->getAvailableVariables(false).at(i);
                bool add = true;
                for (int j=0;j< strings.size();j++) if (!strong.contains(strings.at(j), Qt::CaseInsensitive)) add = false;
                if (add) m_yVariableList->addItem(m_graph->getAvailableVariables(false).at(i));
            }
        }

        m_xVariableList->setCurrentRow(m_graph->getAvailableVariables(true).indexOf(m_graph->getShownXVariable()));
        m_yVariableList->setCurrentRow(m_graph->getAvailableVariables(false).indexOf(m_graph->getShownYVariable()));
		
		m_xLogScale->setChecked(m_graph->getXLogarithmic());
		m_yLogScale->setChecked(m_graph->getYLogarithmic());
		m_graphTitleEdit->setText(m_graph->getTitle());
		m_xLowLimitEdit->setValue(m_graph->getXLowLimit());
		m_xHighLimitEdit->setValue(m_graph->getXHighLimit());
        m_xTickSizeEdit->setValue(m_graph->getXTickFactor());
		m_yLowLimitEdit->setValue(m_graph->getYLowLimit());
		m_yHighLimitEdit->setValue(m_graph->getYHighLimit());
        m_yTickSizeEdit->setValue(m_graph->getYTickFactor());
        m_windowSizeSMA->setValue(m_graph->getWindowSize());
		m_xLimitsManualCheckBox->setChecked(true);
		m_yLimitsManualCheckBox->setChecked(true);
	}
}

void GraphOptionsDialog::onCancelButtonClicked() {
	reject();
}

void GraphOptionsDialog::onRestoreButtonClicked() {
	initView(true);
}

void GraphOptionsDialog::onApplyButtonClicked() {

    if (m_setAllGraphs->isChecked()){

        NewGraph *originalGraph = m_graph;

        for (int i=0;i<g_graphList.size();i++){

            m_graph = g_graphList.at(i);

            m_graph->setWindowSize(m_windowSizeSMA->value());

            m_graph->setXTickFactor(m_xTickSizeEdit->getValue());
            m_graph->setYTickFactor(m_yTickSizeEdit->getValue());

            m_graph->setXLogarithmic(m_xLogScale->isChecked());
            m_graph->setYLogarithmic(m_yLogScale->isChecked());

            /* silent error prevention by ignoring invalid values */

            if (m_xLimitsManualCheckBox->isChecked() && m_yLimitsManualCheckBox->isChecked()){
                m_graph->setOptimalLimits(true);
            }
            else{
                if (!m_xLimitsManualCheckBox->isChecked() && m_xLowLimitEdit->getValue() < m_xHighLimitEdit->getValue()) {
                    m_graph->setXLimits(m_xLowLimitEdit->getValue(), m_xHighLimitEdit->getValue());
                } else {
                    m_graph->setOptimalXLimits();
                }

                if (!m_yLimitsManualCheckBox->isChecked() && m_yLowLimitEdit->getValue() < m_yHighLimitEdit->getValue()) {
                    m_graph->setYLimits(m_yLowLimitEdit->getValue(), m_yHighLimitEdit->getValue());
                } else {
                    m_graph->setOptimalYLimits();
                }
            }

            m_graph->setXGridPen(m_xGridStyleButton->getPen());
            m_graph->setYGridPen(m_yGridStyleButton->getPen());
            m_graph->setMainAxesPen(m_mainAxisStyleButton->getPen());
            m_graph->setBorderColor(m_borderStyleButton->getPen().color());
            m_graph->setBorderWidth(m_borderStyleButton->getPen().width());
            m_graph->setBackgroundColor(m_backgroundColorButton->getColor());
            m_graph->setTickColor(m_tickColorButton->getColor());
            m_graph->setTitleColor(m_titleColorButton->getColor());
            m_graph->setTickFont(m_tickFont);
            m_graph->setTitleFont(m_titleFont);

        }
        m_graph = originalGraph;
    }
    else{

        m_graph->setWindowSize(m_windowSizeSMA->value());

        m_graph->setTitle(m_graphTitleEdit->text());
        m_graph->setShownVariables((m_xVariableList->currentItem() == NULL ? "" : m_xVariableList->currentItem()->text()),
                                   (m_yVariableList->currentItem() == NULL ? "" : m_yVariableList->currentItem()->text()));

        m_graph->setXTickFactor(m_xTickSizeEdit->getValue());
        m_graph->setYTickFactor(m_yTickSizeEdit->getValue());

        m_graph->setXLogarithmic(m_xLogScale->isChecked());
        m_graph->setYLogarithmic(m_yLogScale->isChecked());

        /* silent error prevention by ignoring invalid values */
        if (m_xLimitsManualCheckBox->isChecked() && m_yLimitsManualCheckBox->isChecked()){
            m_graph->setOptimalLimits(true);
        }
        else{
            if (!m_xLimitsManualCheckBox->isChecked() && m_xLowLimitEdit->getValue() < m_xHighLimitEdit->getValue()) {
                m_graph->setXLimits(m_xLowLimitEdit->getValue(), m_xHighLimitEdit->getValue());
            } else {
                m_graph->setOptimalXLimits();
            }
            if (!m_yLimitsManualCheckBox->isChecked() && m_yLowLimitEdit->getValue() < m_yHighLimitEdit->getValue()) {
                m_graph->setYLimits(m_yLowLimitEdit->getValue(), m_yHighLimitEdit->getValue());
            } else {
                m_graph->setOptimalYLimits();
            }
        }

        m_graph->setXGridPen(m_xGridStyleButton->getPen());
        m_graph->setYGridPen(m_yGridStyleButton->getPen());
        m_graph->setMainAxesPen(m_mainAxisStyleButton->getPen());
        m_graph->setBorderColor(m_borderStyleButton->getPen().color());
        m_graph->setBorderWidth(m_borderStyleButton->getPen().width());
        m_graph->setBackgroundColor(m_backgroundColorButton->getColor());
        m_graph->setTickColor(m_tickColorButton->getColor());
        m_graph->setTitleColor(m_titleColorButton->getColor());
        m_graph->setTickFont(m_tickFont);
        m_graph->setTitleFont(m_titleFont);
    }

    TwoDWidgetInterface* interface = g_mainFrame->getTwoDWidgetInterface();
    if (interface) interface->update();
    else g_mainFrame->UpdateView();

}

void GraphOptionsDialog::onOkButtonClicked() {
	onApplyButtonClicked();
	accept();
}

void GraphOptionsDialog::onFontButtonClicked() {
	QPushButton *clickedButton = dynamic_cast<QPushButton*> (QObject::sender());
	QFont *font = (clickedButton == m_tickFontButton) ? &m_tickFont : &m_titleFont;
	*font = QFontDialog::getFont(NULL, *font);
	setFontButtonsText(clickedButton, font->family());
}

void GraphOptionsDialog::setFontButtonsText(QPushButton *button, QString fontName) {
	QString buttonText = fontName;
	QFont defaultFont;  // NM the empty constructor provides the default font
	QFontMetrics metrics (defaultFont);
	if (metrics.width(buttonText) > button->width()-20) {
		const int dotsLength = metrics.width("...");
		do {
			buttonText.chop(1);
		} while (metrics.width(buttonText) + dotsLength > button->width()-20);
		buttonText.append("...");
		button->setToolTip(fontName);
	} else {
		button->setToolTip("");
	}
	button->setText(buttonText);
}

void GraphOptionsDialog::onAxisValueChanged() {
	NumberEdit *changedEdit = dynamic_cast<NumberEdit*> (QObject::sender());
	
	if (changedEdit == m_yLowLimitEdit || changedEdit == m_yHighLimitEdit) {
		m_yLimitsManualCheckBox->setChecked(false);		
	} else if (changedEdit == m_xLowLimitEdit || changedEdit == m_xHighLimitEdit) {
		m_xLimitsManualCheckBox->setChecked(false);
    }
}

void GraphOptionsDialog::onLogarithmicChanged() {
	QCheckBox *changedBox = dynamic_cast<QCheckBox*> (QObject::sender());
	
	if (changedBox == m_xLogScale) {
		m_xTickSizeEdit->setEnabled(!changedBox->isChecked());
	} else {
		m_yTickSizeEdit->setEnabled(!changedBox->isChecked());
	}
}
