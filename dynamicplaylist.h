/***************************************************************************
                          dynamicplaylist.cpp
                             -------------------
    begin                : Mon May 5 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#ifndef DYNAMICPLAYLIST_H
#define DYNAMICPLAYLIST_H

#include "playlist.h"

/**
 * A Playlist that is a union of other playlists that is created dynamically.
 */

class DynamicPlaylist : public Playlist
{
    Q_OBJECT
public:
    /**
     * Creates a dynamic playlist based on lists.
     */
    DynamicPlaylist(const PlaylistList &lists, QWidget *parent, const QString &name = QString::null);

protected:
    /**
     * Returns true if this list's items need to be updated the next time it's
     * shown.
     */
    bool dirty() { return m_dirty; }

    /**
     * Return a list of the items in this playlist.  For example in a search 
     * list this should return only the matched items.  By default it returns
     * all of the items in the playlists associated with this dynamic list.
     */
    virtual PlaylistItemList items() const;

    /**
     * Reimplemented from QWidget.  Here it updates the list of items (when
     * appropriate) as the widget is shown.
     */
    virtual void showEvent(QShowEvent *e);

protected slots:
    void slotSetDirty(bool d = true) { m_dirty = d; }

private slots:
    void slotUpdateItems();

private:
    PlaylistItemList m_items;
    PlaylistList m_playlists;
    bool m_dirty;
};

#endif
