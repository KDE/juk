/***************************************************************************
                          playlistsearch.cpp
                             -------------------
    begin                : Sun Mar 6 2003
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

#include "playlistsearch.h"

PlaylistSearch::PlaylistSearch(const PlaylistList &playlists, 
			       const ComponentList &components, 
			       SearchMode mode) :
    m_playlists(playlists), 
    m_components(components), 
    m_mode(mode)
{
    search();
}

void PlaylistSearch::search()
{

    // This really isn't as bad as it looks.  Despite the four nexted loops
    // most of the time this will be searching one playlist for one search 
    // component -- possibly for one column.

    // Also there should be some caching of previous searches in here and
    // allowance for appending and removing chars.  If one is added it
    // should only search the current list.  If one is removed it should
    // pop the previous search results off of a stack.

    PlaylistList::Iterator playlistIt = m_playlists.begin();
    for(; playlistIt != m_playlists.end(); ++playlistIt) {

	for(QListViewItemIterator it(*playlistIt); it.current(); ++it) {

	    PlaylistItem *item = static_cast<PlaylistItem *>(*it);

	    m_items.append(item);

	    // set our default
	    bool match = bool(m_mode);

	    ComponentList::Iterator componentIt = m_components.begin();
	    for(; componentIt != m_components.end(); ++componentIt) {

		bool componentMatches = (*componentIt)->matches(item);

		if(componentMatches && m_mode == MatchAny) {
		    match = true;
		    break;
		}

		if(!componentMatches && m_mode == MatchAll) {
		    match = false;
		    break;
		}
	    }

	    if(match)
		m_matchedItems.append(item);
	    else
		m_unmatchedItems.append(item);
	}
    }
}

PlaylistSearch::Component::Component(const QString &query, const ColumnList &columns, bool caseSensitive) :
    m_query(query), 
    m_columns(columns), 
    m_searchAllVisible(columns.isEmpty()),
    m_caseSensitive(caseSensitive)
{

}

bool PlaylistSearch::Component::matches(PlaylistItem *item)
{
    if(m_query.isEmpty())
	return false;

    if(m_columns.isEmpty()) {
	Playlist *p = static_cast<Playlist *>(item->listView());
	for(int i = 0; i < PlaylistItem::lastColumn(); i++) {
	    if(p->isColumnVisible(i))
		m_columns.append(PlaylistItem::ColumnType(i));
	}
    }


    for(ColumnList::Iterator it = m_columns.begin(); it != m_columns.end(); ++it) {
	if(item->text(*it).contains(m_query, m_caseSensitive))
	    return true;
    }
    
    return false;
}
