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

#include <kdebug.h>

#include "dynamicplaylist.h"
#include "collectionlist.h"
#include "playlistcollection.h"
#include "tracksequencemanager.h"

class PlaylistDirtyObserver : public PlaylistObserver
{
public:
    PlaylistDirtyObserver(DynamicPlaylist *parent, Playlist *playlist) :
	PlaylistObserver(playlist),
        m_parent(parent) 
    {

    }
    virtual void updateData() { m_parent->slotSetDirty(); }
    virtual void updateCurrent() {}

private:
    DynamicPlaylist *m_parent;
};

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

    setSorting(columns() + 1);

    for(PlaylistList::ConstIterator it = playlists.begin(); it != playlists.end(); ++it)
        m_observers.append(new PlaylistDirtyObserver(this, *it));

    connect(CollectionList::instance(), SIGNAL(signalCollectionChanged()), this, SLOT(slotSetDirty()));
}

DynamicPlaylist::~DynamicPlaylist()
{
    lower();

    for(QValueList<PlaylistObserver *>::ConstIterator it = m_observers.begin();
        it != m_observers.end();
        ++it)
    {
        delete *it;
    }
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
            list.remove(it);
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

    for(PlaylistList::ConstIterator it = m_playlists.begin(); it != m_playlists.end(); ++it)
        siblings += (*it)->items();


    PlaylistItemList newSiblings = siblings;
    if(m_siblings != newSiblings) {
        m_siblings = newSiblings;
        QTimer::singleShot(0, this, SLOT(slotUpdateItems()));
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

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void DynamicPlaylist::slotUpdateItems()
{
    // This should be optimized to check to see which items are already in the
    // list and just adding those and removing the ones that aren't.

    clear();
    createItems(m_siblings);
    if(m_synchronizePlaying)
        synchronizePlayingItems(m_playlists, true);
}

#include "dynamicplaylist.moc"
