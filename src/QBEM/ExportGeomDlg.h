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

#ifndef EXPORTGEOMDLG_H
#define EXPORTGEOMDLG_H

#include <QDialog>
#include "../GUI/NumberEdit.h"
#include <QtWidgets>

class ExportGeomDlg : public QDialog
{
    Q_OBJECT
    friend class QBEM;
public:
    ExportGeomDlg();

private:
    NumberEdit *Chordwise, *Spanwise;
    QPushButton *Export, *Cancel;
};

#endif // EXPORTGEOMDLG_H
