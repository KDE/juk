/***************************************************************************
                          collectionlist.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
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

#include <kurl.h>
#include <kurldrag.h>
#include <kdebug.h>

#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

CollectionList *CollectionList::list = 0;

CollectionList *CollectionList::instance()
{
    return(list);
}

void CollectionList::initialize(QWidget *parent)
{
    list = new CollectionList(parent);
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

void CollectionList::add(const QString &item, bool sorted)
{
    Playlist::add(item);
    emit(collectionChanged());
}

void CollectionList::add(const QStringList &items, bool sorted)
{
    Playlist::add(items);
    emit(collectionChanged());
}

CollectionListItem *CollectionList::lookup(const QString &file)
{
    return(itemsDict.find(file));
}

PlaylistItem *CollectionList::createItem(const QFileInfo &file, bool sorted)
{
    PlaylistItem *item = new CollectionListItem(file);
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionListItem::refresh()
{
    refreshImpl();
    
    // This is connected to refreshImpl() for all of the items children.
    emit(refreshed());
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(QWidget *parent) : Playlist(parent, "collectionList")
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
	    
	    add(files);
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

void CollectionList::addImpl(const QString &item)
{
    if(!lookup(item))
	Playlist::addImpl(item);
}

void CollectionList::addToDict(const QString &file, CollectionListItem *item)
{
    itemsDict.replace(file, item);
}

void CollectionList::removeFromDict(const QString &file)
{
    itemsDict.remove(file);
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

#include "collectionlist.moc"
