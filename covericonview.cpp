/***************************************************************************
    begin                : Sat Jul 9 2005 
    copyright            : (C) 2005 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "covericonview.h"
#include "covermanager.h"

using CoverUtility::CoverIconViewItem;

CoverIconViewItem::CoverIconViewItem(coverKey id, QIconView *parent) :
    KIconViewItem(parent), m_id(id)
{
    CoverDataPtr data = CoverManager::coverInfo(id);
    setText(QString("%1 - %2").arg(data->artist, data->album));
    setPixmap(data->thumbnail());
}

CoverIconView::CoverIconView(QWidget *parent, const char *name) : KIconView(parent, name)
{
    setResizeMode(Adjust);
}

CoverIconViewItem *CoverIconView::currentItem() const
{
    return static_cast<CoverIconViewItem *>(KIconView::currentItem());
}

QDragObject *CoverIconView::dragObject()
{
    CoverIconViewItem *item = currentItem();
    if(item)
	return new CoverDrag(item->id(), this);

    return 0;
}

// vim: set et ts=4 sw=4:
