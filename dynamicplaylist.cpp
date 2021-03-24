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

#include "dynamicplaylist.h"
#include "collectionlist.h"
#include "playlistcollection.h"

#include <QTimer>
#include <QObject>

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DynamicPlaylist::DynamicPlaylist(const PlaylistList &playlists,
                                 PlaylistCollection *collection,
                                 const QString &name,
                                 const QString &iconName,
                                 bool setupPlaylist,
                                 bool synchronizePlaying)
  : Playlist(collection, true)
  , m_dirty(true)
  , m_synchronizePlaying(synchronizePlaying)
{
    if(setupPlaylist)
        collection->setupPlaylist(this, iconName);

    setName(name);
    setAllowDuplicates(false);
    setSortingEnabled(false);

    for(const auto playlist : playlists) {
        m_playlists << QPointer<Playlist>(playlist);

        connect(&(playlist->signaller), &PlaylistInterfaceSignaller::playingItemDataChanged,
                this, &DynamicPlaylist::slotSetDirty);
    }

    connect(CollectionList::instance(), &CollectionList::signalCollectionChanged,
            this, &DynamicPlaylist::slotSetDirty);
}

DynamicPlaylist::~DynamicPlaylist()
{
    // Subclasses need to ensure they update items in their own destructor, or
    // any other virtual calls they may need to make.

    lower();
}

void DynamicPlaylist::setPlaylists(const PlaylistList &playlists)
{
    m_playlists.clear();

    for(const auto playlist : playlists) {
        m_playlists << QPointer<Playlist>(playlist);
    }

    updateItems();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void DynamicPlaylist::slotReload()
{
    for(const auto &playlist : qAsConst(m_playlists)) {
        if(!playlist)
            continue;
        playlist->slotReload();
    }

    checkUpdateItems();
}

void DynamicPlaylist::lower(QWidget *top)
{
    if(top == this || !playing())
        return;

    PlaylistList l;
    l.append(this);

    for(const auto &playlist : qAsConst(m_playlists)) {
        if(!playlist)
            continue;
        playlist->synchronizePlayingItems(l, true);
    }
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItemList DynamicPlaylist::items()
{
    checkUpdateItems();
    return Playlist::items();
}

void DynamicPlaylist::showEvent(QShowEvent *e)
{
    checkUpdateItems();
    Playlist::showEvent(e);
}

void DynamicPlaylist::paintEvent(QPaintEvent *e)
{
    checkUpdateItems();
    Playlist::paintEvent(e);
}

void DynamicPlaylist::updateItems()
{
    PlaylistItemList siblings;

    for(const auto &playlist : qAsConst(m_playlists)) {
        if(!playlist)
            continue;
        siblings += playlist->items();
    }

    if(m_siblings != siblings) {
        m_siblings = siblings;
        this->synchronizeItemsTo(siblings);

        if(m_synchronizePlaying) {
            for(const auto &playlist : qAsConst(m_playlists)) {
                synchronizePlayingItems(playlist, true);
            }
        }
    }
}

bool DynamicPlaylist::synchronizePlaying() const
{
    return m_synchronizePlaying;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void DynamicPlaylist::checkUpdateItems()
{
    if(!m_dirty)
        return;

    updateItems();

    m_dirty = false;
}

// vim: set et sw=4 tw=0 sta:
