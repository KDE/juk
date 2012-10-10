/***************************************************************************
    begin                : Thu Aug 19 2004
    copyright            : (C) 2002 - 2004 by Michael Pyne
    email                : michael.pyne@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "tracksequenceiterator.h"

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <krandom.h>
#include <ktoggleaction.h>

#include "playlist/playlists/playlist.h"
#include "actioncollection.h"
#include "tag.h"
#include "filehandle.h"

using namespace ActionCollection;

TrackSequenceIterator::TrackSequenceIterator() :
    m_current()
{
}

TrackSequenceIterator::TrackSequenceIterator(const TrackSequenceIterator &other) :
    m_current(other.m_current)
{
}

TrackSequenceIterator::~TrackSequenceIterator()
{
}

void TrackSequenceIterator::setCurrent(const QModelIndex &current)
{
    m_current = current;
}

void TrackSequenceIterator::playlistChanged()
{
}

DefaultSequenceIterator::DefaultSequenceIterator() :
    TrackSequenceIterator()
{
}

DefaultSequenceIterator::DefaultSequenceIterator(const DefaultSequenceIterator &other)
    : TrackSequenceIterator(other)
{
}

DefaultSequenceIterator::~DefaultSequenceIterator()
{
}

void DefaultSequenceIterator::advance()
{
    if(!current().isValid())
        return;

    bool isRandom = action("randomPlay") && action<KToggleAction>("randomPlay")->isChecked();
    bool loop = action<KAction>("loopPlaylist") && action<KAction>("loopPlaylist")->isChecked();
    bool albumRandom = action("albumRandomPlay") && action<KToggleAction>("albumRandomPlay")->isChecked();

   
    if(isRandom || albumRandom) {
         //### TODO FIXME
//         if(m_randomItems.isEmpty() && loop) {
// 
//             // Since refillRandomList will remove the currently playing item,
//             // we should clear it out first since that's not good for e.g.
//             // lists with 1-2 items.  We need to remember the Playlist though.
// 
//             const Playlist *playlist = qobject_cast<const Playlist*>(current().model());
//             setCurrent(QModelIndex());
// 
//             refillRandomList(playlist);
//         }
// 
//         if(m_randomItems.isEmpty()) {
//             setCurrent(QModelIndex());
//             return;
//         }
// 
//         QModelIndex item;
// 
//         if(albumRandom) {
//             if(m_albumSearch.isNull() || m_albumSearch.matchedItems().isEmpty()) {
//                 item = m_randomItems[KRandom::random() % m_randomItems.count()];
//                 initAlbumSearch(item);
//             }
// 
//             // This can be null if initAlbumSearch() left the m_albumSearch
//             // empty because the album text was empty.  Since we initAlbumSearch()
//             // with an item, the matchedItems() should never be empty.
// 
//             if(!m_albumSearch.isNull()) {
//                 QModelIndexList albumMatches = m_albumSearch.matchedItems();
//                 if(albumMatches.isEmpty()) {
//                     kError() << "Unable to initialize album random play.\n";
//                     kError() << "List of potential results is empty.\n";
// 
//                     return; // item is still set to random song from a few lines earlier.
//                 }
// 
//                 item = albumMatches[0];
//                 const Playlist *playlist = qobject_cast<const Playlist*>(item.model());
//                 const FileHandle &first = playlist->data(item, Qt::UserRole).value<FileHandle>();
//                 // Pick first song remaining in list.
// 
//                 for(int i = 0; i < albumMatches.count(); ++i) {
//                     playlist = qobject_cast<const Playlist*>(albumMatches[i].model());
//                     const FileHandle &file = playlist->data(albumMatches[i], Qt::UserRole).value<FileHandle>();
//                     if(file.tag()->track() < first.tag()->track())
//                         item = albumMatches[i];
//                 }
//                 m_albumSearch.clearItem(item);
// 
//                 if(m_albumSearch.matchedItems().isEmpty()) {
//                     m_albumSearch.clearComponents();
//                     m_albumSearch.search();
//                 }
//             }
//             else
//                 kError() << "Unable to perform album random play on " << item->file().absFilePath() << endl;
//         }
//         else
//             item = m_randomItems[KRandom::random() % m_randomItems.count()];
// 
//         setCurrent(item);
//         m_randomItems.removeAll(item);
//         }
    } else {
        QModelIndex next;
        const Playlist *playlist = qobject_cast<const Playlist*>(current().model());
        if (current().row() < playlist->rowCount())
            next = playlist->index(current().row() + 1, 0);
        if (!next.isValid() && loop)
            next = playlist->index(0, 0);


        setCurrent(next);
    }
}

void DefaultSequenceIterator::backup()
{
    if(!current().isValid())
        return;

    if (current().row() > 0) {
        const Playlist *playlist = qobject_cast<const Playlist*>(current().model());
        QModelIndex item = playlist->index(current().row() - 1, 0);
        setCurrent(item);
    }
}

void DefaultSequenceIterator::prepareToPlay(const Playlist* playlist)
{
    bool random = action("randomPlay") && action<KToggleAction>("randomPlay")->isChecked();
    bool albumRandom = action("albumRandomPlay") && action<KToggleAction>("albumRandomPlay")->isChecked();

    if(random || albumRandom) {
        // ### TODO: View FIXME
//         PlaylistItemList items = playlist->items();//selectedItems();
// //         if(items.isEmpty())
// //             items = playlist->visibleItems();
// 
//         QModelIndex newItem;
//         if(!items.isEmpty())
//             newItem = items[KRandom::random() % items.count()];
// 
//         setCurrent(newItem);
//         refillRandomList();
    }
    else {
        // ### TODO: View
//         Q3ListViewItemIterator it(playlist, Q3ListViewItemIterator::Visible | Q3ListViewItemIterator::Selected);
//         if(!it.current())
//             it = Q3ListViewItemIterator(playlist, Q3ListViewItemIterator::Visible);

//         setCurrent(static_cast<const QModelIndex &>(it.current()));
        setCurrent(playlist->index(0, 0));
    }
}

void DefaultSequenceIterator::reset()
{
    m_randomItems.clear();
    m_albumSearch.clearComponents();
    m_albumSearch.search();
    setCurrent(QModelIndex());
}

void DefaultSequenceIterator::playlistChanged()
{
    refillRandomList();
}

void DefaultSequenceIterator::setCurrent(const QModelIndex &current)
{
    const QModelIndex &oldCurrent = DefaultSequenceIterator::current();

    TrackSequenceIterator::setCurrent(current);

    bool random = action("randomPlay") && action<KToggleAction>("randomPlay")->isChecked();
    bool albumRandom = action("albumRandomPlay") && action<KToggleAction>("albumRandomPlay")->isChecked();

    if((albumRandom || random) && current.isValid() && m_randomItems.isEmpty()) {

        // We're setting a current item, refill the random list now, and remove
        // the current item.

        refillRandomList();
    }

    m_randomItems.removeAll(current);

    if(albumRandom && current.isValid() && !oldCurrent.isValid()) {

        // Same idea as above

//         initAlbumSearch(current);
        // TODO FIXME ###
//         m_albumSearch.clearItem(current);
    }
}

DefaultSequenceIterator *DefaultSequenceIterator::clone() const
{
    return new DefaultSequenceIterator(*this);
}

void DefaultSequenceIterator::refillRandomList(const Playlist *p)
{
    if(!p) {
        if (!current().isValid())
            return;

        p = qobject_cast<const Playlist*>(current().model());

        if(!p) {
            kError() << "Item has no playlist!\n";
            return;
        }
    }

    // ### FIXME TODO
//     m_randomItems = p->items();//visibleItems();
//     m_randomItems.removeAll(current());
    m_albumSearch.clearComponents();
    m_albumSearch.search();
}

void DefaultSequenceIterator::initAlbumSearch(const QModelIndex& searchItem)
{
//     if(!searchItem.isValid())
//         return;
// 
//     m_albumSearch.clearPlaylists();
//     m_albumSearch.addPlaylist(searchItem->playlist());
// 
//     ColumnList columns;
// 
//     m_albumSearch.setSearchMode(PlaylistSearch::MatchAll);
//     m_albumSearch.clearComponents();
// 
//     // If the album name is empty, it will mess up the search,
//     // so ignore empty album names.
// 
//     if(searchItem->file().tag()->album().isEmpty())
//         return;
// 
//     columns.append(Playlist::AlbumColumn);
// 
//     m_albumSearch.addComponent(PlaylistSearch::Component(
//         searchItem->file().tag()->album(),
//         true,
//         columns,
//         PlaylistSearch::Component::Exact)
//     );
// 
//     // If there is an Artist tag with the track, match against it as well
//     // to avoid things like multiple "Greatest Hits" albums matching the
//     // search.
// 
//     if(!searchItem->file().tag()->artist().isEmpty()) {
//         kDebug() << "Searching both artist and album.";
//         columns[0] = Playlist::ArtistColumn;
// 
//         m_albumSearch.addComponent(PlaylistSearch::Component(
//             searchItem->file().tag()->artist(),
//             true,
//             columns,
//             PlaylistSearch::Component::Exact)
//         );
//     }
// 
//     m_albumSearch.search();
}

// vim: set et sw=4 tw=0 sta:
