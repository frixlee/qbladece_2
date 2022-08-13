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

#include "FloatDelegate.h"


FloatDelegate::FloatDelegate(QObject *parent)
: QItemDelegate(parent){
    QFEM = false;
}

QWidget *FloatDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex & index ) const{
	if(m_Precision[index.column()]>=0)
	{
		//we have a number
		NumberEdit *editor = new NumberEdit();
		editor->setParent(parent);
		editor->setAutomaticPrecision(m_Precision[index.column()]);
		double value = index.model()->data(index, Qt::EditRole).toDouble();
		editor->setValue(value);
        if (QFEM)
        {
            if(index.column()==0) editor->setEnabled(false);
        }
		return editor;
	}
	else
	{
		//we have a string
		QLineEdit *editor = new QLineEdit(parent);
		editor->setAlignment(Qt::AlignLeft);
		return editor;
	}
}

void FloatDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const{
	if(m_Precision[index.column()]>=0)
	{
		double value = index.model()->data(index, Qt::EditRole).toDouble();
		NumberEdit *floatEdit = static_cast<NumberEdit*>(editor);
		floatEdit->setValue(value);
	}
	else
	{
		QLineEdit *pLine = static_cast<QLineEdit*>(editor);
		pLine->setText(index.model()->data(index, Qt::EditRole).toString());
	}
}



void FloatDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
								const QModelIndex &index) const{
	if(m_Precision[index.column()]>=0)
	{
		NumberEdit *floatEdit = static_cast<NumberEdit*>(editor);
		double value = floatEdit->getValue(true);
		model->setData(index, value, Qt::EditRole);
	}
	else
	{
		QLineEdit *pLine = static_cast<QLineEdit*>(editor);
		model->setData(index, pLine->text(), Qt::EditRole);
	}
}


void FloatDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const{
	editor->setGeometry(option.rect);
}

void FloatDelegate::SetPrecision(int *PrecisionTable){
	m_Precision = PrecisionTable;
}

void FloatDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
	QString strong;
	QStyleOptionViewItem myOption = option;
	if(m_Precision[index.column()]>=0)
	{
		myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
		strong = QString("%1").arg(index.model()->data(index, Qt::DisplayRole).toDouble(),0,'f', m_Precision[index.column()]);

	}
	else
	{
		myOption.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
		strong = index.model()->data(index, Qt::DisplayRole).toString();
	}
	drawDisplay(painter, myOption, myOption.rect, strong);
	drawFocus(painter, myOption, myOption.rect);
}















