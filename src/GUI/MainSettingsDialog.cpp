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

#include "MainSettingsDialog.h"
#include "../MainFrame.h"
#include "src/TwoDWidgetInterface.h"
#include "../QBladeApplication.h"
#include <QGroupBox>
#include <QColorDialog>
#include <QFontDialog>
#include <QStyleFactory>
#include "../Globals.h"
#include "../QBEM/BEM.h"
#include "../QDMS/DMS.h"

MainSettingsDialog::MainSettingsDialog()
{
    setWindowTitle(tr("General Settings"));
	SetupLayout();

	connect(m_pctrlStyles, SIGNAL(activated(const QString &)),this, SLOT(OnStyleChanged(const QString &)));
	connect(m_pctrlBackColor, SIGNAL(clicked()),this, SLOT(OnBackgroundColor()));
	connect(m_pctrlTextClr, SIGNAL(clicked()),this, SLOT(OnTextColor()));
	connect(m_pctrlTextFont, SIGNAL(clicked()),this, SLOT(OnTextFont()));
	connect(OKButton, SIGNAL(clicked()),this, SLOT(accept()));
	connect(CancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_pctrlTwoDAntiAliasing, SIGNAL(clicked()), this, SLOT(OnGraphAliasing()));
}


void MainSettingsDialog::SetupLayout()
{
	QVBoxLayout *MainLayout = new QVBoxLayout;

    QGroupBox *StyleBox = new QGroupBox(tr("Window Style"));
    QHBoxLayout *bLay = new QHBoxLayout;
    StyleBox->setLayout(bLay);
	m_pctrlStyles = new QComboBox;
	m_pctrlStyles->addItems(QStyleFactory::keys());
    bLay->addWidget(m_pctrlStyles);

	QHBoxLayout *BackLayout = new QHBoxLayout;
    m_pctrlBackColor      = new NewColorButton;
	m_pctrlBackColor->setMinimumWidth(120);
	BackLayout->addWidget(m_pctrlBackColor);
	QGroupBox *BackBox = new QGroupBox(tr("Background Color"));
	BackBox->setLayout(BackLayout);

	QHBoxLayout *FontLayout = new QHBoxLayout;
	m_pctrlTextFont       = new QPushButton;
	m_pctrlTextClr        = new QPushButton(tr("Text Color"));
	m_pctrlTextFont->setMinimumWidth(120);
	m_pctrlTextClr->setMinimumWidth(120);
	FontLayout->addWidget(m_pctrlTextFont);
	FontLayout->addWidget(m_pctrlTextClr);
	QGroupBox *FontBox = new QGroupBox(tr("Font"));
	FontBox->setLayout(FontLayout);

    QHBoxLayout *TabLayout = new QHBoxLayout;
    m_tabWidthEdit = new NumberEdit();
    m_tabWidthEdit->setAutomaticPrecision(0);
    m_tabWidthEdit->setMinimum(1);
    QLabel *tabLabel = new QLabel("Text Editor Tab Width");
    TabLayout->addWidget(tabLabel);
    TabLayout->addWidget(m_tabWidthEdit);
    QGroupBox *TabBox = new QGroupBox(tr("Tab"));
    TabBox->setLayout(TabLayout);

    QHBoxLayout *ModeLayout = new QHBoxLayout;
    lightMode = new QPushButton("Light Mode");
    darkMode = new QPushButton("Dark Mode");
    ModeLayout->addWidget(lightMode);
    ModeLayout->addWidget(darkMode);
    QGroupBox *ModeBox = new QGroupBox(tr("Mode"));
    ModeBox->setLayout(ModeLayout);

    connect(lightMode,SIGNAL(clicked(bool)),this,SLOT(OnLightMode()));
    connect(darkMode,SIGNAL(clicked(bool)),this,SLOT(OnDarkMode()));

	QHBoxLayout *CommandButtons = new QHBoxLayout;
	OKButton = new QPushButton(tr("OK"));
	OKButton->setAutoDefault(false);
	CancelButton = new QPushButton(tr("Cancel"));
	CancelButton->setAutoDefault(false);
	CommandButtons->addStretch(1);
	CommandButtons->addWidget(OKButton);
	CommandButtons->addStretch(1);
	CommandButtons->addWidget(CancelButton);
	CommandButtons->addStretch(1);

    m_pctrlTwoDAntiAliasing = new QCheckBox("Enable 2D Antialiasing");
    QGroupBox *aliasingBox = new QGroupBox(tr("Graph Appearance"));
    QHBoxLayout *qhblay = new QHBoxLayout;
    aliasingBox->setLayout(qhblay);
    qhblay->addWidget(m_pctrlTwoDAntiAliasing);

	MainLayout->addStretch(1);
    MainLayout->addWidget(StyleBox);
	MainLayout->addStretch(1);
	MainLayout->addWidget(BackBox);
	MainLayout->addStretch(1);
    MainLayout->addWidget(FontBox);
    MainLayout->addWidget(ModeBox);
    MainLayout->addWidget(TabBox);
    MainLayout->addWidget(aliasingBox);
	MainLayout->addSpacing(20);
	MainLayout->addStretch(1);
	MainLayout->addLayout(CommandButtons);
	MainLayout->addStretch(1);

	setLayout(MainLayout);
}

void MainSettingsDialog::OnDarkMode(){

    m_TextColor = QColor(255,255,255);
    m_BackgroundColor = QColor(0,0,0);

    QPalette palette = m_pctrlTextClr->palette();
    QColor listColor = palette.color(QPalette::Button);
    if(listColor.isValid())
    {
        palette.setColor(QPalette::Button, m_BackgroundColor);
        palette.setColor(QPalette::ButtonText, m_TextColor);
        m_pctrlTextClr->setPalette(palette);
    }
    m_pctrlBackColor->setColor(m_BackgroundColor);

    for (int i=0;i<g_graphList.size();i++){
        g_graphList.at(i)->setTitleColor(QColor(255,255,255,255));
        g_graphList.at(i)->setTickColor(QColor(255,255,255,255));
        g_graphList.at(i)->setBackgroundColor(QColor(0,0,0,255));
    }

    TwoDWidgetInterface* inter = g_mainFrame->getTwoDWidgetInterface();
    if (inter) inter->update();
    else g_mainFrame->UpdateView();
}

void MainSettingsDialog::OnLightMode(){

    m_TextColor = QColor(0,0,0);
    m_BackgroundColor = QColor(255,255,255);

    QPalette palette = m_pctrlTextClr->palette();
    QColor listColor = palette.color(QPalette::Button);
    if(listColor.isValid())
    {
        palette.setColor(QPalette::Button, m_BackgroundColor);
        palette.setColor(QPalette::ButtonText, m_TextColor);
        m_pctrlTextClr->setPalette(palette);
    }
    m_pctrlBackColor->setColor(m_BackgroundColor);

    for (int i=0;i<g_graphList.size();i++){
        g_graphList.at(i)->setTitleColor(QColor(70,70,70,255));
        g_graphList.at(i)->setTickColor(QColor(70,70,70,255));
        g_graphList.at(i)->setBackgroundColor(QColor(255,255,255,255));
    }

    TwoDWidgetInterface* inter = g_mainFrame->getTwoDWidgetInterface();
    if (inter) inter->update();
    else g_mainFrame->UpdateView();
}

void MainSettingsDialog::InitDialog()
{
    m_pctrlBackColor->setColor(m_BackgroundColor);
	QString FontName = m_TextFont.family() + QString(" %1").arg(m_TextFont.pointSize());
	m_pctrlTextFont->setText(FontName);
	m_pctrlStyles->setCurrentIndex(m_pctrlStyles->findText(dynamic_cast<QBladeApplication*>(qApp)->getApplicationStyle(),
														   Qt::MatchFixedString));

    m_tabWidthEdit->setValue(g_mainFrame->m_TabWidth);

	QPalette palette = m_pctrlTextClr->palette();
	QColor listColor = palette.color(QPalette::Button);
	if(listColor.isValid())
	{
		palette.setColor(QPalette::Button, m_BackgroundColor);
		palette.setColor(QPalette::ButtonText, m_TextColor);
		m_pctrlTextClr->setPalette(palette);
		m_pctrlTextClr->setFont(m_TextFont);
	}

    m_pctrlTwoDAntiAliasing->setChecked(twoDAntiAliasing);
}

void MainSettingsDialog::OnStyleChanged(const QString &StyleName) {
	dynamic_cast<QBladeApplication*>(qApp)->setApplicationStyle(StyleName);
}

void MainSettingsDialog::OnGraphAliasing(){
    if (m_pctrlTwoDAntiAliasing->isChecked()) twoDAntiAliasing = true;
    else twoDAntiAliasing = false;
}


void MainSettingsDialog::OnBackgroundColor()
{
    if(m_pctrlBackColor->getColor().isValid()) m_BackgroundColor = m_pctrlBackColor->getColor();

    QPalette palette = m_pctrlTextClr->palette();
    QColor listColor = palette.color(QPalette::Button);
    if(listColor.isValid())
    {
        palette.setColor(QPalette::Button, m_BackgroundColor);
        palette.setColor(QPalette::ButtonText, m_TextColor);
        m_pctrlTextClr->setPalette(palette);
    }
}


void MainSettingsDialog::reject()
{
	QDialog::reject();
}

void MainSettingsDialog::OnTextColor()
{
	QColor Color = QColorDialog::getColor(m_TextColor);
	if(Color.isValid()) m_TextColor = Color;

	QPalette palette = m_pctrlTextClr->palette();
	QColor listColor = palette.color(QPalette::Button);
	if(listColor.isValid())
	{
		palette.setColor(QPalette::Button, m_BackgroundColor);
		palette.setColor(QPalette::ButtonText, m_TextColor);
		m_pctrlTextClr->setPalette(palette);
	}
}



void MainSettingsDialog::OnTextFont()
{
	bool ok;
	QFont TextFont;
	TextFont.setStyleHint(QFont::TypeWriter, QFont::OpenGLCompatible);

	TextFont = QFontDialog::getFont(&ok, m_TextFont, this);

	if (ok)
	{
		m_TextFont = TextFont;
		m_pctrlTextFont->setText(m_TextFont.family());
		m_pctrlTextFont->setFont(m_TextFont);
		m_pctrlTextClr->setFont(m_TextFont);
	}

}








