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

#include <kdebug.h>

#include "playlistsearch.h"
#include "playlist.h"
#include "playlistitem.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSearch::PlaylistSearch() :
    m_mode(MatchAny)
{

}

PlaylistSearch::PlaylistSearch(const PlaylistList &playlists,
			       const ComponentList &components,
			       SearchMode mode,
			       bool searchNow) :
    m_playlists(playlists),
    m_components(components),
    m_mode(mode)
{
    if(searchNow)
	search();
}

bool PlaylistSearch::isEmpty() const
{
    if(isNull())
	return true;

    ComponentList::ConstIterator it = m_components.begin();
    for(; it != m_components.end(); ++it) {
	if(!(*it).query().isEmpty() || !(*it).pattern().isEmpty())
	    return false;
    }

    return true;
}

void PlaylistSearch::search()
{
    m_items.clear();
    m_matchedItems.clear();
    m_unmatchedItems.clear();

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

		bool componentMatches = (*componentIt).matches(item);

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

////////////////////////////////////////////////////////////////////////////////
// Component public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSearch::Component::Component() :
    m_searchAllVisible(true),
    m_caseSensitive(false)
{

}

PlaylistSearch::Component::Component(const QString &query, bool caseSensitive, const ColumnList &columns) :
    m_query(query),
    m_columns(columns),
    m_searchAllVisible(columns.isEmpty()),
    m_caseSensitive(caseSensitive),
    m_re(false)
{

}

PlaylistSearch::Component::Component(const QRegExp &query, const ColumnList& columns) :
    m_queryRe(query),
    m_columns(columns),
    m_searchAllVisible(columns.isEmpty()),
    m_caseSensitive(false),
    m_re(true)
{

}

bool PlaylistSearch::Component::matches(PlaylistItem *item)
{
    if((m_re && m_queryRe.isEmpty()) || (!m_re && m_query.isEmpty()))
	return false;

    if(m_columns.isEmpty()) {
	Playlist *p = static_cast<Playlist *>(item->listView());
	for(int i = 0; i < p->columns(); i++) {
	    if(p->isColumnVisible(i))
		m_columns.append(i);
	}
    }


    for(ColumnList::Iterator it = m_columns.begin(); it != m_columns.end(); ++it) {
        int matches = m_re ? item->text(*it).contains(m_queryRe)
                           : item->text(*it).contains(m_query, m_caseSensitive);
        if(matches > 0)
            return true;
    }

    return false;
}
