/***************************************************************************
                          playlistsearch.h  -  description
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

#ifndef PLAYLISTSEARCH_H
#define PLAYLISTSEARCH_H

#include <qvaluelist.h>

#include "playlist.h"
#include "playlistitem.h"

typedef QValueList<PlaylistItem::ColumnType> ColumnList;

class PlaylistSearch
{
public:
    class Component;
    typedef QValueList<Component *> ComponentList;

    enum SearchMode { MatchAny = 0, MatchAll = 1 };

    PlaylistSearch(const PlaylistList &playlists,
		   const ComponentList &components,
		   SearchMode mode = MatchAny);


    PlaylistItemList searchedItems() const { return m_items; }
    PlaylistItemList matchedItems() const { return m_matchedItems; }
    PlaylistItemList unmatchedItems() const { return m_unmatchedItems; }
    
private:
    void search();

    PlaylistList m_playlists;
    ComponentList m_components;
    SearchMode m_mode;

    PlaylistItemList m_items;
    PlaylistItemList m_matchedItems;
    PlaylistItemList m_unmatchedItems;
};

/**
 * A search is built from several search components.  These corespond to to lines
 * in the search bar.
 */

class PlaylistSearch::Component
{
public:
    /**
     * Create a query component.  This defaults to searching all visible coulumns.
     */
    Component(const QString &query, const ColumnList &columns = ColumnList(), bool caseSensitive = false);

    QString query() const { return m_query; }
    ColumnList columns() const { return m_columns; }

    bool matches(PlaylistItem *item);
    
private:
    QString m_query;
    ColumnList m_columns;
    bool m_searchAllVisible;
    bool m_caseSensitive;
};


#endif
