#include "playlistsortfilterproxymodel.h"


PlaylistSortFilterProxyModel* PlaylistSortFilterProxyModel::instance()
{
    static PlaylistSortFilterProxyModel inst;
    return &inst;
}



PlaylistSortFilterProxyModel::PlaylistSortFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

void PlaylistSortFilterProxyModel::setSearch(const PlaylistSearch& search)
{
    if (!m_searchEnabled)
        return;
    
    m_search = search;
}

bool PlaylistSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (!m_searchEnabled)
        return true;
    
    // set our default
    bool match = bool(m_search.mode());

    Q_FOREACH (const PlaylistSearch::Component &component, m_search.components()) {
        bool componentMatches = true;
        if (!component.isEmpty()) {
            ColumnList columns = component.columns();
            if (columns.isEmpty()) {
                for(int i=0; i<sourceParent.model()->columnCount(); i++)
                    columns.append(i);
//                 component.setColumns(columns);
            }
            
            Q_FOREACH(int column, columns) {
                const QModelIndex &index = sourceModel()->index(sourceRow, column, sourceParent);
                QString text = sourceModel()->data(index).toString();
                if (!component.matches(text)) {
                    componentMatches = false;
                    break;
                }
            }

            
            
        }

        if(componentMatches && m_search.mode() == PlaylistSearch::MatchAny) {
            match = true;
            break;
        }

        if(!componentMatches && m_search.mode() == PlaylistSearch::MatchAll) {
            match = false;
            break;
        }
    }

    return match;
}

void PlaylistSortFilterProxyModel::setSearchEnabled(bool enable)
{
    m_searchEnabled = enable;
}
