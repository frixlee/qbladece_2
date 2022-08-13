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

#include "NumberEditDelegate.h"

#include "NumberEdit.h"


NumberEditDelegate::NumberEditDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

QWidget *NumberEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
										  const QModelIndex &/*index*/) const
{
	NumberEdit *edit = new NumberEdit;
	edit->setParent(parent);  // NM not sure if necessary but included in Qt SpinBoxDelegate example
	edit->setAutoFillBackground(true);
	return edit;
}

void NumberEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
	double value = index.model()->data(index, Qt::EditRole).toDouble();
	NumberEdit *edit = static_cast<NumberEdit*>(editor);
	edit->setValue(value);
}

void NumberEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
	NumberEdit *edit = static_cast<NumberEdit*>(editor);
	double value = edit->getValue(true);
	model->setData(index, value, Qt::EditRole);
}

void NumberEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
											  const QModelIndex &/*index*/) const
{
	editor->setGeometry(option.rect);
}
