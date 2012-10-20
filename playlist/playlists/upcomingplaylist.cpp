/***************************************************************************
    begin                : Thu Aug 19 2004
    copyright            : (C) 2002 - 2004, 2008 by Michael Pyne
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

#include "upcomingplaylist.h"
#include "juk-exception.h"

#include <kdebug.h>
#include <kapplication.h>
#include <kaction.h>

#include "playlist/playlistitem.h"
#include "playlist/playlistcollection.h"
#include "tracksequencemanager.h"
#include "collectionlist.h"
#include "actioncollection.h"

using namespace ActionCollection;

UpcomingPlaylist::UpcomingPlaylist(PlaylistCollection *collection, int defaultSize) :
    Playlist(collection, true),
    m_active(false),
    m_oldIterator(0),
    m_defaultSize(defaultSize)
{
    setName(i18n("Play Queue"));
    setAllowDuplicates(true);
//     setSorting(-1);
}

UpcomingPlaylist::~UpcomingPlaylist()
{
    removeIteratorOverride();
}

void UpcomingPlaylist::initialize()
{
    // Prevent duplicate initialization.

    if(m_active)
        return;

    m_active = true;

    m_oldIterator = manager()->takeIterator();
    manager()->installIterator(new UpcomingSequenceIterator(this));

    if(!m_oldIterator->current().isValid())
        m_oldIterator->prepareToPlay(CollectionList::instance());
    else
        manager()->iterator()->setCurrent(m_oldIterator->current());
}

void UpcomingPlaylist::appendItems(const FileHandleList &itemList)
{
    initialize();

    if(itemList.isEmpty())
        return;

    foreach (const FileHandle &file, itemList) {
        insertFile(file);
    }
    
    weChanged();
}

// void UpcomingPlaylist::playNext()
// {
//     initialize();
// 
//     PlaylistItem *next = TrackSequenceManager::instance()->nextItem();
// 
//     if(next) {
//         setPlaying(next);
//         Playlist *source = m_playlistIndex[next];
//         if(source) {
//             PlaylistList l;
//             l.append(this);
//             source->synchronizePlayingItems(l, false);
//         }
//     }
//     else {
//         removeIteratorOverride();
// 
//         // Normally we continue to play the currently playing item that way
//         // a user can continue to hear their song when deselecting Play Queue.
//         // However we're technically still "playing" when the queue empties and
//         // we reinstall the old iterator so in this situation manually advance
//         // to the next track. (Otherwise we hear the same song twice in a row
//         // during the transition)
// 
//         setPlaying(manager()->nextItem());
//     }
// }

void UpcomingPlaylist::addFiles(const QStringList &files, int pos)
{
    CollectionList::instance()->addFiles(files, pos);

    foreach(const QString &file, files) {
        insertFile(FileHandle(file));
    }
}

void UpcomingPlaylist::removeIteratorOverride()
{
    if(!m_active)
        return;

    m_active = false; // Allow re-initialization.

    if(!m_oldIterator)
        return;

    // Install the old track iterator.

    manager()->installIterator(m_oldIterator);

    // If we have an item that is currently playing, allow it to keep playing.
    // Otherwise, just reset to the default iterator (probably not playing
    // anything.)
    // XXX: Reset to the last playing playlist?

    m_oldIterator->reset();
//     if(playingItem())
//         m_oldIterator->setCurrent(playingItem()->collectionItem());

//     setPlaying(manager()->currentItem(), true);

    Watched::currentChanged();
}

TrackSequenceManager *UpcomingPlaylist::manager() const
{
    return TrackSequenceManager::instance();
}

UpcomingPlaylist::UpcomingSequenceIterator::UpcomingSequenceIterator(UpcomingPlaylist *playlist) :
    TrackSequenceIterator(), m_playlist(playlist)
{
}

UpcomingPlaylist::UpcomingSequenceIterator::UpcomingSequenceIterator(const UpcomingSequenceIterator &other) :
    TrackSequenceIterator(other), m_playlist(other.m_playlist)
{
}

UpcomingPlaylist::UpcomingSequenceIterator::~UpcomingSequenceIterator()
{
}

void UpcomingPlaylist::UpcomingSequenceIterator::advance()
{
    const QModelIndex &item = m_playlist->index(0, 0);

    if(item.isValid()) {
        QModelIndex next;
        if (m_playlist->rowCount() > 1)
            next = m_playlist->index(1, 0);
        m_playlist->clearRow(item.row());
        setCurrent(next);
    }
}

void UpcomingPlaylist::UpcomingSequenceIterator::backup()
{
}

UpcomingPlaylist::UpcomingSequenceIterator *UpcomingPlaylist::UpcomingSequenceIterator::clone() const
{
    return new UpcomingSequenceIterator(*this);
}

void UpcomingPlaylist::UpcomingSequenceIterator::setCurrent(const QModelIndex &currentItem)
{
    if(!currentItem.isValid()) {
        TrackSequenceIterator::setCurrent(currentItem);
        return;
    }

    // If the upcoming playlist is playing something, clear it out since
    // apparently the user didn't want to hear it.

//     PlaylistItem *playingItem = m_playlist->playingItem();
//     if(playingItem && playingItem->playlist() == m_playlist && currentItem != playingItem)
//         m_playlist->clearItem(playingItem);

    // If a different playlist owns this item, add it to the upcoming playlist

    const Playlist *p = qobject_cast<const Playlist*>(currentItem.model());

    if(p != m_playlist) {
        m_playlist->insertItem(0, currentItem);
//         PlaylistItem *i = m_playlist->createItem(currentItem, (PlaylistItem *) 0);
//         m_playlist->playlistIndex().insert(i, p);
//         m_playlist->weChanged();
//         m_playlist->slotWeightDirty();
    }
    else {
        // if(p == m_playlist) {

        // Bump this item up to the top
        m_playlist->moveItem(currentItem.row(), 0);
//         m_playlist->takeItem(currentItem);
//         m_playlist->insertItem(currentItem);
    }

    TrackSequenceIterator::setCurrent(m_playlist->index(0, 0));
}

void UpcomingPlaylist::UpcomingSequenceIterator::reset()
{
    setCurrent(QModelIndex());
}

void UpcomingPlaylist::UpcomingSequenceIterator::prepareToPlay(const Playlist*)
{
    if(!m_playlist->rowCount() > 0)
        setCurrent(m_playlist->index(0, 0));
}

QDataStream &operator<<(QDataStream &s, const UpcomingPlaylist &p)
{
    QStringList files = p.files();

    s << qint32(files.count());

    foreach(const QString &file, files)
        s << file;

    return s;
}

QDataStream &operator>>(QDataStream &s, UpcomingPlaylist &p)
{
    QString fileName;
    qint32 count;

    s >> count;

    for(qint32 i = 0; i < count; ++i) {
        s >> fileName;
        if(fileName.isEmpty())
            throw BICStreamException();

        p.insertFile(FileHandle(fileName));
    }

    return s;
}

// vim: set et sw=4 tw=0 sta:
