/***************************************************************************
                          playlist.cpp  -  description
                             -------------------
    begin                : Sat Feb 16 2002
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

#include <kmessagebox.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>

#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qptrlist.h>

#include "playlist.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Playlist::Playlist(QWidget *parent, const char *name) : KListView(parent, name)
{
    setup();
    setAcceptDrops(true);
    allowDuplicates = false;
}

Playlist::~Playlist()
{

}

void Playlist::append(const QString &item, bool sorted)
{
    collectionListChanged = false;

    QApplication::setOverrideCursor(Qt::waitCursor);
    appendImpl(item);
    QApplication::restoreOverrideCursor();
    
    if(collectionListChanged)
	emit(collectionChanged());
}

void Playlist::append(const QStringList &items, bool sorted)
{
    collectionListChanged = false;

    QApplication::setOverrideCursor(Qt::waitCursor);
    for(QStringList::ConstIterator it = items.begin(); it != items.end(); ++it)
        appendImpl(*it);
    QApplication::restoreOverrideCursor();

    if(collectionListChanged)
	emit(collectionChanged());
}

void Playlist::clearItems(const QPtrList<PlaylistItem> &items)
{
    QPtrListIterator<PlaylistItem> it(items);
    while(it.current()) {
        members.remove(it.current()->absFilePath());
        delete(it.current());
        ++it;
    }
}

QPtrList<PlaylistItem> Playlist::selectedItems() const
{
    QPtrList<PlaylistItem> list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i != 0; i = static_cast<PlaylistItem *>(i->itemBelow())) {
        if(i->isSelected())
            list.append(i);
    }
    return(list);
}

void Playlist::remove()
{
    remove(selectedItems());
}

void Playlist::remove(const QPtrList<PlaylistItem> &items)
{
    if(isVisible() && !items.isEmpty()) {
        QString message = i18n("Are you sure that you want to delete:\n");

	for(QPtrListIterator<PlaylistItem> it(items); it.current() != 0; ++it)
            message.append(it.current()->fileName() + "\n");

	if(KMessageBox::warningYesNo(this, message, i18n("Delete Files")) == KMessageBox::Yes) {
	    for(QPtrListIterator<PlaylistItem> it(items); it.current() != 0; ++it) {
		if(QFile::remove(it.current()->filePath()))
		    delete(it.current());
		else
		    KMessageBox::sorry(this, i18n("Could not save delete ") + it.current()->fileName() + ".");
	    }

	}
    }
}

QStringList &Playlist::getArtistList()
{
    return(artistList);
}

QStringList &Playlist::getAlbumList()
{
    return(albumList);
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

QDragObject *Playlist::dragObject()
{
    QPtrList<PlaylistItem> items = selectedItems();
    KURL::List urls;
    for(PlaylistItem *i = items.first(); i; i = items.next()) {
	KURL url;
	url.setPath(i->absFilePath());
	urls.append(url);
    }
    
    KURLDrag *drag = new KURLDrag(urls, this, "Playlist Items");
    drag->setPixmap(SmallIcon("sound"));

    return(drag);
}

void Playlist::contentsDropEvent(QDropEvent *e)
{
    QListViewItem *moveAfter = itemAt(e->pos());
    if(!moveAfter)
	moveAfter = lastItem();

    // This is slightly more efficient since it doesn't have to cast everything
    // to PlaylistItem.

    if(e->source() == this) {
	QPtrList<QListViewItem> items = KListView::selectedItems();
	
	for(QPtrListIterator<QListViewItem> it(items); it.current() != 0; ++it) {
	    (*it)->moveItem(moveAfter);
	    moveAfter = *it;
	}
    }
    else {
	KURL::List urls;
    
	if(KURLDrag::decode(e, urls) && !urls.isEmpty()) {
	    
	    QStringList files;
	    
	    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
		files.append((*it).path());
	    
	    append(files);
	}
    }
}

void Playlist::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(KURLDrag::canDecode(e))
	e->accept(true);
    else
	e->accept(false);
}

PlaylistItem *Playlist::createItem(const QFileInfo &file)
{
    CollectionListItem *item = CollectionList::instance()->lookup(file.absFilePath());

    if(item)
	return(new PlaylistItem(item, this));
    else if(CollectionList::instance()) {
	item = new CollectionListItem(file);
	collectionListChanged = true;
	return(new PlaylistItem(item, this));
    }
    else
	return(0);
}

void Playlist::appendImpl(const QString &item, bool sorted)
{
    processEvents();
    QFileInfo file(QDir::cleanDirPath(item));
    if(file.exists()) {
        if(file.isDir()) {
            QDir dir(file.filePath());
            QStringList dirContents=dir.entryList();
            for(QStringList::Iterator it = dirContents.begin(); it != dirContents.end(); ++it)
                if(*it != "." && *it != "..")
                    appendImpl(file.filePath() + QDir::separator() + *it);
        }
        else {
            QString extension = file.extension(false);
            if(extensions.contains(extension) > 0 && (members.contains(file.absFilePath()) == 0 || allowDuplicates)) {
                members.append(file.absFilePath());
                (void) createItem(file);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::setup()
{
    processed = 0;

    extensions.append("mp3");

    addColumn(i18n("Track Name"));
    addColumn(i18n("Artist"));
    addColumn(i18n("Album"));
    addColumn(i18n("Track"));
    addColumn(i18n("Genre"));
    addColumn(i18n("Year"));
    addColumn(i18n("Length"));
    addColumn(i18n("File Name"));

    setAllColumnsShowFocus(true);
    setSelectionMode(QListView::Extended);
    setShowSortIndicator(true);
    setItemMargin(3);

    setSorting(1);

    connect(this, SIGNAL(selectionChanged()), this, SLOT(emitSelected()));

    addColumn(QString::null);
    setResizeMode(QListView::LastColumn);
    // setFullWidth(true);
}

void Playlist::processEvents()
{
    if(processed == 0)
        qApp->processEvents();
    processed = ( processed + 1 ) % 10;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::emitSelected()
{
    emit(selectionChanged(selectedItems()));
}

#include "playlist.moc"
