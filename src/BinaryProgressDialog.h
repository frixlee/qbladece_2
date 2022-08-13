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

#ifndef BINARYPROGRESSDIALOG_H
#define BINARYPROGRESSDIALOG_H


#include <QDialog>
#include <QProcess>
class QTextEdit;
class QPushButton;
class QProcess;


class BinaryProgressDialog : public QDialog
{
    Q_OBJECT

public:
    BinaryProgressDialog(QString binaryName, bool autoClose);
    void startProcess (QStringList arguments);  // throws QString on error
    void killProcess ();
    void setStandardInputFile(QString file){m_inputFile = file;}
    QProcess* getProcess(){return m_binaryProcess;}

private slots:
    void onFinishedButtonClicked ();
    void onCancelButtonClicked ();
    void onNewProcessOutput ();
    void onProcessFinished (int exitCode);

private:
    bool m_bAutoClose;
    QString m_binaryName;
    QString m_inputFile;
    QProcess *m_binaryProcess;
    QTextEdit *m_textEdit;
    QPushButton *m_finishButton, *m_cancelButton;
};


#endif // BINARYPROGRESSDIALOG_H
