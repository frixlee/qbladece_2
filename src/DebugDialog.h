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

#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>


class DebugDialog : public QDialog
{
    Q_OBJECT
public:
    DebugDialog();

    QTextEdit *textEdit;
    QCheckBox *m_debugStruct, *m_debugTurbine, *m_debugSimulation, *m_debugSerializer, *m_debugController;
    QButtonGroup *m_outputLocationGroup;

private slots:

    void OnBoxChecked();
    void ClearEdit();
    void OnRedirectOutput();

};

#endif // DEBUGDIALOG_H
