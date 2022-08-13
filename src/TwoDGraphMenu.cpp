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

#include "TwoDGraphMenu.h"

#include <QDebug>
#include <QMainWindow>

#include "Module.h"

TwoDGraphMenu::TwoDGraphMenu(QMainWindow *parent, TwoDModule *module)
	: QMenu(parent)
{
	m_module = module;
	
	setTitle ("Graph");
	connect (this, SIGNAL(aboutToShow()), SLOT(onAboutToShow()));
	
	m_singleGraphAction = new QAction("Single Graph", this);
	m_singleGraphAction->setCheckable(true);
	connect(m_singleGraphAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
	addAction(m_singleGraphAction);
	m_twoHorizontalGraphsAction = new QAction("Two Graphs Horizontal", this);
	m_twoHorizontalGraphsAction->setCheckable(true);
	connect(m_twoHorizontalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
	addAction(m_twoHorizontalGraphsAction);
	m_twoVerticalGraphsAction = new QAction("Two Graphs Vertical", this);
	m_twoVerticalGraphsAction->setCheckable(true);
	connect(m_twoVerticalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
	addAction(m_twoVerticalGraphsAction);
    m_threeGraphsAction = new QAction("Three Graphs Vertical", this);
	m_threeGraphsAction->setCheckable(true);
	connect(m_threeGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
	addAction(m_threeGraphsAction);
	m_fourGraphsAction = new QAction("Four Graphs", this);
	m_fourGraphsAction->setCheckable(true);
	connect(m_fourGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
	addAction(m_fourGraphsAction);
    m_fourGraphsVerticalAction = new QAction("Four Graphs Vertical", this);
    m_fourGraphsVerticalAction->setCheckable(true);
    connect(m_fourGraphsVerticalAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_fourGraphsVerticalAction);

    m_sixGraphsAction = new QAction("Six Graphs", this);
    m_sixGraphsAction->setCheckable(true);
    connect(m_sixGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_sixGraphsAction);
    m_sixGraphsVerticalAction = new QAction("Six Graphs Vertical", this);
    m_sixGraphsVerticalAction->setCheckable(true);
    connect(m_sixGraphsVerticalAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_sixGraphsVerticalAction);
    m_eightGraphsAction = new QAction("Eight Graphs", this);
    m_eightGraphsAction->setCheckable(true);
    connect(m_eightGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_eightGraphsAction);
    m_eightGraphsVerticalAction = new QAction("Eight Graphs Vertical", this);
    m_eightGraphsVerticalAction->setCheckable(true);
    connect(m_eightGraphsVerticalAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_eightGraphsVerticalAction);
}

void TwoDGraphMenu::onAboutToShow() {
	m_singleGraphAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::Single);
	m_twoHorizontalGraphsAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::Horizontal);
	m_twoVerticalGraphsAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::Vertical);
	m_threeGraphsAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::Vertical3);
	m_fourGraphsAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::Quad);
    m_fourGraphsVerticalAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::QuadVertical);
    m_sixGraphsAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::Six);
    m_eightGraphsAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::Eight);
    m_sixGraphsVerticalAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::SixVertical);
    m_eightGraphsVerticalAction->setChecked(m_module->getGraphArrangement() == TwoDWidgetInterface::EightVertical);

}

void TwoDGraphMenu::onGraphArrangementChanged() {
	if (QObject::sender() == m_singleGraphAction) {
		m_module->setGraphArrangement(TwoDWidgetInterface::Single);
	} else if (QObject::sender() == m_twoHorizontalGraphsAction) {
		m_module->setGraphArrangement(TwoDWidgetInterface::Horizontal);
	} else if (QObject::sender() == m_twoVerticalGraphsAction) {
		m_module->setGraphArrangement(TwoDWidgetInterface::Vertical);
	} else if (QObject::sender() == m_threeGraphsAction) {
		m_module->setGraphArrangement(TwoDWidgetInterface::Vertical3);
	} else if (QObject::sender() == m_fourGraphsAction) {
		m_module->setGraphArrangement(TwoDWidgetInterface::Quad);
    } else if (QObject::sender() == m_fourGraphsVerticalAction) {
        m_module->setGraphArrangement(TwoDWidgetInterface::QuadVertical);
    } else if (QObject::sender() == m_sixGraphsAction) {
        m_module->setGraphArrangement(TwoDWidgetInterface::Six);
    } else if (QObject::sender() == m_sixGraphsVerticalAction) {
        m_module->setGraphArrangement(TwoDWidgetInterface::SixVertical);
    } else if (QObject::sender() == m_eightGraphsAction) {
        m_module->setGraphArrangement(TwoDWidgetInterface::Eight);
    } else if (QObject::sender() == m_eightGraphsVerticalAction) {
        m_module->setGraphArrangement(TwoDWidgetInterface::EightVertical);
    }

}
