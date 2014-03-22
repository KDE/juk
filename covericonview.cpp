/***************************************************************************
    begin                : Sat Jul 9 2005
    copyright            : (C) 2005 by Michael Pyne
                         : (C) 2014 by Arnold Dumas <contact@arnolddumas.fr>
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

CoverIconViewItem::CoverIconViewItem(coverKey id, KListWidget *parent) :
    QListWidgetItem(parent), m_id(id)
{
    CoverDataPtr data = CoverManager::coverInfo(id);
    setText(QString("%1 - %2").arg(data->artist, data->album));
    setIcon(data->thumbnail());
    setSizeHint(QSize(140, 150));
}

CoverIconView::CoverIconView(QWidget *parent, const char *name) : KListWidget(parent)
{
    setObjectName(name);
    setResizeMode(KListWidget::Adjust);
    setViewMode(KListWidget::IconMode);
    setIconSize(QSize(130, 140));
    setMovement(KListWidget::Static);
    setContextMenuPolicy(Qt::CustomContextMenu);
}

CoverIconViewItem *CoverIconView::currentItem() const
{
    return static_cast<CoverIconViewItem *>(KListWidget::currentItem());
}

// TODO: port to Qt4
#if 0
Q3DragObject *CoverIconView::dragObject()
{
    // Temporarily disabled pending conversion of the cover manager icon view
    // to Qt 4 ish stuff.
    CoverIconViewItem *item = currentItem();
    if(item)
        return new CoverDrag(item->id(), this);
    return 0;
}
#endif

// vim: set et sw=4 tw=0 sta:
