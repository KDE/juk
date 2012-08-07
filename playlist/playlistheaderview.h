#ifndef PLAYLISTHEADERVIEW_H
#define PLAYLISTHEADERVIEW_H

#include <QtGui/QHeaderView>

class KMenu;

class PlaylistHeaderView : public QHeaderView
{
    Q_OBJECT
    
public:
    explicit PlaylistHeaderView(Qt::Orientation orientation, QWidget* parent = 0);
    virtual ~PlaylistHeaderView();

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);
    virtual void setModel (QAbstractItemModel * model);
    
protected slots:
    void slotToggleColumnVisible(QAction *action);
    
private slots:
    void saveConfig();
    
private:
    KMenu *m_contextMenu;
};

#endif // PLAYLISTHEADERVIEW_H
