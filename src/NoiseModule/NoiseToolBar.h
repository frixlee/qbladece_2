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

#ifndef NOISETOOLBAR_H
#define NOISETOOLBAR_H

#include <QToolBar>
class QComboBox;

#include "../StoreAssociatedComboBox_include.h"
class NoiseModule;


class NoiseToolBar : public QToolBar
{
public:
	NoiseToolBar(QMainWindow *parent, NoiseModule *module);
	void setShownSimulation (NoiseSimulation *newSimulation);
	int getShownOpPointIndex ();
	
private:
	NoiseModule *m_module;
	NoiseSimulationComboBox *m_simulationComboBox;
	QComboBox *m_opPointComboBox;
};

#endif // NOISETOOLBAR_H
