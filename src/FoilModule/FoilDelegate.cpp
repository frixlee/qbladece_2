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

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include "FoilDelegate.h"
#include "../Globals.h"
#include "../Store.h"
#include "../FoilModule/Airfoil.h"
#include "FoilModule.h"
#include "FoilDock.h"


FoilDelegate::FoilDelegate(QObject *parent)
 : QItemDelegate(parent)
{
}


QWidget *FoilDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex & index ) const
{
    return NULL;

}


void FoilDelegate::drawCheck(QPainter *painter, const QStyleOptionViewItem &option,const QRect &, Qt::CheckState state) const
{
    int margin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;

    QRect check = QStyle::alignedRect(option.direction, Qt::AlignCenter,doCheck(option, option.rect, Qt::Checked).size(),QRect(option.rect.x() + margin, option.rect.y(),option.rect.width() - (margin * 2), option.rect.height()));

    QItemDelegate::drawCheck(painter, option, check, state);
}


bool FoilDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
						 const QModelIndex &index)
{
	if(index.column()<12) return false;
	// make sure that the item is checkable
	Qt::ItemFlags flags = model->flags(index);
	if (!(flags & Qt::ItemIsUserCheckable) || !(flags & Qt::ItemIsEnabled))
		return false;
	// make sure that we have a check state
	QVariant value = index.data(Qt::CheckStateRole);
	if (!value.isValid())
		return false;

	// make sure that we have the right event type
	if (event->type() == QEvent::MouseButtonRelease)
	{
		const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
		QRect checkRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
                                              doCheck(option, option.rect, Qt::Checked).size(),
											  QRect(option.rect.x() + textMargin, option.rect.y(),
													option.rect.width() - (2 * textMargin), option.rect.height()));

		if (!checkRect.contains(static_cast<QMouseEvent*>(event)->pos())) return false;
	}
	else if (event->type() == QEvent::KeyPress)
	{
		if (   static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space
			&& static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
			return false;
	}
	else
	{
		return false;
	}

	Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
	
	bool bSuccess = model->setData(index, state, Qt::CheckStateRole);
    g_foilModule->m_Dock->FoilVisibleClicked(index);
		
	return bSuccess;
}


void FoilDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QString strong;
	QStyleOptionViewItem myOption = option;
	int NFoils = g_foilStore.size();

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

	if(index.row()> NFoils)
	{
		strong=" ";
		drawDisplay(painter, myOption, myOption.rect, strong);
		drawFocus(painter, myOption, myOption.rect);
	}
	else if(index.column()==0)
	{
		myOption.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
		strong = index.model()->data(index, Qt::DisplayRole).toString();
		drawDisplay(painter, myOption, myOption.rect, strong);
		drawFocus(painter, myOption, myOption.rect);
	}
	else if(index.column()==5)
	{
		myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
		strong = QString("%1").arg(index.model()->data(index, Qt::DisplayRole).toInt());
		drawDisplay(painter, myOption, myOption.rect, strong);
		drawFocus(painter, myOption, myOption.rect);
	}
	else if(index.column()==6 || index.column()==9)
	{
		myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
		strong = QString("%1").arg(index.model()->data(index, Qt::DisplayRole).toDouble(), 0,'f',m_Precision[index.column()]);
		drawDisplay(painter, myOption, myOption.rect, strong);
		drawFocus(painter, myOption, myOption.rect);
	}
	else if(index.column()==12 || index.column()==13 || index.column()==14)
	{
		QVariant value = index.data(Qt::CheckStateRole);
		Qt::CheckState state;
		if(value.toInt()==0)  state = Qt::Unchecked;
        else state = Qt::Checked;
		drawCheck(painter,myOption, myOption.rect, state);
		drawFocus(painter, myOption, myOption.rect);
	}
	else if(index.column()==15)
	{
		QColor color;
		int style, width;
		//get a link to the foil to get its style

        Airfoil *pFoil = g_foilStore.at(index.row());
        color = pFoil->getPen().color();
        style = GetStyleRevers(pFoil->getPen().style());
        width = pFoil->getPen().width();

		QRect r = option.rect;
        r = g_foilModule->m_Dock->m_pFoilTable->visualRect(index);
		QColor ContourColor = Qt::gray;
		
		painter->setBrush(Qt::NoBrush);
		painter->setBackgroundMode(Qt::TransparentMode);
	
		QPen LinePen(color);
        LinePen.setStyle(GetStyle(style));
		LinePen.setWidth(width);
		painter->setPen(LinePen);
		painter->drawLine(r.left()+5, r.top()+r.height()/2, r.right()-5, r.top()+r.height()/2);
	
		QPen ContourPen(ContourColor);
		painter->setPen(ContourPen);
		r.adjust(0,2,-1,-3);
		painter->drawRoundRect(r,5,40);
		
		drawFocus(painter, myOption, myOption.rect);
	}
	else
	{
		myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
		strong = QString("%1").arg(index.model()->data(index, Qt::DisplayRole).toDouble()*100.0, 0,'f', m_Precision[index.column()]);
		drawDisplay(painter, myOption, myOption.rect, strong);
		drawFocus(painter, myOption, myOption.rect);
	}
}


void FoilDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(index.column()==0)
	{
		QString strong = index.model()->data(index, Qt::EditRole).toString();
		QLineEdit *lineEdit = (QLineEdit*)editor;
		lineEdit->setText(strong);
	}
	else
	{
		double value = index.model()->data(index, Qt::EditRole).toDouble();
		NumberEdit *floatEdit = static_cast<NumberEdit*>(editor);
		floatEdit->setValue(value);
	}
}


void FoilDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(index.column()==0)
	{
		QString strong;
		QLineEdit *pLineEdit = static_cast<QLineEdit*>(editor);
		strong = pLineEdit->text();
		model->setData(index, strong, Qt::EditRole);
	}
	else
	{
		NumberEdit *floatEdit = static_cast<NumberEdit*>(editor);
		double value = floatEdit->getValue(true)/100.0;
		model->setData(index, value, Qt::EditRole);
	}
}


void FoilDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

