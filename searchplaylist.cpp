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

#include "searchplaylist.h"
#include "juk-exception.h"

#include <kdebug.h>

#include <QHash>

#include "playlistitem.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SearchPlaylist::SearchPlaylist(PlaylistCollection *collection,
                               const PlaylistSearch &search,
                               const QString &name,
                               bool setupPlaylist,
                               bool synchronizePlaying) :
    DynamicPlaylist(search.playlists(), collection, name, "edit-find",
                    setupPlaylist, synchronizePlaying),
    m_search(search)
{

}

void SearchPlaylist::setPlaylistSearch(const PlaylistSearch &s, bool update)
{
    m_search = s;
    if(update)
        setPlaylists(s.playlists());
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

void SearchPlaylist::updateItems()
{
    // Here we don't simply use "clear" since that would involve a call to
    // items() which would in turn call this method...

    PlaylistItemList l = Playlist::items();

    QHash<CollectionListItem *, PlaylistItem *> oldItems;
    oldItems.reserve(503);

    foreach(PlaylistItem *item, l)
        oldItems.insert(item->collectionItem(), item);

    m_search.search();
    PlaylistItemList matched = m_search.matchedItems();
    PlaylistItemList newItems;

    foreach(PlaylistItem *item, matched) {
        if(oldItems.remove(item->collectionItem()) == 0)
            newItems.append(item->collectionItem());
    }

    clearItems(PlaylistItemList(oldItems.values()));
    createItems(newItems);

    if(synchronizePlaying()) {
        kDebug() << "synchronizing playing";
        synchronizePlayingItems(m_search.playlists(), true);
    }
}


////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const SearchPlaylist &p)
{
    s << p.name()
      << p.playlistSearch();

    return s;
}

QDataStream &operator>>(QDataStream &s, SearchPlaylist &p)
{
    QString name;
    PlaylistSearch search;

    s >> name
      >> search;

    if(name.isEmpty())
        throw BICStreamException();

    p.setName(name);
    p.setPlaylistSearch(search, false);

    return s;
}

// vim: set et sw=4 tw=0 sta:
