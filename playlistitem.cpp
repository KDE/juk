/***************************************************************************
                          playlistitem.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

Tag *PlaylistItem::getTag()
{
    return(data->getTag());
}

AudioData *PlaylistItem::getAudioData()
{
    return(data->getAudioData());
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

PlaylistItem::PlaylistItem(CollectionListItem *item, Playlist *parent, PlaylistItem *after) : QObject(parent), KListViewItem(parent, after)
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
    // PlaylistItemData changes.  Also, currently this also inserts things into the
    // album and artist registries of the Playlist.  If something like sorted insert
    // happens at some point it could either be implemented here or in a subclass of
    // QValueList.  And another note: the artist/album registry doesn't remove items
    // when they no longer exist in the list view.  I decided that this is too much work
    // for something not very useful at the moment, but at some point, a QValueList of
    // a subclass of QPair could track such things...

    // if the text has changed and the artist registry of the Playlist doens't contain
    // this artist, add it to the mentioned registry

    if(Cache::instance()->item(absFilePath())) {

	// ... do stuff relative to a cache hit.  At the moment this will never 
	// happen since the cache isn't yet implemented.

	// The current though is that "Tag" and "Cache::Item" should share a 
	// common virtual interface so that the below code could actually be 
	// used through that interface.  In this if block it would just be
	// decided what object the meta data was going to come from and then
	// something like metaData->getArtist() could be used, where metaData
	// is either a Tag or a Cache::Item.

        // And on further thought this should also be hidden in 
	// PlaylistItem::Data.  I'll work on that later.

    }
    else {
	
	setText(TrackColumn,       getTag()->getTrack());
	setText(ArtistColumn,      getTag()->getArtist());
	setText(AlbumColumn,       getTag()->getAlbum());
	setText(TrackNumberColumn, getTag()->getTrackNumberString());
	setText(GenreColumn,       getTag()->getGenre());
	setText(YearColumn,        getTag()->getYearString());
	setText(LengthColumn,      getAudioData()->getLengthChar());
	setText(FileNameColumn,    filePath());
    }
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
    PlaylistItem *thisPlaylistItem = const_cast<PlaylistItem *>(this);

    // The following statments first check to see if you can sort based on the
    // specified column.  If the values for the two PlaylistItems are the same
    // in that column it then trys to sort based on columns 1, 2, 3 and 0,
    // (artist, album, track number, track name) in that order.

    if(playlistItem && thisPlaylistItem) {
        if(compare(thisPlaylistItem, playlistItem, column, ascending) != 0)
            return(compare(thisPlaylistItem, playlistItem, column, ascending));
        else {
            for(int i = ArtistColumn; i <= TrackNumberColumn; i++) {
                if(compare(thisPlaylistItem, playlistItem, i, ascending) != 0)
                    return(compare(thisPlaylistItem, playlistItem, i, ascending));
            }
            if(compare(thisPlaylistItem, playlistItem, TrackColumn, ascending) != 0)
                return(compare(thisPlaylistItem, playlistItem, TrackColumn, ascending));
            return(0);
        }
    }
    else
        return(0); // cast failed, something is wrong
}

int PlaylistItem::compare(PlaylistItem *firstItem, PlaylistItem *secondItem, int column, bool ascending) const
{
    if(column == TrackNumberColumn) {
        if(firstItem->getTag()->getTrackNumber() > secondItem->getTag()->getTrackNumber())
            return(1);
        else if(firstItem->getTag()->getTrackNumber() < secondItem->getTag()->getTrackNumber())
            return(-1);
        else
            return(0);
    }
    else if(column == LengthColumn) {
        if(firstItem->getAudioData()->getLength() > secondItem->getAudioData()->getLength())
            return(1);
        else if(firstItem->getAudioData()->getLength() < secondItem->getAudioData()->getLength())
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
    return(new Data(file));
}

PlaylistItem::Data *PlaylistItem::Data::newUser()
{
    referenceCount++;
    return(this);
}

void PlaylistItem::Data::refresh()
{
    delete(cache);
    delete(tag);
    delete(audioData);

    cache = 0;
    tag = 0;
    audioData = 0;
}

void PlaylistItem::Data::deleteUser()
{
    // The delete(this) is safe because we control object creation through a
    // protected constructor and the newUser() methods.

    if(--referenceCount == 0)
        delete(this);
}

Tag *PlaylistItem::Data::getTag()
{
    if(!tag)
        tag = new Tag(filePath());
    return(tag);
}

AudioData *PlaylistItem::Data::getAudioData()
{
    if(!audioData)
        audioData = new AudioData(filePath());
    return(audioData);
}

void PlaylistItem::Data::setFile(const QString &file)
{
    delete(tag);
    tag = 0;

    QFileInfo::setFile(file);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem::Data protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::Data::Data(const QFileInfo &file) : QFileInfo(file)
{
    referenceCount = 1;

    // initialize pointers to null
    cache = 0;
    tag = 0;
    audioData = 0;
}

PlaylistItem::Data::~Data()
{
    // Create our cache "on the way out" to avoid having lots of duplicate copies
    // of the same information in memory.  This checks to see if the item is in
    // the cache and 

    if(tag && !cache && !Cache::instance()->isEmpty() && !Cache::instance()->find(absFilePath()) )
	Cache::instance()->replace(absFilePath(), new CacheItem(*tag));

    delete(tag);
    delete(audioData);
}

#include "playlistitem.moc"
