/***************************************************************************
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYLISTBOX_H
#define PLAYLISTBOX_H

#include "playlistcollection.h"

#include <klistview.h>

#include <q3ptrdict.h>

#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QDropEvent>
#include <Q3ValueList>
#include <QMouseEvent>

class Playlist;
class PlaylistItem;
class ViewMode;
class PlaylistSearch;
class SearchPlaylist;

class KMenu;
class KSelectAction;

typedef Q3ValueList<Playlist *> PlaylistList;

/**
 * This is the play list selection box that is by default on the right side of
 * JuK's main widget (PlaylistSplitter).
 */

class PlaylistBox : public KListView, public PlaylistCollection
{
    Q_OBJECT

public:
    class Item;
    typedef Q3ValueList<Item *> ItemList;

    friend class Item;

    PlaylistBox(QWidget *parent, Q3WidgetStack *playlistStack);
    virtual ~PlaylistBox();

    virtual void raise(Playlist *playlist);
    virtual void duplicate();
    virtual void remove();

    /**
     * For view modes that have dynamic playlists, this freezes them from
     * removing playlists.
     */
    virtual void setDynamicListsFrozen(bool frozen);

    Item *dropItem() const { return m_dropItem; }

    void setupPlaylist(Playlist *playlist, const QString &iconName, Item *parentItem = 0);

public slots:
    void paste();
    void clear() {}

    void slotFreezePlaylists();
    void slotUnfreezePlaylists();

protected:
    virtual void setupPlaylist(Playlist *playlist, const QString &iconName);
    virtual void removePlaylist(Playlist *playlist);

signals:
    void signalPlaylistDestroyed(Playlist *);

private:
    void readConfig();
    void saveConfig();

    virtual void decode(QMimeSource *s, Item *item);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    virtual void contentsDragLeaveEvent(QDragLeaveEvent *e);
    virtual void contentsMousePressEvent(QMouseEvent *e);
    virtual void contentsMouseReleaseEvent(QMouseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);

    Q3ValueList<Item *> selectedItems() const;
    void setSingleItem(Q3ListViewItem *item);

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
    void slotDoubleClicked();
    void slotShowContextMenu(Q3ListViewItem *, const QPoint &point, int);
    void slotSetViewMode(int index);
    void slotSavePlaylists();
    void slotShowDropTarget();

    void slotPlaylistItemsDropped(Playlist *p);

    void slotAddItem(const QString &tag, unsigned column);
    void slotRemoveItem(const QString &tag, unsigned column);

private:
    KMenu *m_contextMenu;
    Q3PtrDict<Item> m_playlistDict;
    int m_viewModeIndex;
    QList<ViewMode *> m_viewModes;
    KAction *m_k3bAction;
    bool m_hasSelection;
    bool m_doingMultiSelect;
    Item *m_dropItem;
    QTimer *m_showTimer;
};



class PlaylistBox::Item : public QObject, public KListViewItem
{
    friend class PlaylistBox;
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
    PlaylistBox *listView() const { return static_cast<PlaylistBox *>(KListViewItem::listView()); }
    QString iconName() const { return m_iconName; }
    QString text() const { return m_text; }
    void setSortedFirst(bool first = true) { m_sortedFirst = first; }

    virtual int compare(Q3ListViewItem *i, int col, bool) const;
    virtual void paintCell(QPainter *p, const QColorGroup &colorGroup, int column, int width, int align);
    virtual void paintFocus(QPainter *, const QColorGroup &, const QRect &) {}
    virtual void setText(int column, const QString &text);

    virtual QString text(int column) const { return KListViewItem::text(column); }

    virtual void setup();

    static Item *collectionItem() { return m_collectionItem; }
    static void setCollectionItem(Item *item) { m_collectionItem = item; }


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
