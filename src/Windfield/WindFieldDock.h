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
#ifndef WINDFIELDDOCK_H
#define WINDFIELDDOCK_H

#include <QButtonGroup>
#include <QRadioButton>
#include "../CreatorDock.h"
#include "../ParameterViewer.h"
#include "src/GUI/CurveButton.h"
#include "src/CurveStyleBox.h"

class WindFieldModule;
class WindField;


class WindFieldDock : public CreatorDock<WindField>, public ParameterViewer<Parameter::Windfield>
{
	Q_OBJECT
	
public:
	WindFieldDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags, WindFieldModule *module);
	
	void setShownObject (WindField *newObject);

    CurveStyleBox *m_curveStyleBox;

    QButtonGroup *m_componentGroup;
    QPushButton *m_perspective, *m_showtext, *m_showLegend;
    QGroupBox *m_vizBox;


    //stuff for the probe locations//
    QGroupBox *m_probeGroup;
    QComboBox *m_probeBox;
    QPushButton *m_addProbe, *m_removeProbe;
    NumberEdit *m_x, *m_y, *m_z;
    QCheckBox *m_addAllFields;
    //end stuff for the probe locations//

    void OnTwoDView();
    void OnGLView();
    void adjustShowCheckBox();


private:
	WindFieldModule *m_module;

private slots:
    void ReportGlChanged();
    void AddProbe();
    void DeleteProbe();
    void onUnitsChanged ();
    void onEditCopyButtonClicked ();
	void onRenameButtonClicked ();
	void onDeleteButtonClicked ();
	void onNewButtonClicked ();
    void onLineButtonClicked();
    void onShowCheckBoxCanged();
    void onShowPointsCheckBoxCanged();
    void onShowCurveCheckBoxCanged();
};

#endif // WINDFIELDDOCK_H
