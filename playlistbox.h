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

class KPopupMenu;

/** 
 * This is the play list selection box that is by default on the right side of
 * JuK's main widget (PlaylistSplitter). 
 */

class PlaylistBox : public KListView
{
    Q_OBJECT

public: 
    PlaylistBox(PlaylistSplitter *parent = 0, const char *name = 0);
    virtual ~PlaylistBox();

    void createItem(Playlist *playlist, const char *icon = 0, bool raise = false);

    void raise(Playlist *playlist);
    QStringList names() const { return m_names; }
    PlaylistList playlists();

    // All of the methods use the selected item.
    void save();
    void saveAs();
    void rename();
    void duplicate();
    void deleteItem();

    class Item;
    friend class Item;

public slots:
    void paste();
    void clear() {} // override the (destructive) default

signals:
    void signalCurrentChanged(const PlaylistList &);
    void signalDoubleClicked();

private:
    void save(Item *item);
    void saveAs(Item *item);
    void rename(Item *item);
    void duplicate(Item *item);
    void deleteItem(Item *item);
    void reload(Item *item);

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

private slots:
    /** 
     * Catches QListBox::currentChanged(QListBoxItem *), does a cast and then re-emits
     * the signal as currentChanged(Item *). 
     */
    void slotPlaylistChanged();
    void slotDoubleClicked(QListViewItem *);
    void slotShowContextMenu(QListViewItem *item, const QPoint &point, int);

    // context menu entries
    void slotContextSave();
    void slotContextSaveAs();
    void slotContextRename();
    void slotContextDuplicate();
    void slotContextDeleteItem();
    void slotContextReload();

private:
    PlaylistSplitter *m_splitter;
    QStringList m_names;
    KPopupMenu *m_playlistContextMenu;
    Item *m_contextMenuOn;
    bool m_updatePlaylistStack;
    QPtrDict<Item> m_playlistDict;
    QMap<QString, int> m_popupIndex;
};



class PlaylistBox::Item : public QObject, public KListViewItem
{
    friend class PlaylistBox;

    Q_OBJECT

    // moc won't let me create private QObject subclasses and Qt won't let me
    // make the destructor protected, so here's the closest hack that will
    // compile.

public:
    virtual ~Item();

public slots:
    void slotSetName(const QString &name);
    
protected:
    Item(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l = 0);

    Playlist *playlist() const { return m_list; }
    PlaylistBox *listView() const { return static_cast<PlaylistBox *>(KListViewItem::listView()); }

    virtual int compare(QListViewItem *i, int col, bool) const;
    virtual void paintCell(QPainter *p, const QColorGroup &colorGroup, int column, int width, int align);
    virtual void setText(int column, const QString &text);

private:
    Playlist *m_list;
    QString m_text;
};

#endif
