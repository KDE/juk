/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <QConcatenateTablesProxyModel>

#include "playlistsearch.h"
#include "playlist.h"
#include "playlistitem.h"
#include "collectionlist.h"
#include "juk-exception.h"

#include "juk_debug.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSearch::PlaylistSearch(QObject* parent) :
    QSortFilterProxyModel(parent),
    m_mode(MatchAny)
{

}

PlaylistSearch::PlaylistSearch(const PlaylistList &playlists,
                               const ComponentList &components,
                               SearchMode mode,
                               QObject* parent) :
    QSortFilterProxyModel(parent),
    m_playlists(playlists),
    m_components(components),
    m_mode(mode)
{
    QConcatenateTablesProxyModel* const model = new QConcatenateTablesProxyModel(this);
    for(Playlist* playlist : playlists)
        model->addSourceModel(playlist->model());
    setSourceModel(model);
}

bool PlaylistSearch::checkItem(QModelIndex *item)
{
    return mapFromSource(static_cast<QConcatenateTablesProxyModel*>(sourceModel())->mapFromSource(*item)).isValid();
}

QModelIndexList PlaylistSearch::matchedItems() const{
    QModelIndexList res;
    for(int row = 0; row < rowCount(); ++row)
        res.append(mapToSource(index(row, 0)));
    return res;
}

void PlaylistSearch::addPlaylist(Playlist* p)
{
    static_cast<QConcatenateTablesProxyModel*>(sourceModel())->addSourceModel(p->model());
    m_playlists.append(p);
}

void PlaylistSearch::clearPlaylists()
{
    setSourceModel(new QConcatenateTablesProxyModel(this));
    m_playlists.clear();
}


void PlaylistSearch::addComponent(const Component &c)
{
    m_components.append(c);
    invalidateFilter();
}

void PlaylistSearch::clearComponents()
{
    m_components.clear();
    invalidateFilter();
}

PlaylistSearch::ComponentList PlaylistSearch::components() const
{
    return m_components;
}

bool PlaylistSearch::isNull() const
{
    return m_components.isEmpty();
}

bool PlaylistSearch::isEmpty() const
{
    if(isNull())
        return true;

    for(const auto &component : m_components) {
        if(!component.query().isEmpty() || !component.pattern().pattern().isEmpty())
            return false;
    }

    return true;
}

bool PlaylistSearch::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const{
    QAbstractItemModel* const model = sourceModel();
    auto matcher = [&](Component c){
        return c.matches(source_row, source_parent, model);
    };
    return m_mode == MatchAny? std::any_of(m_components.begin(), m_components.end(), matcher) :
        std::all_of(m_components.begin(), m_components.end(), matcher);
}

////////////////////////////////////////////////////////////////////////////////
// Component public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSearch::Component::Component() :
    m_mode(Contains),
    m_searchAllVisible(true),
    m_caseSensitive(false)
{

}

PlaylistSearch::Component::Component(const QString &query,
                                     bool caseSensitive,
                                     const ColumnList &columns,
                                     MatchMode mode) :
    m_query(query),
    m_columns(columns),
    m_mode(mode),
    m_searchAllVisible(columns.isEmpty()),
    m_caseSensitive(caseSensitive),
    m_re(false)
{

}

PlaylistSearch::Component::Component(const QRegularExpression &query, const ColumnList& columns) :
    m_queryRe(query),
    m_columns(columns),
    m_mode(Exact),
    m_searchAllVisible(columns.isEmpty()),
    m_caseSensitive(false),
    m_re(true)
{

}

bool PlaylistSearch::Component::matches(int row, QModelIndex parent, QAbstractItemModel* model) const
{
    for(int column : std::as_const(m_columns)) {
        const QString str = model->index(row, column, parent).data().toString();
        if(m_re) {
            return str.contains(m_queryRe);
        }

        switch(m_mode) {
        case Contains:
            if(str.contains(m_query, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive))
                return true;
            break;
        case Exact:
            // If lengths match, move on to check strings themselves
            if(str.length() == m_query.length() &&
               (( m_caseSensitive && str == m_query) ||
                (!m_caseSensitive && str.toLower() == m_query.toLower()))
               )
            {
                return true;
            }
            break;
        case ContainsWord:
        {
            int i = str.indexOf(m_query, 0, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

            if(i >= 0) {

                // If we found the pattern and the lengths are the same, then
                // this is a match.

                if(str.length() == m_query.length())
                    return true;

                // First: If the match starts at the beginning of the text or the
                // character before the match is not a word character

                // AND

                // Second: Either the pattern was found at the end of the text,
                // or the text following the match is a non-word character

                // ...then we have a match

                if((i == 0 || !str.at(i - 1).isLetterOrNumber()) &&
                    (i + m_query.length() == str.length() || !str.at(i + m_query.length()).isLetterOrNumber()))
                    return true;
            }
        }
        }
    };
    return false;
}

bool PlaylistSearch::Component::operator==(const Component &v) const
{
    return m_query == v.m_query &&
        m_queryRe == v.m_queryRe &&
        m_columns == v.m_columns &&
        m_mode == v.m_mode &&
        m_searchAllVisible == v.m_searchAllVisible &&
        m_caseSensitive == v.m_caseSensitive &&
        m_re == v.m_re;
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const PlaylistSearch &search)
{
    s << search.components()
      << qint32(search.searchMode());

    return s;
}

QDataStream &operator>>(QDataStream &s, PlaylistSearch &search)
{
    search.clearPlaylists();
    search.addPlaylist(CollectionList::instance());

    search.clearComponents();
    PlaylistSearch::ComponentList components;
    s >> components;
    for(const auto &component : components)
        search.addComponent(component);

    qint32 mode;
    s >> mode;
    search.setSearchMode(PlaylistSearch::SearchMode(mode));

    return s;
}

QDataStream &operator<<(QDataStream &s, const PlaylistSearch::Component &c)
{
    s << c.isPatternSearch()
      << (c.isPatternSearch() ? c.pattern().pattern() : c.query())
      << c.isCaseSensitive()
      << c.columns()
      << qint32(c.matchMode());

    return s;
}

QDataStream &operator>>(QDataStream &s, PlaylistSearch::Component &c)
{
    bool patternSearch;
    QString pattern;
    bool caseSensitive;
    ColumnList columns;
    qint32 mode;

    s >> patternSearch
      >> pattern
      >> caseSensitive
      >> columns
      >> mode;

    if(patternSearch)
        c = PlaylistSearch::Component(QRegularExpression(pattern), columns);
    else
        c = PlaylistSearch::Component(pattern, caseSensitive, columns, PlaylistSearch::Component::MatchMode(mode));

    return s;
}

// vim: set et sw=4 tw=0 sta:
