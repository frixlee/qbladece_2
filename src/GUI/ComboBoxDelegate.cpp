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

#include "ComboBoxDelegate.h"

#include <QComboBox>
#include "../FoilModule/Airfoil.h"
#include "../QBEM/Polar360.h"
#include "../Store.h"

ComboBoxDelegate::ComboBoxDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
										  const QModelIndex &index) const
{
	QComboBox *edit = new QComboBox (parent);
	edit->setAutoFillBackground(true);

	// fill the box
	if (index.column() == 6) {
		for (int i = 0; i < g_foilStore.size(); ++i) {
			edit->addItem(g_foilStore.at(i)->getName());
		}
	} else if (index.column() == 7) {
		QModelIndex foilIndex = index.model()->index(index.row(), index.column()-1, QModelIndex());
		Airfoil *foil = g_foilStore.getObjectByName(index.model()->data(foilIndex).toString(), NULL);
		for (int i = 0; i < g_360PolarStore.size(); ++i) {
            if (g_360PolarStore.at(i)->GetAirfoil() == foil) {
				edit->addItem(g_360PolarStore.at(i)->getName());
			}
		}
	}

	return edit;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
	QString value = index.model()->data(index, Qt::EditRole).toString();
	QComboBox *edit = static_cast<QComboBox*>(editor);
	edit->setCurrentText(value);
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
	QComboBox *edit = static_cast<QComboBox*>(editor);
	QString value = edit->currentText();
	model->setData(index, value, Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
											  const QModelIndex &/*index*/) const
{
	editor->setGeometry(option.rect);
}
