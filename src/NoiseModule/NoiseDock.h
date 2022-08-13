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

#ifndef NOISEDOCK_H
#define NOISEDOCK_H

#include "../CreatorTwoDDock.h"
#include "../ParameterViewer.h"
class TwoDModule;
class NoiseModule;


class NoiseDock : public TwoDDock<NoiseSimulation>, public ParameterViewer<Parameter::NoiseSimulation>
{
public:
	NoiseDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags, NoiseModule *module);
	~NoiseDock();
	
	bool isColorByOpPoint();
	
private:
	TwoDModule* module();
	virtual QList<NewGraph::GraphType> affectedGraphTypes();
	
	NoiseModule *m_module;
	QCheckBox *m_colorByOpPoint;
	
private/* slots*/:
	virtual void onUnitsChanged();
	virtual void onEditCopyButtonClicked ();
	virtual void onRenameButtonClicked ();
	virtual void onDeleteButtonClicked ();
	virtual void onNewButtonClicked ();
	virtual void onColorByOpPoint ();
};

#endif // NOISEDOCK_H
