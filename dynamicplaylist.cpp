/***************************************************************************
                          dynamicplaylist.h
                             -------------------
    begin                : Mon May 5 2003
    copyright            : (C) 2003 by Scott Wheeler
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

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DynamicPlaylist::DynamicPlaylist(const PlaylistList &playlists, QWidget *parent, const QString &name) :
    Playlist(parent, name),
    m_playlists(playlists),
    m_dirty(true)
{
    setSorting(columns() + 1);

    for(PlaylistList::ConstIterator it = m_playlists.begin(); it != m_playlists.end(); ++it) {
        if(*it) {
            connect(*it, SIGNAL(signalDataChanged()), this, SLOT(slotSetDirty()));
            connect(*it, SIGNAL(signalCountChanged(Playlist *)), this, SLOT(slotSetDirty()));
        }
        else
            m_playlists.remove(*it);
    }
    connect(CollectionList::instance(), SIGNAL(signalCollectionChanged()), this, SLOT(slotSetDirty()));
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
}

#include "dynamicplaylist.moc"
