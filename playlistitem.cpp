/***************************************************************************
                          playlistitem.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#include <kdebug.h>

#include "playlistitem.h"
#include "playlist.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::~PlaylistItem()
{
    data->deleteUser();
}

void PlaylistItem::setFile(const QString &file)
{
    data->setFile(file);
    refresh();
}

Tag *PlaylistItem::tag() const
{
    return(data->tag());
}

// QFileInfo-ish methods

QString PlaylistItem::fileName() const { return(data->fileName()); }
QString PlaylistItem::filePath() const { return(data->filePath()); }
QString PlaylistItem::absFilePath() const { return(data->absFilePath()); }
QString PlaylistItem::dirPath(bool absPath) const { return(data->dirPath(absPath)); }
bool PlaylistItem::isWritable() const { return(data->isWritable()); }

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistItem::refresh()
{
    // This signal will be received by the "parent" CollectionListItem which will
    // in turn call refreshImpl() for all of its children, including this item.
    emit(refreshed());
}

void PlaylistItem::refreshFromDisk()
{
    data->refresh();
    refresh();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::PlaylistItem(CollectionListItem *item, Playlist *parent) : QObject(parent), KListViewItem(parent)
{
    setup(item, parent);
}

PlaylistItem::PlaylistItem(CollectionListItem *item, Playlist *parent, QListViewItem *after) : QObject(parent), KListViewItem(parent, after)
{
    setup(item, parent);
}

PlaylistItem::PlaylistItem(Playlist *parent) : QObject(parent), KListViewItem(parent)
{
    setDragEnabled(true);
}

PlaylistItem::Data *PlaylistItem::getData()
{
    return(data);
}

void PlaylistItem::setData(Data *d)
{
    data = d;
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem protected slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistItem::refreshImpl()
{
    // This should be the only function that needs to be rewritten if the structure of    
    // PlaylistItemData changes.  

    setText(TrackColumn,       tag()->track());
    setText(ArtistColumn,      tag()->artist());
    setText(AlbumColumn,       tag()->album());
    setText(TrackNumberColumn, tag()->trackNumberString());
    setText(GenreColumn,       tag()->genre());
    setText(YearColumn,        tag()->yearString());
    setText(LengthColumn,      tag()->lengthString());
    setText(FileNameColumn,    filePath());
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistItem::setup(CollectionListItem *item, Playlist *parent)
{
    if(item) {
	data = item->getData()->newUser();
	item->addChildItem(this);
	refreshImpl();
	connect(this, SIGNAL(refreshed()), parent, SIGNAL(dataChanged()));
    }

    setDragEnabled(true);
}

int PlaylistItem::compare(QListViewItem *item, int column, bool ascending) const
{
    // reimplemented from QListViewItem

    // This is pretty ugly.  This needs to be a const method to match the
    // signature from QListViewItem::compare(), but for our purposes, we need
    // to be able to call non-const methods, so we're casting this to a 
    // non-const pointer.  Yuck.

    PlaylistItem *playlistItem = dynamic_cast<PlaylistItem *>(item);

    // The following statments first check to see if you can sort based on the
    // specified column.  If the values for the two PlaylistItems are the same
    // in that column it then trys to sort based on columns 1, 2, 3 and 0,
    // (artist, album, track number, track name) in that order.

    if(playlistItem) {
        if(compare(this, playlistItem, column, ascending) != 0)
            return(compare(this, playlistItem, column, ascending));
        else {
            for(int i = ArtistColumn; i <= TrackNumberColumn; i++) {
                if(compare(this, playlistItem, i, ascending) != 0)
                    return(compare(this, playlistItem, i, ascending));
            }
            if(compare(this, playlistItem, TrackColumn, ascending) != 0)
                return(compare(this, playlistItem, TrackColumn, ascending));
            return(0);
        }
    }
    else
        return(0); // cast failed, something is wrong
}

int PlaylistItem::compare(const PlaylistItem *firstItem, const PlaylistItem *secondItem, int column, bool ascending) const
{
    if(column == TrackNumberColumn) {
        if(firstItem->tag()->trackNumber() > secondItem->tag()->trackNumber())
            return(1);
        else if(firstItem->tag()->trackNumber() < secondItem->tag()->trackNumber())
            return(-1);
        else
            return(0);
    }
    else if(column == LengthColumn) {
        if(firstItem->tag()->seconds() > secondItem->tag()->seconds())
            return(1);
        else if(firstItem->tag()->seconds() < secondItem->tag()->seconds())
            return(-1);
        else
            return(0);
    }
    else
        return(firstItem->key(column, ascending).compare(secondItem->key(column, ascending)));
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem::Data public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::Data *PlaylistItem::Data::newUser(const QFileInfo &file)
{
    return new Data(file);
}

PlaylistItem::Data *PlaylistItem::Data::newUser()
{
    referenceCount++;
    return(this);
}

void PlaylistItem::Data::refresh()
{
    delete(dataTag);
    dataTag = Tag::createTag(filePath());
}

void PlaylistItem::Data::deleteUser()
{
    // The delete(this) is safe because we control object creation through a
    // protected constructor and the newUser() methods.

    if(--referenceCount == 0)
        delete(this);
}

Tag *PlaylistItem::Data::tag() const
{
    return(dataTag);
}

void PlaylistItem::Data::setFile(const QString &file)
{
    delete(dataTag);
    dataTag = 0;

    QFileInfo::setFile(file);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem::Data protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::Data::Data(const QFileInfo &file) : QFileInfo(file)
{
    referenceCount = 1;
    dataTag = Tag::createTag(filePath());
}

PlaylistItem::Data::~Data()
{
    delete(dataTag);
}

#include "playlistitem.moc"
