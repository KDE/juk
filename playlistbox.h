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

#include <qwidgetstack.h>

#include "listboxpixmap.h"
#include "playlist.h"

class PlaylistBoxItem;

/** This is the play list selection box that is by default on the right side of
    JuK's main widget (PlaylistSplitter). */

class PlaylistBox : public KListBox
{
    friend class PlaylistBoxItem;

    Q_OBJECT
public: 
    PlaylistBox(QWidget *parent = 0, const char *name = 0);
    virtual ~PlaylistBox();

    QStringList names() const;

protected:
    virtual void resizeEvent(QResizeEvent *e);
    virtual void dropEvent(QDropEvent *e);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    /** This is used by PlaylistItemBox (a friend class) to add names to the name
	list returned by names(). */
    void addName(const QString &name);

private:
    QWidgetStack *stack;
    QStringList nameList;

private slots:
    /** Catches QListBox::clicked(QListBoxItem *), does a cast and then re-emits
        the signal as  clicked(PlaylistBoxItem *). */
    void currentItemChanged(QListBoxItem *item);

signals:
    void currentChanged(PlaylistBoxItem *);
};

class PlaylistBoxItem : public ListBoxPixmap
{
public:
    PlaylistBoxItem(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l = 0);
    PlaylistBoxItem(PlaylistBox *listbox, const QString &text, Playlist *l = 0);
    virtual ~PlaylistBoxItem();

    Playlist *playlist() const;
    
private:
    Playlist *list;
};

#endif
