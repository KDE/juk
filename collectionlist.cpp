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
    return(list);
}

void CollectionList::initialize(PlaylistSplitter *s, QWidget *parent, bool restoreOnLoad)
{
    list = new CollectionList(s, parent);

    if(restoreOnLoad)
	for(QDictIterator<Tag>it(*Cache::instance()); it.current(); ++it)
	    new CollectionListItem(it.current()->fileInfo());
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

QStringList CollectionList::artists() const
{
    return(artistList);
}

QStringList CollectionList::albums() const
{
    return(albumList);
}

CollectionListItem *CollectionList::lookup(const QString &file)
{
    return(itemsDict.find(file));
}

PlaylistItem *CollectionList::createItem(const QFileInfo &file, QListViewItem *)
{
    if(itemsDict.find(file.absFilePath()))
	return(0);

    return(new CollectionListItem(file));
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(PlaylistSplitter *s, QWidget *parent) : Playlist(s, parent, "collectionList")
{

}

CollectionList::~CollectionList()
{

}

void CollectionList::contentsDropEvent(QDropEvent *e)
{
    if(e->source() != this) {
	KURL::List urls;
	
	if(KURLDrag::decode(e, urls) && !urls.isEmpty()) {
	    
	    QStringList files;
	    
	    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
		files.append((*it).path());
	    
	    if(playlistSplitter())
		playlistSplitter()->add(files, this);
	}
    }
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
    if(artistList.contains(artist) == 0)
	artistList.append(artist);
}

void CollectionList::addAlbum(const QString &album)
{
    if(albumList.contains(album) == 0)
	albumList.append(album);
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public methods
////////////////////////////////////////////////////////////////////////////////

CollectionListItem::CollectionListItem(const QFileInfo &file) : PlaylistItem(CollectionList::instance())
{
    CollectionList *l = CollectionList::instance();
    if(l) {
	l->addToDict(file.absFilePath(), this);
	setData(Data::newUser(file));
	refresh();
	connect(this, SIGNAL(refreshed()), l, SIGNAL(dataChanged()));
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
