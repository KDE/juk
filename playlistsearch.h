/***************************************************************************
                          playlistsearch.h
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
#include <qregexp.h>

class Playlist;
typedef QValueList<Playlist *> PlaylistList;
class PlaylistItem;
typedef QValueList<PlaylistItem *> PlaylistItemList;
typedef QValueList<int> ColumnList;

class PlaylistSearch
{
public:
    class Component;
    typedef QValueList<Component> ComponentList;

    enum SearchMode { MatchAny = 0, MatchAll = 1 };

    PlaylistSearch();
    PlaylistSearch(const PlaylistSearch &search);
    PlaylistSearch(const PlaylistList &playlists,
		   const ComponentList &components,
		   SearchMode mode = MatchAny);


    PlaylistItemList searchedItems() const { return m_items; }
    PlaylistItemList matchedItems() const { return m_matchedItems; }
    PlaylistItemList unmatchedItems() const { return m_unmatchedItems; }

    PlaylistList playlists() const { return m_playlists; }

    ComponentList components() const { return m_components; }

    bool isNull() const { return m_components.isEmpty(); }

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
     * Create an empty search component.  This is only provided for use by 
     * QValueList and should not be used in any other context.
     */
    Component();

    /**
     * Copy constructor.
     */
    Component(const Component &component);

    /**
     * Create a query component.  This defaults to searching all visible coulumns.
     */
    Component(const QString &query, bool caseSensitive = false, const ColumnList &columns = ColumnList());

    /**
     * Create a query component.  This defaults to searching all visible coulumns.
     */
    Component(const QRegExp &query, const ColumnList &columns = ColumnList());

    QString query() const { return m_query; }
    QRegExp pattern() const { return m_queryRe; }
    ColumnList columns() const { return m_columns; }

    bool matches(PlaylistItem *item);
    bool isPatternSearch() const { return m_re; }
    bool isCaseSensitive() const { return m_caseSensitive; }
    
private:
    QString m_query;
    QRegExp m_queryRe;
    ColumnList m_columns;
    bool m_searchAllVisible;
    bool m_caseSensitive;
    bool m_re;
};


#endif
