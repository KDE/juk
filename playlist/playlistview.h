#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QTreeView>

#include "playlist/playlists/playlist.h"

class PlaylistView : public QTreeView
{
    Q_OBJECT
public:
    PlaylistView(QWidget *parent);

    
public slots:
    void copy();
    void paste();
protected slots:
    virtual void contextMenuEvent(QContextMenuEvent*);
    virtual bool event(QEvent*);

private slots:
    void slotRefresh();
    
private:
    Playlist *playlist() { return qobject_cast<Playlist*>(model()); }
    void decode(const QMimeData* s, const QModelIndex& after);
    
    
    KMenu *m_contextMenu;
    QAction *m_editAction;
    int m_currentColumn;
// private slots:
//     void slotPopulateBackMenu() const;
//     void slotPlayFromBackMenu(QAction *) const;
};

#endif//PLAYLISTVIEW_H