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
#include <kdebug.h>

#include "collectionlist.h"
#include "playlistsplitter.h"
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

void CollectionList::initialize(PlaylistSplitter *s, QWidget *parent, bool restoreOnLoad)
{
    list = new CollectionList(s, parent);

    if(restoreOnLoad)
	for(QDictIterator<Tag>it(*Cache::instance()); it.current(); ++it)
	    new CollectionListItem(it.current()->fileInfo(), it.current()->absFilePath());
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

QStringList CollectionList::artists() const
{
    return artistList.values();
}

QStringList CollectionList::albums() const
{
    return albumList.values();
}

CollectionListItem *CollectionList::lookup(const QString &file)
{
    return itemsDict.find(file);
}

PlaylistItem *CollectionList::createItem(const QFileInfo &file, QListViewItem *)
{
    QString path = file.absFilePath();
    if(itemsDict.find(path))
	return 0;

    return new CollectionListItem(file, path);
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(PlaylistSplitter *s, QWidget *parent) : Playlist(s, parent, i18n("Collection List"))
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
	
    if(playlistSplitter())
	playlistSplitter()->add(files, this);
}

void CollectionList::contentsDropEvent(QDropEvent *e)
{
    if(e->source() == this)
	return;


}

void CollectionList::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(KURLDrag::canDecode(e) && e->source() != this)
	e->accept(true);
    else
	e->accept(false);
}

void CollectionList::addToDict(const QString &file, CollectionListItem *item)
{
    itemsDict.replace(file, item);
}

void CollectionList::removeFromDict(const QString &file)
{
    itemsDict.remove(file);
}

void CollectionList::addArtist(const QString &artist)
{
    // Do a bit of caching since there will very often be "two in a row" insertions.
    static QString previousArtist;

    if(artist != previousArtist && !artistList.insert(artist))
	previousArtist = artist;
}

void CollectionList::addAlbum(const QString &album)
{
    // Do a bit of caching since there will very often be "two in a row" insertions.
    static QString previousAlbum;

    if(album != previousAlbum && !albumList.insert(album))
	previousAlbum = album;
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public methods
////////////////////////////////////////////////////////////////////////////////

CollectionListItem::CollectionListItem(const QFileInfo &file, const QString &path) : PlaylistItem(CollectionList::instance())
{
    CollectionList *l = CollectionList::instance();
    if(l) {
	l->addToDict(path, this);
	setData(Data::newUser(file, path));
	refresh();
	connect(this, SIGNAL(refreshed()), l, SIGNAL(dataChanged()));
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
	l->removeFromDict(getData()->absFilePath());
}

void CollectionListItem::addChildItem(PlaylistItem *child)
{
    connect(child, SIGNAL(refreshed()), this, SLOT(refresh()));
    connect(this, SIGNAL(refreshed()), child, SLOT(refreshImpl()));   
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionListItem::refresh()
{
    refreshImpl();
    
    if(CollectionList::instance()) {
	CollectionList::instance()->addArtist(text(ArtistColumn));
	CollectionList::instance()->addAlbum(text(AlbumColumn));	
    }
    // This is connected to refreshImpl() for all of the items children.
    emit(refreshed());
}

#include "collectionlist.moc"
