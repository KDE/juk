/**
 * Copyright (C) 2002-2004, 2008 Michael Pyne <mpyne@kde.org>
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

#include "upcomingplaylist.h"
#include "juk-exception.h"

#include "playlistitem.h"
#include "playlistcollection.h"
#include "collectionlist.h"
#include "actioncollection.h"
#include "juk_debug.h"

using namespace ActionCollection;

UpcomingPlaylist::UpcomingPlaylist(PlaylistCollection *collection)
  : Playlist(collection, true)
{
    setName(i18n("Play Queue"));
    setAllowDuplicates(true);
    setSortingEnabled(false);
}

void UpcomingPlaylist::appendItems(const PlaylistItemList &itemList)
{
    if(itemList.isEmpty())
        return;

    PlaylistItem *after = static_cast<PlaylistItem *>(topLevelItem(topLevelItemCount() - 1));

    for(auto *playlistItem : itemList) {
        after = createItem(playlistItem, after);
        m_playlistIndex.insert(after, playlistItem->playlist());
    }

    playlistItemsChanged();
    slotWeightDirty();
}

void UpcomingPlaylist::playNext()
{
    auto currentPlaying = firstChild();
    const bool wasOurItem = currentPlaying
        && currentPlaying == Playlist::playingItem()
        && currentPlaying->playlist() == this;

    if(!currentPlaying) {
        qCWarning(JUK_LOG) << "Unexpectedly ended up looking to play from empty Play Queue??";
        CollectionList::instance()->slotBeginPlayback();
        return;
    }

    PlaylistItem *newToPlay = wasOurItem
        ? static_cast<PlaylistItem *>(currentPlaying->itemBelow())
        : currentPlaying;

    if(!newToPlay) {
        // We emptied the playlist, switch back to the playlist for the last item
        Playlist *source = m_playlistIndex[currentPlaying];
        if(source) {
            source->playNext();
        }
        else {
            CollectionList::instance()->slotBeginPlayback();
        }
    }
    else {
        beginPlayingItem(newToPlay);

        Playlist *source = m_playlistIndex[newToPlay];
        if(source) {
            source->synchronizePlayingItems(this, false);
        }
    }

    if(wasOurItem) {
        clearItem(currentPlaying);
    }
}

void UpcomingPlaylist::clearItem(PlaylistItem *item)
{
    m_playlistIndex.remove(item);
    Playlist::clearItem(item);
}

void UpcomingPlaylist::addFiles(const QStringList &files, PlaylistItem *after)
{
    CollectionList::instance()->addFiles(files, after);

    PlaylistItemList l;
    for(const auto &file : files) {
        const FileHandle f(file);
        PlaylistItem *i = CollectionList::instance()->lookup(f.absFilePath());
        if(i)
            l.append(i);
    }

    appendItems(l);
}

QMap< PlaylistItem::Pointer, QPointer<Playlist> > &UpcomingPlaylist::playlistIndex()
{
    return m_playlistIndex;
}

QDataStream &operator<<(QDataStream &s, const UpcomingPlaylist &p)
{
    const PlaylistItemList l = const_cast<UpcomingPlaylist *>(&p)->items();

    s << qint32(l.count());

    for(const auto &playlistItem : l) {
        s << playlistItem->file().absFilePath();
    }

    return s;
}

QDataStream &operator>>(QDataStream &s, UpcomingPlaylist &p)
{
    QStringList upcomingFiles;
    QString fileName;
    qint32 count;

    s >> count;

    for(qint32 i = 0; i < count; ++i) {
        s >> fileName;
        if(fileName.isEmpty())
            throw BICStreamException();

        upcomingFiles << fileName;
    }

    p.addFiles(upcomingFiles);

    return s;
}

// vim: set et sw=4 tw=0 sta:
