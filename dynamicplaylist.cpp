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
#include "tracksequencemanager.h"

#include <QTimer>
#include <QObject>

PlaylistDirtyObserver::PlaylistDirtyObserver(DynamicPlaylist* parent, Playlist* playlist) :
    m_parent(parent)
{
    QObject::connect(&(playlist->signaller), &PlaylistInterfaceSignaller::playingItemDataChanged, m_parent, &DynamicPlaylist::slotSetDirty);
}


////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DynamicPlaylist::DynamicPlaylist(const PlaylistList &playlists,
                                 PlaylistCollection *collection,
                                 const QString &name,
                                 const QString &iconName,
                                 bool setupPlaylist,
                                 bool synchronizePlaying) :
    Playlist(collection, true),
    m_playlists(playlists),
    m_dirty(true),
    m_synchronizePlaying(synchronizePlaying)
{
    if(setupPlaylist)
        collection->setupPlaylist(this, iconName);
    setName(name);
    setAllowDuplicates(false);

    setSortingEnabled(false);

    for(PlaylistList::ConstIterator it = playlists.constBegin(); it != playlists.constEnd(); ++it)
        m_observers.append(new PlaylistDirtyObserver(this, *it));

    connect(CollectionList::instance(), SIGNAL(signalCollectionChanged()), this, SLOT(slotSetDirty()));
}

DynamicPlaylist::~DynamicPlaylist()
{
    lower();

    foreach(PlaylistDirtyObserver *observer, m_observers)
        delete observer;
}

void DynamicPlaylist::setPlaylists(const PlaylistList &playlists)
{
    m_playlists = playlists;
    updateItems();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void DynamicPlaylist::slotReload()
{
    for(PlaylistList::Iterator it = m_playlists.begin(); it != m_playlists.end(); ++it)
        (*it)->slotReload();

    checkUpdateItems();
}

void DynamicPlaylist::lower(QWidget *top)
{
    if(top == this)
        return;

    if(playing()) {
        PlaylistList l;
        l.append(this);
        for(PlaylistList::Iterator it = m_playlists.begin();
            it != m_playlists.end(); ++it)
        {
            (*it)->synchronizePlayingItems(l, true);
        }
    }

    PlaylistItemList list = PlaylistItem::playingItems();
    for(PlaylistItemList::Iterator it = list.begin(); it != list.end(); ++it) {
        if((*it)->playlist() == this) {
            list.erase(it);
            break;
        }
    }

    if(!list.isEmpty())
        TrackSequenceManager::instance()->setCurrentPlaylist(list.front()->playlist());
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

    for(PlaylistList::ConstIterator it = m_playlists.constBegin(); it != m_playlists.constEnd(); ++it)
        siblings += (*it)->items();

    if(m_siblings != siblings) {
        m_siblings = siblings;
        this->synchronizeItemsTo(siblings);

        if(m_synchronizePlaying) {
            synchronizePlayingItems(m_playlists, true);
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
