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

#include <kconfig.h>
#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>

#include "actioncollection.h"
#include "tracksequencemanager.h"
#include "playlist.h"
#include "playlistitem.h"
#include "tracksequenceiterator.h"
#include "tag.h"
#include "filehandle.h"
#include "collectionlist.h"

/////////////////////////////////////////////////////////////////////////////
// public functions
/////////////////////////////////////////////////////////////////////////////

TrackSequenceManager::~TrackSequenceManager()
{
    // m_playlist and m_popupMenu don't belong to us, don't try to delete them
    if(m_iterator == m_defaultIterator)
        m_iterator = 0;

    delete m_iterator;
    delete m_defaultIterator;
}

bool TrackSequenceManager::installIterator(TrackSequenceIterator *iterator)
{
    PlaylistItem *oldItem = m_iterator ? m_iterator->current() : 0;
    
    if(m_iterator != m_defaultIterator)
        delete m_iterator;

    m_iterator = m_defaultIterator;
    if(iterator)
        m_iterator = iterator;

    m_iterator->setCurrent(oldItem);

    return true;
}

PlaylistItem *TrackSequenceManager::currentItem() const
{
    return m_iterator->current();
}

TrackSequenceIterator *TrackSequenceManager::takeIterator()
{
    TrackSequenceIterator *temp = m_iterator;

    m_iterator = 0;
    return temp;
}

TrackSequenceManager *TrackSequenceManager::instance()
{
    static TrackSequenceManager manager;

    if(!manager.m_initialized)
        manager.initialize();

    return &manager;
}

PlaylistItem *TrackSequenceManager::nextItem()
{
    if(m_playNextItem) {
        
        // Force the iterator to reset state (such as random item lists)

        m_iterator->reset();        
        m_iterator->setCurrent(m_playNextItem);
        m_playNextItem = 0;
    }
    else if(m_iterator->current())
        m_iterator->advance();
    else if(currentPlaylist())
        m_iterator->prepareToPlay(currentPlaylist());
    else
        m_iterator->prepareToPlay(CollectionList::instance());

    return m_iterator->current();
}

PlaylistItem *TrackSequenceManager::previousItem()
{
    m_iterator->backup();
    return m_iterator->current();
}

/////////////////////////////////////////////////////////////////////////////
// public slots
/////////////////////////////////////////////////////////////////////////////

void TrackSequenceManager::setNextItem(PlaylistItem *item)
{
    m_playNextItem = item;
}

void TrackSequenceManager::setCurrentPlaylist(Playlist *list)
{
    if(m_playlist)
        m_playlist->disconnect(this);
    m_playlist = list;

    connect(m_playlist, SIGNAL(signalAboutToRemove(PlaylistItem *)),
            this,       SLOT(slotItemAboutToDie(PlaylistItem *)));
}

void TrackSequenceManager::setCurrent(PlaylistItem *item)
{
    if(item != m_iterator->current()) {
        m_iterator->setCurrent(item);
        if(item)
        setCurrentPlaylist(item->playlist());
    else
            m_iterator->reset();        
    }
}

/////////////////////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////////////////////

void TrackSequenceManager::initialize()
{
    CollectionList *collection = CollectionList::instance();

    if(!collection)
        return;

    // Make sure we don't use m_playNextItem if it's invalid.
    connect(collection, SIGNAL(signalAboutToRemove(PlaylistItem *)),
            this, SLOT(slotItemAboutToDie(PlaylistItem *)));

    m_initialized = true;
}

TrackSequenceManager::TrackSequenceManager() :
    QObject(),
    m_playlist(0),
    m_playNextItem(0),
    m_popupMenu(0),
    m_iterator(0),
    m_initialized(false)
{
    m_defaultIterator = new DefaultSequenceIterator();
    m_iterator = m_defaultIterator;
}

/////////////////////////////////////////////////////////////////////////////
// protected slots
/////////////////////////////////////////////////////////////////////////////

void TrackSequenceManager::slotItemAboutToDie(PlaylistItem *item)
{
    if(item == m_playNextItem)
        m_playNextItem = 0;

    m_iterator->itemAboutToDie(item);
}

#include "tracksequencemanager.moc"

// vim: set et sw=4 tw=0:
