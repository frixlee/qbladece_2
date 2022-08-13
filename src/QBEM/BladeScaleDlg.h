/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#ifndef BLADESCALEDLG_H
#define BLADESCALEDLG_H


#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include "../GUI/NumberEdit.h"


class BladeScaleDlg : public QDialog
{
        Q_OBJECT
	
        friend class MainFrame;
public:
        BladeScaleDlg(void *pParent = NULL);
        void InitDialog(double const &RefSpan, double const &RefChord, double const &RefTwist);
private:
        void SetupLayout();
        void ReadData();
        void SetResults();
        void EnableControls();

private slots:
        void OnOK();
        void OnClickedCheckBox();
        void OnEditingFinished();


public:

        QCheckBox *m_pctrlSpan, *m_pctrlChord, *m_pctrlTwist;
        NumberEdit  *m_pctrlNewSpan, *m_pctrlNewChord, *m_pctrlNewTwist;
        QLabel *m_pctrlRefSpan, *m_pctrlRefChord, *m_pctrlRefTwist;
        QLabel *m_pctrlSpanRatio, *m_pctrlChordRatio, *m_pctrlTwistRatio;
        QLabel *m_pctrlUnit20, *m_pctrlUnit21;

        bool  m_bSpan, m_bChord, m_bTwist;
        double  m_NewChord, m_NewTwist, m_NewSpan;
        double  m_RefChord, m_RefTwist, m_RefSpan;
};




#endif // BLADESCALEDLG_H
