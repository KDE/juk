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
#include <qinputdialog.h>

#include "playlistbox.h"
#include "collectionlist.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlaylistSplitter *parent, const char *name) : KListBox(parent, name)
{
    splitter = parent;

    collectionContextMenu = new KPopupMenu();
    collectionContextMenu->insertItem(SmallIcon("editcopy"), i18n("Duplicate..."), this, SLOT(contextDuplicateItem()));

    playlistContextMenu = new KPopupMenu();
    playlistContextMenu->insertItem(SmallIcon("filesave"), i18n("Save"), this, SLOT(contextSave()));
    playlistContextMenu->insertItem(SmallIcon("filesaveas"), i18n("Save As..."), this, SLOT(contextSaveAs()));
    playlistContextMenu->insertItem(i18n("Rename..."), this, SLOT(contextRename()));
    playlistContextMenu->insertItem(SmallIcon("editcopy"), i18n("Duplicate..."), this, SLOT(contextDuplicateItem()));
    playlistContextMenu->insertItem(SmallIcon("editdelete"), i18n("Delete"), this, SLOT(contextDeleteItem()));

    setAcceptDrops(true);

    connect(this, SIGNAL(currentChanged(QListBoxItem *)), 
	    this, SLOT(currentItemChanged(QListBoxItem *)));
}

PlaylistBox::~PlaylistBox()
{
    delete(playlistContextMenu);
}

QStringList PlaylistBox::names() const
{
    return(nameList);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private methods
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
	    i->playlist()->add(files);
    }
}

void PlaylistBox::dragMoveEvent(QDragMoveEvent *e)
{
    // If we can decode the input source, there is a non-null item at the "move"
    // position, the playlist for that PlaylistBoxItem is non-null, is not the 
    // selected playlist and is not the CollectionList, then accept the event.
    //
    // Otherwise, do not accept the event.
    
    if(KURLDrag::canDecode(e) && itemAt(e->pos())) {
	PlaylistBoxItem *i = static_cast<PlaylistBoxItem *>(itemAt(e->pos()));

	// This is a semi-dirty hack to check if the items are coming from within
	// JuK.  If they are not coming from a Playlist (or subclass) then the
	// dynamic_cast will fail and we can safely assume that the item is 
	// coming from outside of JuK.

	if(dynamic_cast<Playlist *>(e->source())) {
	    if(i->playlist() && i->playlist() != CollectionList::instance())
		if(selectedItem() && i != selectedItem())
		    e->accept(true);
		else
		    e->accept(false);
	    else
		e->accept(false);
	}
	else // the dropped items are coming from outside of JuK
	    e->accept(true);
    }
    else
	e->accept(false);

}

void PlaylistBox::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::RightButton) {
	QListBoxItem *i = itemAt(e->pos());
	if(i)
	    drawContextMenu(i, e->globalPos());
	e->accept();
    }
    else {
	e->ignore();
	QListBox::mousePressEvent(e);
    }
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

void PlaylistBox::drawContextMenu(QListBoxItem *item, const QPoint &point)
{
    PlaylistBoxItem *i = static_cast<PlaylistBoxItem *>(item);

    contextMenuOn = i;

    if(i)
	if(i->playlist() == CollectionList::instance())
	    collectionContextMenu->popup(point);
	else
	    playlistContextMenu->popup(point);
}

void PlaylistBox::contextSave()
{
    if(contextMenuOn)
	contextMenuOn->playlist()->save();
}

void PlaylistBox::contextSaveAs()
{
    if(contextMenuOn)
	contextMenuOn->playlist()->saveAs();
}

void PlaylistBox::contextRename()
{
    if(contextMenuOn) {
	bool ok;

	QString name = QInputDialog::getText(i18n("Rename..."), i18n("Please enter a name for this playlist:"),
					     QLineEdit::Normal, contextMenuOn->text(), &ok);
	if(ok) {
	    nameList.remove(contextMenuOn->text());
	    nameList.append(name);
	    contextMenuOn->setText(name);
	    updateItem(contextMenuOn);
	}
    }
}

void PlaylistBox::contextDuplicateItem()
{
    if(contextMenuOn) {
	bool ok;

	// If this text is changed, please also change it in PlaylistSplitter::createPlaylist().

	QString name = QInputDialog::getText(i18n("New Playlist..."), i18n("Please enter a name for the new playlist:"),
					     QLineEdit::Normal, splitter->uniquePlaylistName(contextMenuOn->text(), true), &ok);
	if(ok) {
	    Playlist *p = splitter->createPlaylist(name);
	    p->add(contextMenuOn->playlist()->files());
	}
    }
}

void PlaylistBox::contextDeleteItem()
{
    if(contextMenuOn) {
	nameList.remove(contextMenuOn->text());
	delete(contextMenuOn->playlist());
	delete(contextMenuOn);
	contextMenuOn = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBoxItem public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBoxItem::PlaylistBoxItem(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l) 
    : ListBoxPixmap(listbox, pix, text)
{
    list = l;
    setOrientation(Qt::Vertical);
    listbox->addName(text);
}

PlaylistBoxItem::PlaylistBoxItem(PlaylistBox *listbox, const QString &text, Playlist *l) 
    : ListBoxPixmap(listbox, SmallIcon("midi", 32), text)
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
