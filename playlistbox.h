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

#include "listboxpixmap.h"

class Playlist;
class PlaylistItem;

class PlaylistBoxItem;
class PlaylistSplitter;

/** 
 * This is the play list selection box that is by default on the right side of
 * JuK's main widget (PlaylistSplitter). 
 */

class PlaylistBox : public KListBox
{
    friend class PlaylistBoxItem;

    Q_OBJECT

public: 
    PlaylistBox(PlaylistSplitter *parent = 0, const QString &name = QString::null);
    virtual ~PlaylistBox();

    void sort();
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

    void paste();
    void clear() {}

signals:
    void currentChanged(PlaylistBoxItem *);
    void doubleClicked();

private:
    virtual void resizeEvent(QResizeEvent *e);
    virtual void decode(QMimeSource *s, PlaylistBoxItem *item);
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
     * the signal as  currentChanged(PlaylistBoxItem *). 
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
    PlaylistBoxItem *contextMenuOn;
    bool updatePlaylistStack;
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
    PlaylistBox *listBox() const;

public slots:
    void setName(const QString &name);
    
private:
    Playlist *list;
};

#endif
