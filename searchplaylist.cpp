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

#include "searchplaylist.h"

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

PlaylistItemList SearchPlaylist::items() const
{
    return m_search.matchedItems();
}
