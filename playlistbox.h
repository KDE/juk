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
#include <QVector>
#include <QTreeWidget>

class Playlist;
class PlaylistItem;
class ViewMode;

class QMenu;

typedef QVector<Playlist *> PlaylistList;

/**
 * This is the play list selection box that is by default on the left side of
 * JuK's main widget (PlaylistSplitter).
 */

class PlaylistBox final : public QTreeWidget, public PlaylistCollection
{
    Q_OBJECT

public:
    class Item;
    typedef QVector<Item *> ItemList;

    friend class Item;

    PlaylistBox(PlayerManager *player, QWidget *parent, QStackedWidget *playlistStack);
    virtual ~PlaylistBox();

    virtual void raise(Playlist *playlist) override;
    virtual void duplicate() override;
    virtual void remove() override;

    // Called after files loaded to pickup any new files that might be present
    // in managed directories.
    virtual void scanFolders() override;

    virtual bool requestPlaybackFor(const FileHandle &file) override;

    /**
     * For view modes that have dynamic playlists, this freezes them from
     * removing playlists.
     */
    virtual void setDynamicListsFrozen(bool frozen) override;

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
    virtual void setupPlaylist(Playlist *playlist, const QString &iconName) override;
    virtual void removePlaylist(Playlist *playlist) override;
    virtual Qt::DropActions supportedDropActions() const override;
    virtual bool dropMimeData(QTreeWidgetItem *, int, const QMimeData *, Qt::DropAction) override;
    virtual QStringList mimeTypes() const override;

signals:
    void signalPlaylistDestroyed(Playlist *);
    void signalMoveFocusAway(); // Handles keyboard scrolling up out of playlist
    void startupComplete(); ///< Emitted after playlists are loaded.
    void signalPlayFile(const FileHandle &file);

private:
    void readConfig();
    void saveConfig();

    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    virtual void keyReleaseEvent(QKeyEvent *e) override;

    // selectedItems already used for something different

    ItemList selectedBoxItems();
    void setSingleItem(QTreeWidgetItem *item);

    void setupItem(Item *item);
    void setupUpcomingPlaylist();
    int viewModeIndex() const { return m_viewModeIndex; }
    ViewMode *viewMode() const { return m_viewModes[m_viewModeIndex]; }
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

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

    void slotUpdatePlayingPlaylist();
    void slotClearPlayingIndicators();

    void slotAddItem(const QString &tag, unsigned column);
    void slotRemoveItem(const QString &tag, unsigned column);

    // Used to load the playlists after GUI setup.
    void slotLoadCachedPlaylists();

private:
    QHash<Playlist *, Item *> m_playlistDict;
    QTimer *m_showTimer            = nullptr;
    QTimer *m_savePlaylistTimer    = nullptr;
    Item *m_dropItem               = nullptr;
    QMenu *m_contextMenu           = nullptr;
    QVector<ViewMode *> m_viewModes;
    int m_viewModeIndex            = 0;
    bool m_hasSelection            = false;
    bool m_doingMultiSelect        = false;
};

class PlaylistBox::Item final : public QObject, public QTreeWidgetItem
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
    virtual ~Item() = default;

protected:
    using QTreeWidgetItem::text;

    Item(PlaylistBox *listBox, const QString &icon, const QString &text, Playlist *l = nullptr);
    Item(Item *parent, const QString &icon, const QString &text, Playlist *l = nullptr);

    Playlist *playlist() const { return m_playlist; }
    PlaylistBox *listView() const { return static_cast<PlaylistBox *>(QTreeWidgetItem::treeWidget()); }
    QString iconName() const { return m_iconName; }
    QString text() const { return QTreeWidgetItem::text(0); }

    void setSortedFirst(bool first = true) { m_sortedFirst = first; }
    void setPlaying(bool isPlaying);

    virtual void setup();

    static Item *collectionItem() { return m_collectionItem; }

    // Used to post a timer in PlaylistBox to save playlists.
    void playlistItemDataChanged();


protected slots:
    void slotSetName(const QString &name);

private:
    // setup() was already taken.
    void init();
    QString sortTextFor(const QString &name) const;

    Playlist *m_playlist;
    QString m_iconName;
    bool m_sortedFirst;

    static Item *m_collectionItem;
};

#endif

// vim: set et sw=4 tw=0 sta:
