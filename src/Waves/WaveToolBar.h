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

#ifndef WAVETOOLBAR_H
#define WAVETOOLBAR_H

#include <QToolBar>
class QSlider;
class QComboBox;

#include "../StoreAssociatedComboBox_include.h"
class WaveModule;
class NumberEdit;


class WaveToolBar : public QToolBar
{
	Q_OBJECT
	
public:
    WaveToolBar (QMainWindow *parent, WaveModule *module);
		
    QAction *TwoDView, *GLView, *DualView, *HideWidgets;

    WaveComboBox *m_waveComboBox;

private:
    WaveModule *m_module;
	
public slots:

};

#endif // WINDFIELDTOOLBAR_H
