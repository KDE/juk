/***************************************************************************
                          playlistbox.cpp  -  description
                             -------------------
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002 by Scott Wheeler, 
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

#include <kiconloader.h>
#include <kurldrag.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#include <qfile.h>
#include <qdrawutil.h>
#include <qinputdialog.h>

#include "playlist.h"
#include "playlistbox.h"
#include "collectionlist.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlaylistSplitter *parent, const char *name) : KListBox(parent, name)
{
    splitter = parent;
    updatePlaylistStack = true;

    collectionContextMenu = new KPopupMenu();
    collectionContextMenu->insertItem(SmallIcon("editcopy"), i18n("Duplicate..."), this, SLOT(contextDuplicate()));

    playlistContextMenu = new KPopupMenu();
    playlistContextMenu->insertItem(SmallIcon("filesave"), i18n("Save"), this, SLOT(contextSave()));
    playlistContextMenu->insertItem(SmallIcon("filesaveas"), i18n("Save As..."), this, SLOT(contextSaveAs()));
    playlistContextMenu->insertItem(i18n("Rename..."), this, SLOT(contextRename()));
    playlistContextMenu->insertItem(SmallIcon("editcopy"), i18n("Duplicate..."), this, SLOT(contextDuplicate()));
    playlistContextMenu->insertItem(SmallIcon("editdelete"), i18n("Delete"), this, SLOT(contextDeleteItem()));

    setAcceptDrops(true);

    connect(this, SIGNAL(currentChanged(QListBoxItem *)), 
	    this, SLOT(playlistChanged(QListBoxItem *)));

    connect(this, SIGNAL(doubleClicked(QListBoxItem *)), 
	    this, SLOT(playlistDoubleClicked(QListBoxItem *)));
}

PlaylistBox::~PlaylistBox()
{
    delete(playlistContextMenu);
}

void PlaylistBox::sort()
{
    QListBoxItem *collectionItem = firstItem();

    bool collectionSelected = isSelected(collectionItem);

    updatePlaylistStack = false;

    if(collectionSelected)
	setSelected(collectionItem, false);

    takeItem(collectionItem);
    KListBox::sort();
    insertItem(collectionItem, 0);

    if(collectionSelected)
	setSelected(collectionItem, true);

    updatePlaylistStack = true;
}

QStringList PlaylistBox::names() const
{
    return(nameList);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::save()
{
    save(static_cast<PlaylistBoxItem *>(selectedItem()));
}

void PlaylistBox::save(PlaylistBoxItem *item)
{
    if(item)
	item->playlist()->save();    
}

void PlaylistBox::saveAs()
{
    saveAs(static_cast<PlaylistBoxItem *>(selectedItem()));
}

void PlaylistBox::saveAs(PlaylistBoxItem *item)
{
    if(item)
        item->playlist()->saveAs();
}

void PlaylistBox::rename()
{
    rename(static_cast<PlaylistBoxItem *>(selectedItem()));
}

void PlaylistBox::rename(PlaylistBoxItem *item)
{
    if(item) {
	bool ok;

	QString name = QInputDialog::getText(i18n("Rename..."), i18n("Please enter a name for this playlist:"),
					     QLineEdit::Normal, item->text(), &ok);
	if(ok) {
	    item->setText(name);
	   
	    // Telling the playlist to change it's name will emit a signal that
	    // is connected to PlaylistItem::setName().

	    if(item->playlist())
		item->playlist()->setName(name);
	}
    }
}

void PlaylistBox::duplicate()
{
    duplicate(static_cast<PlaylistBoxItem *>(selectedItem()));
}

void PlaylistBox::duplicate(PlaylistBoxItem *item)
{
    if(item) {
	bool ok;

	// If this text is changed, please also change it in PlaylistSplitter::createPlaylist().

	QString name = QInputDialog::getText(i18n("New Playlist..."), i18n("Please enter a name for the new playlist:"),
					     QLineEdit::Normal, splitter->uniquePlaylistName(item->text(), true), &ok);
	if(ok) {
	    Playlist *p = splitter->createPlaylist(name);
	    splitter->add(item->playlist()->files(), p);
	}
    }
}

void PlaylistBox::deleteItem()
{
    deleteItem(static_cast<PlaylistBoxItem *>(selectedItem()));
}

void PlaylistBox::deleteItem(PlaylistBoxItem *item)
{
    if(item && item->playlist()) {

	// If the file is "internal" (not loaded from a file and not yet saved),
	// or the file name is null, or the user specifically chooses to delete 
	// the file then delete it.  Otherwise, just remove the file from the
	// PlaylistBox.

	if(item->playlist()->fileName() != QString::null) {
	    if(item->playlist()->isInternalFile())
		QFile::remove(item->playlist()->fileName());
	    else {
		int remove = KMessageBox::warningYesNoCancel(this, i18n("Do you want to delete this file from the disk as well?"));
		
		if(remove == KMessageBox::Yes) {
		    if(!QFile::remove(item->playlist()->fileName()))
			KMessageBox::sorry(this, i18n("Could not delete the specified file."));
		}
		else if(remove == KMessageBox::Cancel)
		    return;
	    }
	}

	nameList.remove(item->text());
	delete(item->playlist());
	delete(item);
    }
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::resizeEvent(QResizeEvent *e)
{
    // hack-ish, but functional 

//    for(int i = 0; i <= count(); i++)
//	updateItem(i);
    triggerUpdate(true);

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
	    splitter->add(files, i->playlist());
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

void PlaylistBox::playlistChanged(QListBoxItem *item)
{
    PlaylistBoxItem *i = dynamic_cast<PlaylistBoxItem *>(item);
    if(updatePlaylistStack && i)
	emit(currentChanged(i));
}

void PlaylistBox::playlistDoubleClicked(QListBoxItem *)
{
/*
    PlaylistBoxItem *i = dynamic_cast<PlaylistBoxItem *>(item);
    if(i)
	emit(doubleClicked(i));
*/
    emit(doubleClicked());
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
    save(contextMenuOn);
}

void PlaylistBox::contextSaveAs()
{
    saveAs(contextMenuOn);
}

void PlaylistBox::contextRename()
{
    rename(contextMenuOn);
}

void PlaylistBox::contextDuplicate()
{
    duplicate(contextMenuOn);
}

void PlaylistBox::contextDeleteItem()
{
    deleteItem(contextMenuOn);
    contextMenuOn = 0;
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBoxItem public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBoxItem::PlaylistBoxItem(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l) 
    : QObject(listbox), ListBoxPixmap(listbox, pix, text)
{
    list = l;
    setOrientation(Qt::Vertical);
    listbox->addName(text);

    connect(l, SIGNAL(nameChanged(const QString &)), this, SLOT(setName(const QString &)));
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

PlaylistBox *PlaylistBoxItem::listBox() const
{
    return(dynamic_cast<PlaylistBox *>(ListBoxPixmap::listBox()));
}

void PlaylistBoxItem::setName(const QString &name)
{
    if(listBox()) {
	listBox()->nameList.remove(text());
	listBox()->nameList.append(name);

	setText(name);
	listBox()->updateItem(this);
    }
}

#include "playlistbox.moc"
