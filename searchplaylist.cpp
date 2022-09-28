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

#include "playlistitem.h"
#include "collectionlist.h"
#include "juk_debug.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SearchPlaylist::SearchPlaylist(PlaylistCollection *collection,
                               PlaylistSearch& search,
                               const QString &name,
                               bool setupPlaylist,
                               bool synchronizePlaying) :
    DynamicPlaylist(search.playlists(), collection, name, "edit-find",
                    setupPlaylist, synchronizePlaying),
    m_search(&search)
{

}

SearchPlaylist::~SearchPlaylist()
{
    // DynamicPlaylist needs us to call this while the virtual call still works
    updateItems();
}

void SearchPlaylist::setPlaylistSearch(PlaylistSearch* s, bool update)
{
    m_search = s;
    if(update)
        setPlaylists(s->playlists());
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

void SearchPlaylist::updateItems()
{
    // Here we don't simply use "clear" since that would involve a call to
    // items() which would in turn call this method...

    PlaylistItemList items;
    const auto matchingItems = m_search->matchedItems();
    for(const QModelIndex &index : matchingItems)
        items.push_back(static_cast<PlaylistItem*>(itemFromIndex(index)));
    synchronizeItemsTo(items);

    if(synchronizePlaying()) {
        qCDebug(JUK_LOG) << "synchronizing playing";
        synchronizePlayingItems(m_search->playlists(), true);
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
    auto search = new PlaylistSearch(&p);

    s >> name
      >> *search;

    if(name.isEmpty())
        throw BICStreamException();

    p.setName(name);
    p.setPlaylistSearch(search, false);

    return s;
}

// vim: set et sw=4 tw=0 sta:
