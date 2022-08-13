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

#ifndef LINESTYLEDIALOG_H
#define LINESTYLEDIALOG_H

#include <QDialog>
#include <QPen>

class LineStyleComboBox;
class NewColorButton;


class LineStyleDialog : public QDialog
{
	Q_OBJECT
	
public:
	LineStyleDialog(QWidget *parent, QPen pen, bool style, bool width, bool color);
	QPen getPen();
	
public slots:
	void setPen(QPen pen);
	void setColor(QColor color);
	
private:
	LineStyleComboBox *m_styleButton, *m_widthButton;
	NewColorButton *m_colorButton;
	QPushButton *m_okButton, *m_cancelButton;
};

#endif // LINESTYLEDIALOG_H
