/***************************************************************************
                          playlistbox.h  -  description
                             -------------------
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#include <klistview.h>

#include <qwidgetstack.h>
#include <qptrdict.h>
#include <qmap.h>

#include "playlist.h"

class PlaylistItem;
class PlaylistSplitter;
class ViewMode;
class PlaylistSearch;
class SearchPlaylist;

class KPopupMenu;
class KSelectAction;

/** 
 * This is the play list selection box that is by default on the right side of
 * JuK's main widget (PlaylistSplitter). 
 */

class PlaylistBox : public KListView
{
    Q_OBJECT

public: 
    PlaylistBox(PlaylistSplitter *parent = 0, const QString &name = QString::null);
    virtual ~PlaylistBox();

    void createItem(Playlist *playlist, const char *icon = 0,
		    bool raise = false, bool sortedFirst = false);

    void createSearchItem(SearchPlaylist *playlist, const QString &searchCategory);

    void raise(Playlist *playlist);
    QStringList names() const { return m_names; }

    /**
     * A list of all of the playlists in the PlaylistBox, not counting dynamic
     * playlists.
     */
    PlaylistList playlists();

    // All of the methods use the selected item.
    void save();
    void saveAs();
    void rename();
    void duplicate();

    /**
     * Delete the item associated with \a playlist.
     */
    void deleteItem(Playlist *playlist);
    void deleteItems() { deleteItems(selectedItems()); }

    bool hasSelection() const  { return m_hasSelection; }

    ViewMode *viewMode()       { return m_viewModes[m_viewModeIndex]; }
    int viewModeIndex() const  { return m_viewModeIndex; }

    class Item;
    friend class Item;
    typedef QValueList<Item *> ItemList;

public slots:
    void paste();
    void clear() {} // override the (destructive) default

signals:
    void signalCurrentChanged(const PlaylistList &);
    void signalDoubleClicked();
    void signalCreatePlaylist(const QStringList &files);
    void signalCreateSearchList(const PlaylistSearch &search,
				const QString &searchCategory,
				const QString &name);
    void signalCollectionInitialized();

private:
    void readConfig();
    void saveConfig();

    void save(Item *item);
    void saveAs(Item *item);
    void rename(Item *item);
    void duplicate(Item *item);
    void deleteItems(const QValueList<Item *> &items, bool confirm = true);

    virtual void decode(QMimeSource *s, Item *item);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    /** 
     * This is used by PlaylistItemBox (a friend class) to add names to the name
     * list returned by names(). 
     */
    void addName(const QString &name) { m_names.append(name); }

    QValueList<Item *> selectedItems();

    void setSingleItem(QListViewItem *item);
    void ensureCurrentVisible() { ensureItemVisible(currentItem()); }

    void setupItem(Item *item, Playlist *playlist);

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
    PlaylistSplitter *m_splitter;
    QStringList m_names;
    KPopupMenu *m_contextMenu;
    bool m_updatePlaylistStack;
    QPtrDict<Item> m_playlistDict;
    int m_viewModeIndex;
    QValueList<ViewMode *> m_viewModes;
    KSelectAction *m_viewModeAction;
    bool m_hasSelection;
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
    Item(PlaylistBox *listBox, const char *icon, const QString &text, Playlist *l = 0);
    Item(Item *parent, const char *icon, const QString &text, Playlist *l = 0);

    Playlist *playlist() const { return m_list; }
    PlaylistBox *listView() const { return static_cast<PlaylistBox *>(KListViewItem::listView()); }
    const char *iconName() const { return m_iconName; }
    QString text() const { return m_text; }
    void setSortedFirst(bool first) { m_sortedFirst = first; }

    virtual int compare(QListViewItem *i, int col, bool) const;
    virtual void paintCell(QPainter *p, const QColorGroup &colorGroup, int column, int width, int align);
    virtual void setText(int column, const QString &text);

    virtual QString text(int column) const { return KListViewItem::text(column); }

    static Item *collectionItem() { return m_collectionItem; }

protected slots:
    void slotSetName(const QString &name);

private:
    // setup() was already taken.
    void init();

    Playlist *m_list;
    QString m_text;
    const char *m_iconName;
    bool m_sortedFirst;
    static Item *m_collectionItem;
};

#endif
