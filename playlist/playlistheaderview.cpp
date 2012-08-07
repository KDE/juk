#include "playlistheaderview.h"
#include "playlist/playlists/playlist.h"
#include <KMenu>
#include <KLocalizedString>
#include <QContextMenuEvent>
#include <KConfigGroup>
#include <KGlobal>

PlaylistHeaderView::PlaylistHeaderView(Qt::Orientation orientation, QWidget* parent):
    QHeaderView(orientation, parent),
    m_contextMenu(new KMenu(i18n("Show"), this))
{
    setResizeMode(QHeaderView::ResizeToContents);
    setMovable(true);
    
    connect(m_contextMenu, SIGNAL(triggered(QAction*)), SLOT(slotToggleColumnVisible(QAction*)));
}

PlaylistHeaderView::~PlaylistHeaderView()
{
    delete m_contextMenu;
    
    if (model())
        saveConfig();
}    

void PlaylistHeaderView::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "HeaderView");
    config.writeEntry("State", saveState());
}

void PlaylistHeaderView::setModel(QAbstractItemModel* model)
{
    QHeaderView::setModel(model);
    
    // We need to do this after the model is set
    KConfigGroup config(KGlobal::config(), "HeaderView");
    restoreState(config.readEntry("State", QByteArray()));

}

void PlaylistHeaderView::contextMenuEvent(QContextMenuEvent* event)
{
    QAction *showAction;
    m_contextMenu->clear();

    for(int i = 0; i < count(); ++i) {
        if(i == Playlist::FileNameColumn)
            m_contextMenu->addSeparator();

        showAction = new QAction(model()->headerData(i, orientation()).toString(), m_contextMenu);
        showAction->setData(i);
        showAction->setCheckable(true);
        showAction->setChecked(!isSectionHidden(i));
        m_contextMenu->addAction(showAction);
    }

    m_contextMenu->exec(event->globalPos());
}

void PlaylistHeaderView::slotToggleColumnVisible(QAction *action)
{
    int column = action->data().toInt();

    if(!isSectionHidden(column)) {
        if(column == Playlist::FileNameColumn && !isSectionHidden(Playlist::FullPathColumn)) {
            showSection(Playlist::FullPathColumn);
        }
        if(column == Playlist::FullPathColumn && !isSectionHidden(Playlist::FileNameColumn)) {
            showSection(Playlist::FileNameColumn);
        }
    }

    if(isSectionHidden(column))
        showSection(column);
    else
        hideSection(column);
    
    saveConfig();
}
