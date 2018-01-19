/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef DYNAMICPLAYLIST_H
#define DYNAMICPLAYLIST_H

#include "playlist.h"

#include <QList>

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
    DynamicPlaylist(const PlaylistList &lists,
                    PlaylistCollection *collection,
                    const QString &name = QString(),
                    const QString &iconName = "audio-midi",
                    bool setupPlaylist = true,
                    bool synchronizePlaying = false);

    virtual ~DynamicPlaylist();

    virtual bool canReload() const { return false; }

    void setPlaylists(const PlaylistList &playlists);

public slots:
    /**
     * Reimplemented so that it will reload all of the playlists that are
     * associated with the dynamic list.
     */
    virtual void slotReload();
    void slotSetDirty() { m_dirty = true; }

    /**
     * This is called when lowering the widget from the widget stack so that
     * it can synchronize the playing item with the one that playlist it was
     * create from.
     */
    void lower(QWidget *top = 0);

protected:
    /**
     * Returns true if this list's items need to be updated the next time it's
     * shown.
     */
    bool dirty() const { return m_dirty; }

    /**
     * Return a list of the items in this playlist.  For example in a search
     * list this should return only the matched items.  By default it returns
     * all of the items in the playlists associated with this dynamic list.
     */
    virtual PlaylistItemList items();

    /**
     * Reimplemented from QWidget.  Here it updates the list of items (when
     * appropriate) as the widget is shown.
     */
    virtual void showEvent(QShowEvent *e);

    virtual void paintEvent(QPaintEvent *e);

    /**
     * Updates the items (unconditionally).  This should be reimplemented in
     * subclasses to refresh the items in the dynamic list (i.e. running a
     * search).
     */
    virtual void updateItems();

    bool synchronizePlaying() const;

private:
    /**
     * Checks to see if the current list of items is "dirty" and if so updates
     * this dynamic playlist's items to be in sync with the lists that it is a
     * wrapper around.
     */
    void checkUpdateItems();

private:
    QList<PlaylistObserver *> m_observers;
    PlaylistItemList m_siblings;
    PlaylistList m_playlists;
    bool m_dirty;
    bool m_synchronizePlaying;
};

#endif

// vim: set et sw=4 tw=0 sta:
