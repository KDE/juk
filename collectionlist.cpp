/***************************************************************************
                          collectionlist.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
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

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qtimer.h>

#include "collectionlist.h"
#include "cache.h"
#include "splashscreen.h"

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

CollectionList *CollectionList::list = 0;

CollectionList *CollectionList::instance()
{
    return list;
}

void CollectionList::initialize(QWidget *parent, bool restoreOnLoad)
{
    list = new CollectionList(parent);

    if(restoreOnLoad)
	for(QDictIterator<Tag>it(*Cache::instance()); it.current(); ++it)
	    new CollectionListItem(it.current()->fileInfo(), it.current()->absFilePath());
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem *CollectionList::createItem(const QFileInfo &file, QListViewItem *)
{
    QString filePath = resolveSymLinks(file);

    if(m_itemsDict.find(filePath))
	return 0;

    return new CollectionListItem(file, filePath);
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionList::clear()
{
    int result = KMessageBox::warningYesNo(this, 
			      i18n("Removing an item from the collection will also remove it from "
				   "all of your playlists.  Are you sure you want to continue?  \n\n"
				   "Note however that if the directory that these files are in are in "
				   "your scan on startup list, then they will be readded on startup."));
    if(result == KMessageBox::Yes)			      
	Playlist::clear();

    emit signalCollectionChanged();
}

void CollectionList::slotCheckCache()
{
    for(QDictIterator<CollectionListItem>it(m_itemsDict); it.current(); ++it) {
	it.current()->checkCurrent();
	kapp->processEvents();
    }
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(QWidget *parent) : Playlist(parent, i18n("Collection List"))
{

}

CollectionList::~CollectionList()
{

}

void CollectionList::decode(QMimeSource *s)
{
    KURL::List urls;
    
    if(!KURLDrag::decode(s, urls) || urls.isEmpty())
	return;
	
    QStringList files;
	
    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	files.append((*it).path());
	
    emit signalFilesDropped(files, this);
}

void CollectionList::contentsDropEvent(QDropEvent *e)
{
    if(e->source() == this)
	return;
    else
	decode(e);
}

void CollectionList::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(KURLDrag::canDecode(e) && e->source() != this)
	e->accept(true);
    else
	e->accept(false);
}

void CollectionList::addArtist(const QString &artist)
{
    // Do a bit of caching since there will very often be "two in a row" insertions.
    static QString previousArtist;

    if(artist != previousArtist && !m_artists.insert(artist))
	previousArtist = artist;
}

void CollectionList::addAlbum(const QString &album)
{
    // Do a bit of caching since there will very often be "two in a row" insertions.
    static QString previousAlbum;

    if(album != previousAlbum && !m_albums.insert(album))
	previousAlbum = album;
}
////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionListItem::slotRefresh()
{
    slotRefreshImpl();
    
    if(CollectionList::instance()) {
	CollectionList::instance()->addArtist(text(ArtistColumn));
	CollectionList::instance()->addAlbum(text(AlbumColumn));	
    }
    // This is connected to slotRefreshImpl() for all of the items children.
    emit(signalRefreshed());
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionListItem::CollectionListItem(const QFileInfo &file, const QString &path) : PlaylistItem(CollectionList::instance())
{
    CollectionList *l = CollectionList::instance();
    if(l) {
	l->addToDict(path, this);
	setData(Data::newUser(file, path));
	slotRefresh();
	connect(this, SIGNAL(signalRefreshed()), l, SIGNAL(signalDataChanged()));
	l->emitNumberOfItemsChanged();
    }
    else
	kdError() << "CollectionListItems should not be created before"
		  << "CollectionList::initialize() has been called." << endl;

    SplashScreen::increment();
}

CollectionListItem::~CollectionListItem()
{
    CollectionList *l = CollectionList::instance();
    if(l)
	l->removeFromDict(Playlist::resolveSymLinks(*data()));
}

void CollectionListItem::addChildItem(PlaylistItem *child)
{
    connect(child, SIGNAL(signalRefreshed()), this, SLOT(slotRefresh()));
    connect(this, SIGNAL(signalRefreshed()), child, SLOT(slotRefreshImpl()));
}

void CollectionListItem::checkCurrent()
{
    if(!data()->exists() || !data()->isFile())
	CollectionList::instance()->clearItem(this);
    else if(!data()->tag()->current())
	data()->refresh();
}

#include "collectionlist.moc"
