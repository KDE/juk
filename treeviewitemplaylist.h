#ifndef TREEVIEWITEMPLAYLIST_H
#define TREEVIEWITEMPLAYLIST_H

#include "searchplaylist.h"
#include "playlistitem.h"

class QStringList;

class TreeViewItemPlaylist : public SearchPlaylist
{
    Q_OBJECT

public:
    TreeViewItemPlaylist(PlaylistCollection *collection,
                         const PlaylistSearch &search = PlaylistSearch(),
                         const QString &name = QString::null,
                         bool setupPlaylist = true);

    void retag(const QStringList &files, Playlist *donorPlaylist);

signals:
    void signalTagsChanged();

private:
    PlaylistItem::ColumnType m_columnType;
};

#endif // TREEVIEWITEMPLAYLIST_H
