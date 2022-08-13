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

#include "BinaryProgressDialog.h"

#include <QTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

#include "Globals.h"

BinaryProgressDialog::BinaryProgressDialog(QString binaryName, bool autoClose)
{

    m_bAutoClose = autoClose;
    m_binaryName = binaryName;
    m_inputFile = "";

    setMinimumWidth(750);
    setMinimumHeight(450);

    QVBoxLayout *vBox = new QVBoxLayout ();
        setLayout(vBox);
        m_textEdit = new QTextEdit ();
        m_textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
        m_textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
        vBox->addWidget(m_textEdit);
        QHBoxLayout *hBox = new QHBoxLayout ();
        vBox->addLayout(hBox);
            hBox->addStretch();
            m_cancelButton = new QPushButton ("Cancel");
            connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(onCancelButtonClicked()));
            hBox->addWidget(m_cancelButton);
            m_finishButton = new QPushButton ("Finished");
            m_finishButton->setDefault(true);
            m_finishButton->setEnabled(false);
            connect(m_finishButton, SIGNAL(clicked()), this, SLOT(onFinishedButtonClicked()));
            hBox->addWidget(m_finishButton);
}

void BinaryProgressDialog::startProcess(QStringList arguments) {

    QFile binaryFile (QString(g_applicationDirectory + QDir::separator() + "Binaries" + QDir::separator() + m_binaryName));
    if (! binaryFile.exists()) {
        qDebug() << QString("Can't find binary: " + binaryFile.fileName());
    }
    binaryFile.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser);
    m_binaryProcess = new QProcess (this);

    connect(m_binaryProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(onNewProcessOutput()));
    connect(m_binaryProcess, SIGNAL(finished(int)), this, SLOT(onProcessFinished(int)));

    if(m_inputFile.size()) m_binaryProcess->setStandardInputFile(m_inputFile);
    m_binaryProcess->start(binaryFile.fileName(),arguments);

}

void BinaryProgressDialog::killProcess(){
    m_binaryProcess->kill();
}

void BinaryProgressDialog::onNewProcessOutput() {
    m_textEdit->moveCursor(QTextCursor::End);
    m_textEdit->insertPlainText(QString(m_binaryProcess->readAllStandardOutput()));
}

void BinaryProgressDialog::onProcessFinished(int exitCode) {
    if (m_bAutoClose && exitCode == 0) onFinishedButtonClicked();
    m_textEdit->moveCursor(QTextCursor::End);
    if (exitCode == 0) {  // assuming that TurbSim never returns 0 if there was an error...
        m_finishButton->setEnabled(true);
    }
}

void BinaryProgressDialog::onFinishedButtonClicked() {
    accept();
}

void BinaryProgressDialog::onCancelButtonClicked() {
    reject();
}

