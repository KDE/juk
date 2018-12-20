/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYLISTBOX_H
#define PLAYLISTBOX_H

#include "playlistcollection.h"

#include <QHash>
#include <QTreeWidget>

class Playlist;
class PlaylistItem;
class ViewMode;

class QMenu;

template<class T>
class QVector;

typedef QVector<Playlist *> PlaylistList;

/**
 * This is the play list selection box that is by default on the left side of
 * JuK's main widget (PlaylistSplitter).
 */

class PlaylistBox : public QTreeWidget, public PlaylistCollection
{
    Q_OBJECT

public:
    class Item;
    typedef QList<Item *> ItemList;

    friend class Item;

    PlaylistBox(PlayerManager *player, QWidget *parent, QStackedWidget *playlistStack);
    virtual ~PlaylistBox();

    virtual void raise(Playlist *playlist);
    virtual void duplicate();
    virtual void remove();

    // Called after files loaded to pickup any new files that might be present
    // in managed directories.
    virtual void scanFolders();

    /**
     * For view modes that have dynamic playlists, this freezes them from
     * removing playlists.
     */
    virtual void setDynamicListsFrozen(bool frozen);

    Item *dropItem() const { return m_dropItem; }

    void setupPlaylist(Playlist *playlist, const QString &iconName, Item *parentItem = nullptr);

public slots:
    void paste();
    void clear() {}

    void slotFreezePlaylists();
    void slotUnfreezePlaylists();
    void slotPlaylistDataChanged();
    void slotSetHistoryPlaylistEnabled(bool enable);

protected:
    virtual void setupPlaylist(Playlist *playlist, const QString &iconName);
    virtual void removePlaylist(Playlist *playlist);

signals:
    void signalPlaylistDestroyed(Playlist *);
    void signalMoveFocusAway(); // Handles keyboard scrolling up out of playlist
    void startupComplete(); ///< Emitted after playlists are loaded.
    void startFilePlayback(const FileHandle &file);

private:
    void readConfig();
    void saveConfig();

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);

    // selectedItems already used for something different

    ItemList selectedBoxItems() const;
    void setSingleItem(QTreeWidgetItem *item);

    void setupItem(Item *item);
    void setupUpcomingPlaylist();
    int viewModeIndex() const { return m_viewModeIndex; }
    ViewMode *viewMode() const { return m_viewModes[m_viewModeIndex]; }

private slots:
    /**
     * Catches QListBox::currentChanged(QListBoxItem *), does a cast and then re-emits
     * the signal as currentChanged(Item *).
     */
    void slotPlaylistChanged();
    void slotDoubleClicked(QTreeWidgetItem *);
    void slotShowContextMenu(const QPoint &point);
    void slotSetViewMode(int index);
    void slotSavePlaylists();
    void slotShowDropTarget();

    void slotPlaylistItemsDropped(Playlist *p);

    void slotAddItem(const QString &tag, unsigned column);
    void slotRemoveItem(const QString &tag, unsigned column);

    // Used to load the playlists after GUI setup.
    void slotLoadCachedPlaylists();

private:
    QMenu *m_contextMenu;
    QHash<Playlist *, Item*> m_playlistDict;
    int m_viewModeIndex;
    QList<ViewMode *> m_viewModes;
    bool m_hasSelection;
    bool m_doingMultiSelect;
    Item *m_dropItem;
    QTimer *m_showTimer;
    QTimer *m_savePlaylistTimer;
};

class PlaylistBox::Item : public QObject, public QTreeWidgetItem, public PlaylistObserver
{
    friend class PlaylistBox;
    friend class PlaylistSplitter;
    friend class ViewMode;
    friend class CompactViewMode;
    friend class TreeViewMode;

    Q_OBJECT

    // moc won't let me create private QObject subclasses and Qt won't let me
    // make the destructor protected, so here's the closest hack that will
    // compile.

public:
    virtual ~Item();

protected:
    Item(PlaylistBox *listBox, const QString &icon, const QString &text, Playlist *l = 0);
    Item(Item *parent, const QString &icon, const QString &text, Playlist *l = 0);

    Playlist *playlist() const { return m_playlist; }
    PlaylistBox *listView() const { return static_cast<PlaylistBox *>(QTreeWidgetItem::treeWidget()); }
    QString iconName() const { return m_iconName; }
    QString text() const { return m_text; }
    void setSortedFirst(bool first = true) { m_sortedFirst = first; }

    virtual int compare(QTreeWidgetItem *i, int col, bool) const;
    /*virtual void paintCell(QPainter *p, const QColorGroup &colorGroup, int column, int width, int align);
    virtual void paintFocus(QPainter *, const QColorGroup &, const QRect &) {}*/
    virtual void setText(int column, const QString &text);

    virtual QString text(int column) const { return QTreeWidgetItem::text(column); }

    virtual void setup();

    static Item *collectionItem() { return m_collectionItem; }
    static void setCollectionItem(Item *item) { m_collectionItem = item; }

    //
    // Reimplemented from PlaylistObserver
    //

    virtual void playingItemHasChanged() Q_DECL_FINAL;

    // Used to post a timer in PlaylistBox to save playlists.
    virtual void playlistItemDataHasChanged() Q_DECL_FINAL;


protected slots:
    void slotSetName(const QString &name);

private:
    // setup() was already taken.
    void init();

    Playlist *m_playlist;
    QString m_text;
    QString m_iconName;
    bool m_sortedFirst;
    static Item *m_collectionItem;
};

#endif

// vim: set et sw=4 tw=0 sta:
