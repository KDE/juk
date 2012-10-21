#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QTreeView>

#include "playlist/playlists/playlist.h"

class PlaylistSortFilterProxyModel;
class PlaylistView : public QTreeView
{
    Q_OBJECT
public:
    PlaylistView(QWidget *parent);

    virtual void setModel(QAbstractItemModel *model);
    virtual QAbstractItemModel *model();
    void setSearch(const PlaylistSearch &s);
    void setSearchEnabled(bool);
    
public slots:
    void copy();
    void paste();
protected slots:
    virtual void contextMenuEvent(QContextMenuEvent*);
    virtual bool event(QEvent*);
    virtual void play(QModelIndex);

private slots:
    void slotRefresh();
    
private:
    Playlist *playlist() { return qobject_cast<Playlist*>(model()); }
    void decode(const QMimeData* s, const QModelIndex& after);
    
    
    KMenu *m_contextMenu;
    QAction *m_editAction;
    int m_currentColumn;
    PlaylistSortFilterProxyModel *m_proxyModel;
// private slots:
//     void slotPopulateBackMenu() const;
//     void slotPlayFromBackMenu(QAction *) const;
};

#endif//PLAYLISTVIEW_H