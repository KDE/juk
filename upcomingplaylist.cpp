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

#include <kdebug.h>
#include <kapplication.h>
#include <kaction.h>

#include "upcomingplaylist.h"
#include "playlistitem.h"
#include "playlistcollection.h"
#include "tracksequencemanager.h"
#include "collectionlist.h"
#include "actioncollection.h"

using namespace ActionCollection;

UpcomingPlaylist::UpcomingPlaylist(PlaylistCollection *collection, int defaultSize, const QString &name) :
    Playlist(collection, true),
    m_oldIterator(0),
    m_defaultSize(defaultSize)
{
    setName(name);
    setAllowDuplicates(true);
    setSorting(-1);
}

UpcomingPlaylist::~UpcomingPlaylist()
{
    removeIteratorOverride();
    setUpcomingPlaylist(0);
}

void UpcomingPlaylist::initialize()
{
    m_oldIterator = manager()->takeIterator();
    manager()->installIterator(new UpcomingSequenceIterator(this));

    if(playingItem()) {
        PlaylistItemList list;
        list.append(playingItem());
        createItems(list);
    }

    if(!m_oldIterator->current())
        m_oldIterator->prepareToPlay(CollectionList::instance());
    else
        manager()->iterator()->setCurrent(m_oldIterator->current());

    // If an item was already playing, then it will be the last item in the
    // list instead of the first, so we'll need to move it after the list is
    // initialized.  Also, we should fill the list to our set number of items.

    bool needsAdjusting = !items().isEmpty();

    if(needsAdjusting && action<KToggleAction>("loopPlaylist")->isChecked()) {
        m_oldIterator->advance();
        fillList();
        PlaylistItem *item = static_cast<PlaylistItem *>(lastItem());
        takeItem(item);
        insertItem(item);
        setPlaying(item, false);
    }

    setUpcomingPlaylist(this);
}

void UpcomingPlaylist::appendItems(const PlaylistItemList &itemList)
{
    if(itemList.isEmpty())
        return;

    createItems(itemList, static_cast<PlaylistItem *>(lastChild()));
}

void UpcomingPlaylist::removeIteratorOverride()
{
    if(!m_oldIterator)
        return;

    PlaylistItem *newItem = 0;
    if(playingItem())
        newItem = playingItem()->collectionItem();

    m_oldIterator->reset();
    m_oldIterator->setCurrent(newItem);

    manager()->setCurrent(newItem);
    manager()->installIterator(m_oldIterator);

    setPlaying(newItem, false);

    Watched::currentChanged();
}

void UpcomingPlaylist::fillList()
{
    PlaylistItemList list;
    int i = 0, limit = m_defaultSize - items().count();

    if(!m_oldIterator->current())
        m_oldIterator->prepareToPlay(CollectionList::instance());

    if(!m_oldIterator->current())
        return;

    while(i++ < limit && m_oldIterator->current()) {
        list.append(m_oldIterator->current());
        m_oldIterator->advance();
    }

    createItems(list);
}

void UpcomingPlaylist::addNewItem()
{
    m_oldIterator->advance();
    PlaylistItem *last = static_cast<PlaylistItem *>(lastChild());
    
    if(m_oldIterator->current())
        createItem(m_oldIterator->current()->file(), last);
}

inline TrackSequenceManager *UpcomingPlaylist::manager() const
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
    bool loop = action("loopPlaylist") && action<KToggleAction>("loopPlaylist")->isChecked();

    PlaylistItem *item = m_playlist->firstChild();
    if(item) {
        if(loop)
            m_playlist->addNewItem();

        m_playlist->setPlaying(0, false);
        m_playlist->clearItem(item);
    }
    else
        m_playlist->fillList();

    setCurrent(m_playlist->firstChild());
}

void UpcomingPlaylist::UpcomingSequenceIterator::backup()
{
}

UpcomingPlaylist::UpcomingSequenceIterator *UpcomingPlaylist::UpcomingSequenceIterator::clone() const
{
    return new UpcomingSequenceIterator(*this);
}

void UpcomingPlaylist::UpcomingSequenceIterator::setCurrent(PlaylistItem *currentItem)
{
    if(!currentItem) {
        TrackSequenceIterator::setCurrent(currentItem);
        return;
    }

    // If the upcoming playlist is playing something, clear it out since
    // apparently the user didn't want to hear it.

    PlaylistItem *playingItem = m_playlist->playingItem();
    if(playingItem && playingItem->playlist() == m_playlist && currentItem != playingItem) {
        if(action<KToggleAction>("loopPlaylist")->isChecked())
            m_playlist->addNewItem();

        m_playlist->clearItem(playingItem);
    }

    // If a different playlist owns this item, add it to the upcoming playlist

    Playlist *p = currentItem->playlist();

    if(p != m_playlist) {
        PlaylistItemList list;
        list.append(currentItem);
        m_playlist->createItems(list);
    }
    else {

        // Bump this item up to the top

        m_playlist->takeItem(currentItem);
        m_playlist->insertItem(currentItem);
    }

    TrackSequenceIterator::setCurrent(m_playlist->firstChild());
}

void UpcomingPlaylist::UpcomingSequenceIterator::reset()
{
    setCurrent(0);
}

void UpcomingPlaylist::UpcomingSequenceIterator::prepareToPlay(Playlist *)
{
    if(m_playlist->items().isEmpty())
        m_playlist->fillList();
    setCurrent(m_playlist->firstChild());
}

QDataStream &operator<<(QDataStream &s, const UpcomingPlaylist &p)
{
    PlaylistItemList l = const_cast<UpcomingPlaylist *>(&p)->items();

    s << Q_INT32(l.count());

    for(PlaylistItemList::ConstIterator it = l.begin(); it != l.end(); ++it)
        s << (*it)->file().absFilePath();

    return s;
}

QDataStream &operator>>(QDataStream &s, UpcomingPlaylist &p)
{
    QString fileName;
    PlaylistItem *newItem = 0;
    Q_INT32 count;

    s >> count;

    for(Q_INT32 i = 0; i < count; ++i) {
        s >> fileName;
        newItem = p.createItem(FileHandle(fileName), newItem, false);
    }

    return s;
}

// vim: set et ts=4 sw=4:
