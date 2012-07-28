#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QTreeView>

class PlaylistView : public QTreeView
{
    Q_OBJECT
public:
    PlaylistView(QWidget *parent);
    
protected slots:
    void slotPopulateBackMenu() const;
    void slotPlayFromBackMenu(QAction *) const;
};

#endif//PLAYLISTVIEW_H