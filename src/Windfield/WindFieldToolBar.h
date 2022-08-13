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

#ifndef WINDFIELDTOOLBAR_H
#define WINDFIELDTOOLBAR_H

#include <QToolBar>
class QSlider;
class QComboBox;

#include "../StoreAssociatedComboBox_include.h"
class WindFieldModule;
class NumberEdit;


class WindFieldToolBar : public QToolBar
{
	Q_OBJECT
	
public:
	WindFieldToolBar (QMainWindow *parent, WindFieldModule *module);
	
    void useWindField (WindField *newShownWindField);  // will show the current WindFields information

    QAction *TwoDView, *GLView, *DualView, *HideWidgets;

    WindFieldComboBox *m_windFieldComboBox;

private:
	WindFieldModule *m_module;
	WindField *m_shownWindField;  // the currently shown WindField
	
	NumberEdit *m_timestepEdit;
	QSlider *m_timestepSlider;
	
public slots:
	void onSliderChanged (int newValue);  // shows another timestep
	void onTimestepEditEdited ();  // shows another timestep
};

#endif // WINDFIELDTOOLBAR_H
