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
#include <kapplication.h>
#include <kdebug.h>

#include <qfile.h>
#include <qdrawutil.h>
#include <qinputdialog.h>
#include <qclipboard.h>

#include "playlist.h"
#include "playlistbox.h"
#include "collectionlist.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlaylistSplitter *parent, const char *name) : KListBox(parent, name),
								       m_splitter(parent),
								       m_updatePlaylistStack(true)
{
    m_collectionContextMenu = new KPopupMenu();
    m_collectionContextMenu->insertItem(SmallIcon("editcopy"), i18n("Duplicate..."), this, SLOT(contextDuplicate()));

    m_playlistContextMenu = new KPopupMenu();
    m_playlistContextMenu->insertItem(SmallIcon("filesave"), i18n("Save"), this, SLOT(contextSave()));
    m_playlistContextMenu->insertItem(SmallIcon("filesaveas"), i18n("Save As..."), this, SLOT(contextSaveAs()));
    m_playlistContextMenu->insertItem(i18n("Rename..."), this, SLOT(contextRename()));
    m_playlistContextMenu->insertItem(SmallIcon("editcopy"), i18n("Duplicate..."), this, SLOT(contextDuplicate()));
    m_playlistContextMenu->insertItem(SmallIcon("edittrash"), i18n("Remove"), this, SLOT(contextDeleteItem()));

    setAcceptDrops(true);

    connect(this, SIGNAL(currentChanged(QListBoxItem *)), 
	    this, SLOT(slotPlaylistChanged(QListBoxItem *)));

    connect(this, SIGNAL(doubleClicked(QListBoxItem *)), 
	    this, SLOT(slotPlaylistDoubleClicked(QListBoxItem *)));
}

PlaylistBox::~PlaylistBox()
{
    delete m_collectionContextMenu;
    delete m_playlistContextMenu;
}

void PlaylistBox::createItem(Playlist *playlist, const char *icon, bool raise)
{
    if(!playlist)
	return;

    Item *i = new Item(this, SmallIcon(icon, 32), playlist->name(), playlist);
    m_playlistDict.insert(playlist, i);
    
    if(raise) {
	setCurrentItem(i);
	ensureCurrentVisible();	
    }

    sort();
}

void PlaylistBox::sort()
{
    QListBoxItem *collectionItem = firstItem();

    bool collectionSelected = isSelected(collectionItem);

    m_updatePlaylistStack = false;

    if(collectionSelected)
	setSelected(collectionItem, false);

    takeItem(collectionItem);
    KListBox::sort();
    insertItem(collectionItem, 0);

    if(collectionSelected)
	setSelected(collectionItem, true);

    m_updatePlaylistStack = true;
}

void PlaylistBox::raise(Playlist *playlist)
{
    if(!playlist)
	return;

    Item *i = m_playlistDict.find(playlist);

    clearSelection();
    setSelected(i, true);
    
    setCurrentItem(i);
    ensureCurrentVisible();
}

QPtrList<Playlist> PlaylistBox::playlists() const
{
    QPtrList<Playlist> l;

    // skip the collection list

    for(uint j = 1; j < count(); j++) {
	Item *i = static_cast<Item *>(item(j));
	l.append(i->playlist());
    }

    return l;
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::save()
{
    save(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::saveAs()
{
    saveAs(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::rename()
{
    rename(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::duplicate()
{
    duplicate(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::deleteItem()
{
    deleteItem(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::paste()
{
    Item *i = static_cast<Item *>(selectedItem());
    decode(kapp->clipboard()->data(), i);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::save(Item *item)
{
    if(item)
	item->playlist()->save();
}

void PlaylistBox::saveAs(Item *item)
{
    if(item)
        item->playlist()->saveAs();
}

void PlaylistBox::rename(Item *item)
{
    if(!item)
	return;

    bool ok;

    QString name = QInputDialog::getText(i18n("Rename"), i18n("Please enter a name for this playlist:"),
					 QLineEdit::Normal, item->text(), &ok);
    if(ok) {
	item->setText(name);
	
	// Telling the playlist to change it's name will emit a signal that
	// is connected to PlaylistItem::setName().
	
	if(item->playlist())
	    item->playlist()->setName(name);
	
	sort();
	setSelected(item, true);
	ensureCurrentVisible();
    }
}

void PlaylistBox::duplicate(Item *item)
{
    if(item) {
	bool ok;

	// If this text is changed, please also change it in PlaylistSplitter::createPlaylist().

	QString name = QInputDialog::getText(i18n("New Playlist"), i18n("Please enter a name for the new playlist:"),
					     QLineEdit::Normal, m_splitter->uniquePlaylistName(item->text(), true), &ok);
	if(ok) {
	    Playlist *p = m_splitter->createPlaylist(name);
	    m_splitter->add(item->playlist()->files(), p);
	}
    }
}

void PlaylistBox::deleteItem(Item *item)
{
    if(!item || !item->playlist())
	return;

    if(!item->playlist()->fileName().isEmpty()) {
	int remove = KMessageBox::warningYesNoCancel(this, i18n("Do you want to delete this file from the disk as well?"));
	
	if(remove == KMessageBox::Yes) {
	    if(!QFile::remove(item->playlist()->fileName()))
		KMessageBox::sorry(this, i18n("Could not delete the specified file."));
	}
	else if(remove == KMessageBox::Cancel)
	    return;
    }
    else {
	if(KMessageBox::warningYesNo(this, i18n("Are you sure you want to remove this item?")) == KMessageBox::No)
	    return;
    }
    
    m_names.remove(item->text());
    m_playlistDict.remove(item->playlist());
    delete item->playlist();
    delete item;
}

void PlaylistBox::resizeEvent(QResizeEvent *e)
{
    triggerUpdate(true);

    KListBox::resizeEvent(e);
}

void PlaylistBox::decode(QMimeSource *s, Item *item)
{
    if(!s || !item || !item->playlist())
	return;

    KURL::List urls;
    
    if(KURLDrag::decode(s, urls) && !urls.isEmpty()) {
	
	QStringList files;
	
	for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	    files.append((*it).path());
	
	m_splitter->add(files, item->playlist());
    }
}

void PlaylistBox::dropEvent(QDropEvent *e)
{
    Item *i = static_cast<Item *>(itemAt(e->pos()));
    decode(e, i);
}

void PlaylistBox::dragMoveEvent(QDragMoveEvent *e)
{
    // If we can decode the input source, there is a non-null item at the "move"
    // position, the playlist for that Item is non-null, is not the 
    // selected playlist and is not the CollectionList, then accept the event.
    //
    // Otherwise, do not accept the event.
    
    if(KURLDrag::canDecode(e) && itemAt(e->pos())) {
	Item *i = static_cast<Item *>(itemAt(e->pos()));

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
	    slotDrawContextMenu(i, e->globalPos());
	e->accept();
    }
    else {
	e->ignore();
	QListBox::mousePressEvent(e);
    }
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::slotPlaylistChanged(QListBoxItem *item)
{
    Item *i = dynamic_cast<Item *>(item);
    if(m_updatePlaylistStack && i && i->playlist())
	emit(currentChanged(i->playlist()));
}

void PlaylistBox::slotPlaylistDoubleClicked(QListBoxItem *)
{
    emit(doubleClicked());
}

void PlaylistBox::slotDrawContextMenu(QListBoxItem *item, const QPoint &point)
{
    Item *i = static_cast<Item *>(item);

    m_contextMenuOn = i;

    if(i)
	if(i->playlist() == CollectionList::instance())
	    m_collectionContextMenu->popup(point);
	else
	    m_playlistContextMenu->popup(point);
}

void PlaylistBox::contextSave()
{
    save(m_contextMenuOn);
}

void PlaylistBox::contextSaveAs()
{
    saveAs(m_contextMenuOn);
}

void PlaylistBox::contextRename()
{
    rename(m_contextMenuOn);
}

void PlaylistBox::contextDuplicate()
{
    duplicate(m_contextMenuOn);
}

void PlaylistBox::contextDeleteItem()
{
    deleteItem(m_contextMenuOn);
    m_contextMenuOn = 0;
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::Item::Item(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l) 
    : QObject(listbox), ListBoxPixmap(listbox, pix, text)
{
    list = l;
    setOrientation(Qt::Vertical);
    listbox->addName(text);

    connect(l, SIGNAL(nameChanged(const QString &)), this, SLOT(setName(const QString &)));
}

PlaylistBox::Item::Item(PlaylistBox *listbox, const QString &text, Playlist *l) 
    : ListBoxPixmap(listbox, SmallIcon("midi", 32), text)
{
    list = l;
    setOrientation(Qt::Vertical);
}

PlaylistBox::Item::~Item()
{

}

Playlist *PlaylistBox::Item::playlist() const
{
    return list;
}

PlaylistBox *PlaylistBox::Item::listBox() const
{
    return dynamic_cast<PlaylistBox *>(ListBoxPixmap::listBox());
}

void PlaylistBox::Item::setName(const QString &name)
{
    if(listBox()) {
	listBox()->m_names.remove(text());
	listBox()->m_names.append(name);

	setText(name);
	listBox()->updateItem(this);
    }
}

#include "playlistbox.moc"
