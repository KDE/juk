#ifndef PLAYLISTSORTFILTERPROXYMODEL_H
#define PLAYLISTSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "playlist/playlistsearch.h"

class PlaylistSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    static PlaylistSortFilterProxyModel *instance();
    
    void setSearch(const PlaylistSearch&);
    void setSearchEnabled(bool);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    
private:
    PlaylistSortFilterProxyModel(QObject *parent=0);
    PlaylistSortFilterProxyModel(const QSortFilterProxyModel& );
    
    PlaylistSearch m_search;
    bool m_searchEnabled;
};

#endif // PLAYLISTSORTFILTERPROXYMODEL_H
