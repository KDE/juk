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

#include <kiconloader.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kdebug.h>

#include <qdrawutil.h>

#include "playlistbox.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(QWidget *parent, const char *name) : KListBox(parent, name)
{
    setAcceptDrops(true);
    connect(this, SIGNAL(currentChanged(QListBoxItem *)), this, SLOT(currentItemChanged(QListBoxItem *)));
}

PlaylistBox::~PlaylistBox()
{

}

QStringList PlaylistBox::names() const
{
    return(nameList);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox protected methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::resizeEvent(QResizeEvent *e)
{
    // hack-ish, but functional 

    for(int i = 0; i <= count(); i++)
	updateItem(i);

    KListBox::resizeEvent(e);
}

void PlaylistBox::dropEvent(QDropEvent *e)
{
    KURL::List urls;
    
    if(KURLDrag::decode(e, urls) && !urls.isEmpty()) {

	QStringList files;

	for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	    files.append((*it).path());

	PlaylistBoxItem *i = static_cast<PlaylistBoxItem *>(itemAt(e->pos()));

	if(i && i->playlist())
	    i->playlist()->append(files);
    }
}

void PlaylistBox::dragMoveEvent(QDragMoveEvent *e)
{
    // If we can decode the input source, there is a non-null item at the "move"
    // position, the playlist for that PlaylistBoxItem is non-null, is not the 
    // selected playlist and is not the CollectionList, then accept the event.
    //
    // Otherwise, do not accept the event.

    // At the moment this does not account for dragging from sources outside of
    // JuK onto the CollectionList or to the selected PlaylistBoxItem.  It makes
    // sense to not allow this from the top widget of the QWidgetStack of 
    // playlists, however, it should allow dragging from other applications, i.e.
    // Konq to these PlaylistBoxItems.  This just involves checking the source 
    // in the logic below.
    
    if(KURLDrag::canDecode(e) && itemAt(e->pos())) {
	PlaylistBoxItem *i = static_cast<PlaylistBoxItem *>(itemAt(e->pos()));
	if(i->playlist() && i->playlist() != CollectionList::instance())
	    if(selectedItem() && i != selectedItem())
		e->accept(true);
	    else
		e->accept(false);
	else
	    e->accept(false);
    }
    else
	e->accept(false);
}

void PlaylistBox::addName(const QString &name)
{
    nameList.append(name);
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

PlaylistBoxItem::PlaylistBoxItem(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l) : ListBoxPixmap(listbox, pix, text)
{
    list = l;
    setOrientation(Qt::Vertical);
    listbox->addName(text);
}

PlaylistBoxItem::PlaylistBoxItem(PlaylistBox *listbox, const QString &text, Playlist *l) : ListBoxPixmap(listbox, SmallIcon("midi", 32), text)
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
