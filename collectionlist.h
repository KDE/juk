/***************************************************************************
                          collectionlist.h  -  description
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

#ifndef COLLECTIONLIST_H
#define COLLECTIONLIST_H

#include <qdict.h>

#include "playlist.h"
#include "playlistitem.h"

/** This is the "collection", or all of the music files that have been opened
    in any playlist and not explicitly removed from the collection.

    It is being implemented as a "semi-singleton" because I need universal access
    to just one instance.  However, because the collection needs initialization 
    parameters (that will not always be available when an instance is needed).  
    Hence there will be the familiar singleton "instance()" method allong with an
    "initialize()" method.
*/

class CollectionListItem;

class CollectionList : public Playlist
{
    friend class CollectionListItem;

    Q_OBJECT
public: 
    static CollectionList *instance();
    static void initialize(QWidget *parent);

    virtual void append(const QString &item);
    virtual void append(const QStringList &items);

    CollectionListItem *lookup(const QString &file);
    virtual PlaylistItem *createItem(const QFileInfo &file);
    
protected:
    CollectionList(QWidget *parent = 0);
    virtual ~CollectionList();

    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);

    virtual void appendImpl(const QString &item);

    void addToDict(const QString &file, CollectionListItem *item);
    void removeFromDict(const QString &file);

private:
    static CollectionList *list;
    QDict<CollectionListItem> itemsDict;
};

class CollectionListItem : public PlaylistItem
{
    Q_OBJECT
public:
    CollectionListItem(const QFileInfo &file);
    virtual ~CollectionListItem();

    void addChildItem(PlaylistItem *child);

public slots:
    virtual void refresh();
};

#endif
