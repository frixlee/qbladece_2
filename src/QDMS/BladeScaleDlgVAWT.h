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

#ifndef BLADESCALEDLGVAWT_H
#define BLADESCALEDLGVAWT_H

#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include "../GUI/NumberEdit.h"

class BladeScaleDlgVAWT : public QDialog
{
    Q_OBJECT

    friend class MainFrame;

public:
    BladeScaleDlgVAWT(void *pParent = NULL);
    void InitDialog(double const &RefSpan, double const &RefShift, double const &RefChord, double const &RefOffset, double const &RefTwist);

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

    QCheckBox *m_pctrlSpan, *m_pctrlChord, *m_pctrlOffset, *m_pctrlTwist, *m_pctrlShift;
    NumberEdit  *m_pctrlNewSpan, *m_pctrlNewChord, *m_pctrlNewOffset, *m_pctrlNewTwist, *m_pctrlNewShift;
    QLabel *m_pctrlRefSpan, *m_pctrlRefChord, *m_pctrlRefOffset, *m_pctrlRefTwist, *m_pctrlRefShift;
    QLabel *m_pctrlSpanRatio, *m_pctrlChordRatio, *m_pctrlOffsetRatio, *m_pctrlShiftRatio;
    QLabel *m_pctrlUnit20, *m_pctrlUnit21, *m_pctrlUnit22, *m_pctrlUnit23;

    bool  m_bSpan, m_bChord, m_bOffset, m_bTwist, m_bShift;
    double  m_NewChord, m_NewTwist, m_NewOffset, m_NewSpan, m_NewShift;
    double  m_RefChord, m_RefTwist, m_RefOffset, m_RefSpan, m_RefShift;

};

#endif // BLADESCALEDLGVAWT_H
