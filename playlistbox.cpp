/***************************************************************************
                          playlistbox.cpp  -  description
                             -------------------
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002 by Scott Wheeler, 
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

#include <klocale.h>
#include <kiconloader.h>

#include <qdrawutil.h>

#include "playlistbox.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(QWidget *parent, const char *name) : KListBox(parent, name)
{
    connect(this, SIGNAL(currentChanged(QListBoxItem *)), this, SLOT(currentItemChanged(QListBoxItem *)));
}

PlaylistBox::~PlaylistBox()
{

}

void PlaylistBox::resizeEvent(QResizeEvent *e)
{
    // hack-ish, but functional 

    for(int i = 0; i <= count(); i++)
	updateItem(i);

    KListBox::resizeEvent(e);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::currentItemChanged(QListBoxItem *item)
{
    PlaylistBoxItem *i = dynamic_cast<PlaylistBoxItem *>(item);
    if(i)
	emit(currentChanged(i));
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBoxItem public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBoxItem::PlaylistBoxItem(QListBox *listbox, const QPixmap &pix, const QString &text, Playlist *l) : ListBoxPixmap(listbox, pix, text)
{
    list = l;
    setOrientation(Qt::Vertical);
}

PlaylistBoxItem::PlaylistBoxItem(QListBox *listbox, const QString &text, Playlist *l) : ListBoxPixmap(listbox, SmallIcon("midi", 32), text)
{
    list = l;
    setOrientation(Qt::Vertical);
}

PlaylistBoxItem::~PlaylistBoxItem()
{

}

Playlist *PlaylistBoxItem::playlist() const
{
    return(list);
}

#include "playlistbox.moc"
