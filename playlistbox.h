/***************************************************************************
                          playlistbox.h  -  description
                             -------------------
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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

#include <qptrdict.h>

class Playlist;
class PlaylistItem;
class DynamicPlaylist;
class ViewMode;
class PlaylistSearch;
class SearchPlaylist;

class KPopupMenu;
class KSelectAction;

typedef QValueList<Playlist *> PlaylistList;

/** 
 * This is the play list selection box that is by default on the right side of
 * JuK's main widget (PlaylistSplitter). 
 */

class PlaylistBox : public KListView, public PlaylistCollection
{
    Q_OBJECT

public: 
    PlaylistBox(QWidget *parent, QWidgetStack *playlistStack,
		const QString &name = QString::null);

    virtual ~PlaylistBox();

    void createSearchItem(SearchPlaylist *playlist, const QString &searchCategory);

    void raise(Playlist *playlist);
    QStringList names() const { return m_names; }

    /**
     * A list of all of the playlists in the PlaylistBox, not counting dynamic
     * playlists.
     */
    PlaylistList playlists();

    void duplicate();

    /**
     * Delete the item associated with \a playlist.
     */
    void deleteItem(Playlist *playlist);
    void deleteItems() { deleteItems(selectedItems()); }

    bool hasSelection() const  { return m_hasSelection; }

    ViewMode *viewMode()       { return m_viewModes[m_viewModeIndex]; }
    int viewModeIndex() const  { return m_viewModeIndex; }
    void ensureCurrentVisible() { ensureItemVisible(currentItem()); }

    class Item;
    friend class Item;
    typedef QValueList<Item *> ItemList;

    Item *dropItem() const { return m_dropItem; }

public slots:
    void paste();
    void clear() {} // override the (destructive) default

    virtual Playlist *currentPlaylist() const;
protected:
    // virtual Playlist *currentPlaylist() const;
    virtual void setupPlaylist(Playlist *playlist, const QString &iconName);

signals:
    void signalDoubleClicked();
    void signalCreatePlaylist(const QStringList &files);
    void signalCreateSearchList(const PlaylistSearch &search,
				const QString &searchCategory,
				const QString &name);
    void signalCollectionInitialized();

private:
    void readConfig();
    void saveConfig();

    void duplicate(Item *item);
    void deleteItems(const QValueList<Item *> &items, bool confirm = true);

    virtual void decode(QMimeSource *s, Item *item);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    virtual void contentsDragLeaveEvent(QDragLeaveEvent *e);
    virtual void contentsMousePressEvent(QMouseEvent *e);
    virtual void contentsMouseReleaseEvent(QMouseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void keyReleaseEvent(QKeyEvent *e);

    /** 
     * This is used by PlaylistItemBox (a friend class) to add names to the name
     * list returned by names(). 
     */
    void addName(const QString &name) { m_names.append(name); }

    QValueList<Item *> selectedItems();

    void setSingleItem(QListViewItem *item);
    void setupItem(Item *item);

private slots:
    /** 
     * Catches QListBox::currentChanged(QListBoxItem *), does a cast and then re-emits
     * the signal as currentChanged(Item *). 
     */
    void slotPlaylistChanged();
    void slotDoubleClicked(QListViewItem *);
    void slotShowContextMenu(QListViewItem *, const QPoint &point, int);
    void slotSetViewMode(int index);

private:
    QStringList m_names;
    KPopupMenu *m_contextMenu;
    bool m_updatePlaylistStack;
    QPtrDict<Item> m_playlistDict;
    int m_viewModeIndex;
    QValueList<ViewMode *> m_viewModes;
    KSelectAction *m_viewModeAction;
    bool m_hasSelection;
    bool m_doingMultiSelect;
    Item *m_dropItem;
    DynamicPlaylist *m_dynamicPlaylist;
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

    virtual int compare(QListViewItem *i, int col, bool) const;
    virtual void paintCell(QPainter *p, const QColorGroup &colorGroup, int column, int width, int align);
    virtual void paintFocus(QPainter *, const QColorGroup &, const QRect &) {}
    virtual void setText(int column, const QString &text);

    virtual QString text(int column) const { return KListViewItem::text(column); }

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
