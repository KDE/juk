/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "painteater.h"
#include "playlist.h"

PaintEater::PaintEater(Playlist *list) : 
    QObject(list),
    m_list(list),
    m_allowOne(false),
    m_previousHeight(0)
{
    // We want to catch paint events for both the contents and the frame of
    // our listview.

    list->installEventFilter(this);
    list->viewport()->installEventFilter(this);
}

bool PaintEater::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::Paint) {

        // There are two cases where we want to let our viewport repaint
        // itself -- if the actual contents have changed as indicated by
        // m_allowOne being true, or if the height has changed indicating
        // that we've either scrolled or resized the widget.

        if(o == m_list->viewport()) {
            if(m_allowOne) {
                m_allowOne = false;
                return false;
            }

            int newHeight = static_cast<QPaintEvent *>(e)->rect().top();

            if(m_previousHeight != newHeight) {
                m_previousHeight = newHeight;
                return false;
            }
        }
        else
            m_allowOne = true;

        if(m_list->count() < 20)
            m_list->slotWeightDirty();

        return true;
    }

    return false;
}
