/**
 * Copyright (C) 2005 Michael Pyne <mpyne@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "covericonview.h"
#include "covermanager.h"

using CoverUtility::CoverIconViewItem;

CoverIconViewItem::CoverIconViewItem(coverKey id, Q3IconView *parent) :
    K3IconViewItem(parent), m_id(id)
{
    CoverDataPtr data = CoverManager::coverInfo(id);
    setText(QString("%1 - %2").arg(data->artist, data->album));
    setPixmap(data->thumbnail());
}

CoverIconView::CoverIconView(QWidget *parent, const char *name) : K3IconView(parent, name)
{
    setResizeMode(Adjust);
}

CoverIconViewItem *CoverIconView::currentItem() const
{
    return static_cast<CoverIconViewItem *>(K3IconView::currentItem());
}

Q3DragObject *CoverIconView::dragObject()
{
#if 0
    // Temporarily disabled pending conversion of the cover manager icon view
    // to Qt 4 ish stuff.
    CoverIconViewItem *item = currentItem();
    if(item)
        return new CoverDrag(item->id(), this);
#endif
    return 0;
}

// vim: set et sw=4 tw=0 sta:
