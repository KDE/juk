/***************************************************************************
                          playlistbox.h  -  description
                             -------------------
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
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

#include "listboxpixmap.h"

class Playlist;
class PlaylistItem;

class PlaylistBoxItem;
class PlaylistSplitter;

/** This is the play list selection box that is by default on the right side of
    JuK's main widget (PlaylistSplitter). */

class PlaylistBox : public KListBox
{
    friend class PlaylistBoxItem;

    Q_OBJECT

public: 
    PlaylistBox(PlaylistSplitter *parent = 0, const char *name = 0);
    virtual ~PlaylistBox();

    QStringList names() const;

public slots:
    // All of the slots without parameters default to the selected item.
    void save();
    void save(PlaylistBoxItem *item);
    void saveAs();
    void saveAs(PlaylistBoxItem *item);
    void rename();
    void rename(PlaylistBoxItem *item);
    void duplicate();
    void duplicate(PlaylistBoxItem *item);
    void deleteItem();
    void deleteItem(PlaylistBoxItem *item);

private:
    virtual void resizeEvent(QResizeEvent *e);
    virtual void dropEvent(QDropEvent *e);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    /** 
     * This is used by PlaylistItemBox (a friend class) to add names to the name
     * list returned by names(). 
     */
    void addName(const QString &name);

    PlaylistSplitter *splitter;
    QStringList nameList;
    KPopupMenu *collectionContextMenu;
    KPopupMenu *playlistContextMenu;
    PlaylistBoxItem *contextMenuOn;

private slots:
    /** 
     * Catches QListBox::currentChanged(QListBoxItem *), does a cast and then re-emits
     * the signal as  currentChanged(PlaylistBoxItem *). 
     */
    void playlistChanged(QListBoxItem *item);
    void playlistDoubleClicked(QListBoxItem *item);
    void drawContextMenu(QListBoxItem *item, const QPoint &point);

    // context menu entries
    void contextSave();
    void contextSaveAs();
    void contextRename();
    void contextDuplicate();
    void contextDeleteItem();

signals:
    void currentChanged(PlaylistBoxItem *);
    void doubleClicked(PlaylistBoxItem *);


#if QT_VERSION < 0x031000
public:
    // This method is defined in Qt 3.1 and later.
    QListBoxItem *selectedItem() { return(item(currentItem())); }
#endif
};



class PlaylistBoxItem : public QObject, public ListBoxPixmap
{
    friend class PlaylistBox;

    Q_OBJECT

public:
    PlaylistBoxItem(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l = 0);
    PlaylistBoxItem(PlaylistBox *listbox, const QString &text, Playlist *l = 0);
    virtual ~PlaylistBoxItem();

    Playlist *playlist() const;

public slots:
    void changeFile(const QString &file);
    
private:
    Playlist *list;
};

#endif
