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

#include "DebugDialog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QRadioButton>
#include "Globals.h"
#include "MainFrame.h"

void redirectOutputToDialog(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if(MainFrame::s_debugDlg != NULL)
//        if (type == QtDebugMsg)
            QMetaObject::invokeMethod(MainFrame::s_debugDlg->textEdit, "append", Qt::QueuedConnection, Q_ARG(QString, msg));
            //DM could also invoke function thats declated with Q_INVOKABLE
}

void redirectOutputToLogFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if(MainFrame::s_debugDlg != NULL)
//        if (type == QtDebugMsg)
            QMetaObject::invokeMethod(g_mainFrame, "OnWriteLogFile", Qt::QueuedConnection, Q_ARG(QString, msg));
            //DM could also invoke function thats declated with Q_INVOKABLE

}

DebugDialog::DebugDialog()
{

    QVBoxLayout *vBox = new QVBoxLayout;
    textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QPushButton *clearButton = new QPushButton(tr("Clear Output"));

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    m_debugSimulation = new QCheckBox("Simulation Debug Output");
    m_debugSimulation->setChecked(false);
    m_debugTurbine = new QCheckBox("Turbine Debug Output");
    m_debugTurbine->setChecked(false);
    m_debugController = new QCheckBox("Controller Debug Output");
    m_debugController->setChecked(false);
    m_debugStruct = new QCheckBox("Structural Model Debug Output");
    m_debugStruct->setChecked(false);
    m_debugSerializer = new QCheckBox("Serializer and Store Debug Output");
    m_debugSerializer->setChecked(false);

    connect(m_debugSimulation,SIGNAL(clicked()), this, SLOT(OnBoxChecked()));
    connect(m_debugTurbine,SIGNAL(clicked()), this, SLOT(OnBoxChecked()));
    connect(m_debugController,SIGNAL(clicked()), this, SLOT(OnBoxChecked()));
    connect(m_debugStruct,SIGNAL(clicked()), this, SLOT(OnBoxChecked()));
    connect(m_debugSerializer,SIGNAL(clicked()), this, SLOT(OnBoxChecked()));

    int gridRowCount = 0;

    QGridLayout *grid = new QGridLayout();
    grid->addWidget(m_debugSimulation,gridRowCount++,0);
    grid->addWidget(m_debugTurbine,gridRowCount++,0);
    grid->addWidget(m_debugController,gridRowCount++,0);
    grid->addWidget(m_debugStruct,gridRowCount++,0);
    grid->addWidget(m_debugSerializer,gridRowCount++,0);

    QLabel *label = new QLabel (tr("Redirect Debug Output to: "));
    grid->addWidget (label, gridRowCount, 0);
    QHBoxLayout *miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    m_outputLocationGroup = new QButtonGroup(miniHBox);
    QRadioButton *radioButton = new QRadioButton ("LogFile");
    m_outputLocationGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Dialog");
    m_outputLocationGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Console");
    m_outputLocationGroup->addButton(radioButton, 2);
    miniHBox->addWidget(radioButton);
    miniHBox->addStretch();
    connect(m_outputLocationGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnRedirectOutput()));

    setGeometry(this->x()+width/30, this->y()+height/20,width*1/3, height*2/3);

    setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addLayout(grid);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addWidget(clearButton);
    hBox->addStretch();
    hBox->addWidget(closeButton);
    closeButton->setAutoDefault(true);

    connect (closeButton,SIGNAL(clicked()), this,SLOT(hide()));
    connect (clearButton,SIGNAL(clicked()), this,SLOT(ClearEdit()));
}

void DebugDialog::OnRedirectOutput(){

    if (m_outputLocationGroup->button(0)->isChecked())
        qInstallMessageHandler(redirectOutputToLogFile);

    if (m_outputLocationGroup->button(1)->isChecked())
        qInstallMessageHandler(redirectOutputToDialog);

    if (m_outputLocationGroup->button(2)->isChecked())
        qInstallMessageHandler(0);
}

void DebugDialog::OnBoxChecked(){

    debugStruct = m_debugStruct->isChecked();
    debugTurbine = m_debugTurbine->isChecked();
    debugController = m_debugController->isChecked();
    debugSimulation = m_debugSimulation->isChecked();
    debugSerializer = m_debugSerializer->isChecked();
    debugStores = m_debugSerializer->isChecked();
}

void DebugDialog::ClearEdit(){

    textEdit->clear();
}

