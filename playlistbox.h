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

#include <klistbox.h>
#include <kpopupmenu.h>

#include <qwidgetstack.h>
#include <qptrlist.h>
#include <qptrdict.h>

#include "listboxpixmap.h"

class Playlist;
class PlaylistItem;

class PlaylistSplitter;

/** 
 * This is the play list selection box that is by default on the right side of
 * JuK's main widget (PlaylistSplitter). 
 */

class PlaylistBox : public KListBox
{
    Q_OBJECT

public: 
    enum ItemType { Collection, Plain };

    PlaylistBox(PlaylistSplitter *parent = 0, const char *name = 0);
    virtual ~PlaylistBox();

    void createItem(Playlist *playlist, const char *icon = 0, bool raise = false);

    void sort();
    void raise(Playlist *playlist);
    QStringList names() const;
    QPtrList<Playlist> playlists() const;

public slots:
    // All of the slots without parameters default to the selected item.
    void save();
    void saveAs();
    void rename();
    void duplicate();
    void deleteItem();

    void paste();
    /**
     * Override the default behavior of clear so that the clipboard code doesn't
     * do bad things.
     */
    void clear() {}

signals:
    void currentChanged(Playlist *);
    void doubleClicked();

private:
    class Item;
    friend class Item;

    void save(Item *item);
    void saveAs(Item *item);
    void rename(Item *item);
    void duplicate(Item *item);
    void deleteItem(Item *item);

    virtual void resizeEvent(QResizeEvent *e);
    virtual void decode(QMimeSource *s, Item *item);
    virtual void dropEvent(QDropEvent *e);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    /** 
     * This is used by PlaylistItemBox (a friend class) to add names to the name
     * list returned by names(). 
     */
    void addName(const QString &name);

private slots:
    /** 
     * Catches QListBox::currentChanged(QListBoxItem *), does a cast and then re-emits
     * the signal as  currentChanged(Item *). 
     */
    void playlistChanged(QListBoxItem *item);
    void playlistDoubleClicked(QListBoxItem *);
    void drawContextMenu(QListBoxItem *item, const QPoint &point);

    // context menu entries
    void contextSave();
    void contextSaveAs();
    void contextRename();
    void contextDuplicate();
    void contextDeleteItem();

private:
    PlaylistSplitter *splitter;
    QStringList nameList;
    KPopupMenu *collectionContextMenu;
    KPopupMenu *playlistContextMenu;
    Item *contextMenuOn;
    bool updatePlaylistStack;
    QPtrDict<Item> _playlistDict;
};



class PlaylistBox::Item : public QObject, public ListBoxPixmap
{
    friend class PlaylistBox;

    Q_OBJECT

public:
    Item(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l = 0);
    Item(PlaylistBox *listbox, const QString &text, Playlist *l = 0);
    virtual ~Item();

    Playlist *playlist() const;
    PlaylistBox *listBox() const;

public slots:
    void setName(const QString &name);
    
private:
    Playlist *list;
};

#endif
