/***************************************************************************
                          searchplaylist.h
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

#include "searchplaylist.h"
#include "playlistitem.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SearchPlaylist::SearchPlaylist(const PlaylistSearch &search, QWidget *parent, const QString &name) :
    DynamicPlaylist(search.playlists(), parent, name),
    m_search(search)
{

}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

void SearchPlaylist::updateItems()
{
    // Here we don't simply use "clear" since that would involve a call to
    // items() which would in turn call this method...

    clearItems(Playlist::items());
    m_search.search();
    createItems(m_search.matchedItems());
}

#include "searchplaylist.moc"
