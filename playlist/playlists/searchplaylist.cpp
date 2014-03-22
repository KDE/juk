/***************************************************************************
    begin                : Mon May 5 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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

#include "searchplaylist.h"
#include "juk-exception.h"

#include <kdebug.h>

#include <QHash>

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
    // ### TODO FIXME we are going to nuke this class in favour of proxy models, so fuck this for now
    // Here we don't simply use "clear" since that would involve a call to
    // items() which would in turn call this method...

//     PlaylistItemList l = Playlist::items();
// 
//     QHash<CollectionListItem *, PlaylistItem *> oldItems;
//     oldItems.reserve(503); //### WTF 
// 
//     foreach(PlaylistItem *item, l)
//         oldItems.insert(item->collectionItem(), item);
// 
//     m_search.search();
//     QModelIndexList matched = m_search.matchedItems();
//     QModelIndexList newItems;
// 
//     foreach(const QModelIndex &item, matched) {
//         if(oldItems.remove(item->collectionItem()) == 0)
//             newItems.append(item->collectionItem());
//     }
// 
//     foreach(const QModelIndex &item, oldItems)
//         clearItem(item, false);
// 
//     if(!oldItems.isEmpty() && newItems.isEmpty())
//         weChanged();
// 
//     createItems(newItems);
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

#include "searchplaylist.moc"

// vim: set et sw=4 tw=0 sta: