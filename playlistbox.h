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
    Q_OBJECT
public: 
    PlaylistBox(QWidget *parent = 0, const char *name = 0);
    virtual ~PlaylistBox();

protected:
    virtual void resizeEvent(QResizeEvent *e);

private:
    QWidgetStack *stack;

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
    PlaylistBoxItem(QListBox *listbox, const QPixmap &pix, const QString &text, Playlist *l = 0);
    PlaylistBoxItem(QListBox *listbox, const QString &text, Playlist *l = 0);
    virtual ~PlaylistBoxItem();

    // This (and the playlist member variable) should be switched to the Playlist class once
    // the design is ready for that.

    Playlist *playlist() const;
    
private:
    Playlist *list;
};

#endif
