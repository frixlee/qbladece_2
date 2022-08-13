/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#ifndef DISPLAYSETTINGSDLG_H
#define DISPLAYSETTINGSDLG_H

#include <QDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include "NewColorButton.h"
#include "NumberEdit.h"

class MainSettingsDialog : public QDialog
{
	Q_OBJECT

	friend class MainFrame;
public:
    MainSettingsDialog();
	void InitDialog();

private slots:
	void OnStyleChanged(const QString &StyleName);
	void OnBackgroundColor();
	void OnTextColor();
	void OnTextFont();
    void OnGraphAliasing();
    void OnDarkMode();
    void OnLightMode();

private:
	void reject();
	void SetupLayout();

    NewColorButton *m_pctrlBackColor;
	QPushButton *m_pctrlTextClr;
	QPushButton *m_pctrlTextFont;
    QPushButton *OKButton, *CancelButton;
    QPushButton *darkMode, *lightMode;
    QCheckBox *m_pctrlTwoDAntiAliasing;
    NumberEdit *m_tabWidthEdit;

	QComboBox *m_pctrlStyles;
	QPushButton *OK, *Cancel;

	QColor m_BackgroundColor;
	QColor m_TextColor;
	QFont m_TextFont;

};


#endif // DisplaySettingsDlg_H









