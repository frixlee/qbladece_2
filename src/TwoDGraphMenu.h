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

#ifndef TWODGRAPHMENU_H
#define TWODGRAPHMENU_H

#include <QMenu>

class TwoDModule;
class QMainWindow;


class TwoDGraphMenu : public QMenu
{
	Q_OBJECT
	
public:
	TwoDGraphMenu(QMainWindow *parent, TwoDModule *module);
	
private:
	TwoDModule *m_module;
	QAction *m_singleGraphAction, *m_twoHorizontalGraphsAction, *m_twoVerticalGraphsAction,
            *m_threeGraphsAction, *m_fourGraphsAction, *m_fourGraphsVerticalAction,
            *m_sixGraphsAction, *m_sixGraphsVerticalAction, *m_eightGraphsAction, *m_eightGraphsVerticalAction;
	
private slots:
	void onAboutToShow ();
	void onGraphArrangementChanged ();
};

#endif // TWODGRAPHMENU_H
