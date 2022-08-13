/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#include "QFEMMenu.h"
#include <QDebug>
#include <QFileDialog>
#include <QMainWindow>

#include "../MainFrame.h"
#include "BladeStructure.h"


QFEMMenu::QFEMMenu(QMainWindow *parent, QFEMModule *module)
    : QMenu (parent)
{
    setTitle (tr("Blade Structure"));
    m_module = module;

    connect (this, SIGNAL(aboutToShow()), SLOT(onAboutToShow()));

    m_writeStructFileQBlade= new QAction(tr("Export the current structural definition into QBLADE Format"), this);
    connect(m_writeStructFileQBlade, SIGNAL(triggered()), this, SLOT(onWriteStructFileQBlade()));
    addAction(m_writeStructFileQBlade);

    m_writeBladeFileFAST= new QAction(tr("Export the current structural definition into FAST BLADE Format"), this);
    connect(m_writeBladeFileFAST, SIGNAL(triggered()), this, SLOT(onWriteBladeFileFAST()));
    addAction(m_writeBladeFileFAST);

    m_writeTowerFileFAST= new QAction(tr("Export the current structural definition into FAST TOWER Format"), this);
    connect(m_writeTowerFileFAST, SIGNAL(triggered()), this, SLOT(onWriteTowerFileFAST()));
    addAction(m_writeTowerFileFAST);
}

void QFEMMenu::onWriteStructFileQBlade(){

    QString FileName, BladeName;
    QString SelectedFilter;
    QFileDialog::Options options;

    BladeName = m_module->getShownBladeStructure()->getName();
    BladeName.replace("/", "_");
    BladeName.replace(" ", "_");
    FileName = QFileDialog::getSaveFileName(this, tr("Export QBlade Blade File"), g_mainFrame->m_LastDirName+QDir::separator()+BladeName,
                                            tr("Structural File (*.str)"),
                                             &SelectedFilter, options);

    if (!FileName.isEmpty()) {
        m_module->getShownBladeStructure()->writeQBladeStructFile(FileName);
    }
}

void QFEMMenu::onWriteBladeFileFAST(){

    QString FileName, BladeName;
    QString SelectedFilter;
    QFileDialog::Options options;

    BladeName = m_module->getShownBladeStructure()->getName();
    BladeName.replace("/", "_");
    BladeName.replace(" ", "_");
    FileName = QFileDialog::getSaveFileName(this, tr("Export FAST Blade File"), g_mainFrame->m_LastDirName+QDir::separator()+BladeName,
                                            tr("Text File (*.dat)"),
                                            &SelectedFilter, options);

    if (!FileName.isEmpty()) {
        m_module->getShownBladeStructure()->writeFASTBladeFile(FileName);
    }
}

void QFEMMenu::onWriteTowerFileFAST(){

    QString FileName, BladeName;
    QString SelectedFilter;
    QFileDialog::Options options;

    BladeName = m_module->getShownBladeStructure()->getName();
    BladeName.replace("/", "_");
    BladeName.replace(" ", "_");
    FileName = QFileDialog::getSaveFileName(this, tr("Export FAST Tower File"), g_mainFrame->m_LastDirName+QDir::separator()+BladeName,
                                            tr("Text File (*.dat)"),
                                            &SelectedFilter, options);

    if (!FileName.isEmpty()) {
        m_module->getShownBladeStructure()->writeFASTTowerFile(FileName);
    }
}

void QFEMMenu::onAboutToShow() {
    m_writeStructFileQBlade->setEnabled(m_module->getShownBladeStructure());
    m_writeBladeFileFAST->setEnabled(m_module->getShownBladeStructure());
    m_writeTowerFileFAST->setEnabled(m_module->getShownBladeStructure());
}
