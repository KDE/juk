/***************************************************************************
                          filelistitem.cpp  -  description
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

#include "filelistitem.h"
#include "filelist.h"
#include "cache.h"

class FileListItem::Data : public QFileInfo
{
public:
    static Data *newUser(const QFileInfo &file);
    Data *newUser();
    void deleteUser();

    Tag *getTag();
    AudioData *getAudioData();

    void setFile(const QString &file);

protected:
    // Because we're trying to use this as a shared item, we want all access
    // to be through pointers (so that it's safe to use delete this).  Thus
    // creation of the object should be done by the newUser methods above
    // and deletion should be handled by deleteUser.  Making the constructor
    // and destructor private ensures this.

    Data(const QFileInfo &file);    
    virtual ~Data();

private:
    int referenceCount;

    Cache::Item *cache;
    Tag *tag;
    AudioData *audioData;
};

////////////////////////////////////////////////////////////////////////////////
// FileListItem::Data public methods
////////////////////////////////////////////////////////////////////////////////

FileListItem::Data *FileListItem::Data::newUser(const QFileInfo &file)
{
    return(new Data(file));
}

FileListItem::Data *FileListItem::Data::newUser()
{
    referenceCount++;
    return(this);
}

void FileListItem::Data::deleteUser()
{
    if(--referenceCount == 0)
        delete(this);
}

Tag *FileListItem::Data::getTag()
{
    if(!tag)
        tag = new Tag(filePath());
    return(tag);
}

AudioData *FileListItem::Data::getAudioData()
{
    if(!audioData) {
        audioData = new AudioData(filePath());
    }
    return(audioData);
}

void FileListItem::Data::setFile(const QString &file)
{
    delete(tag);
    tag = 0;

    QFileInfo::setFile(file);
}

////////////////////////////////////////////////////////////////////////////////
// FileListItem::Data protected methods
////////////////////////////////////////////////////////////////////////////////

FileListItem::Data::Data(const QFileInfo &file) : QFileInfo(file)
{
    referenceCount = 1;

    // initialize pointers to null
    cache = 0;
    tag = 0;
    audioData = 0;
}

FileListItem::Data::~Data()
{
    delete(cache);
    delete(tag);
    delete(audioData);
}

////////////////////////////////////////////////////////////////////////////////
// FileListItem public methods
////////////////////////////////////////////////////////////////////////////////

FileListItem::FileListItem(const QFileInfo &file, FileList *parent) : QObject(parent), KListViewItem(parent)
{
    data = Data::newUser(file);
    refresh();
    connect(this, SIGNAL(refreshed()), parent, SIGNAL(dataChanged()));
}

FileListItem::FileListItem(FileListItem &item, FileList *parent) : QObject(parent), KListViewItem(parent)
{
    data = item.getData()->newUser();
    //  connect(&item, SIGNAL(destroyed(FileListItem *)), this, SLOT(parentDestroyed(FileListItem *)));
    addSibling(&item);

    refresh();
    connect(this, SIGNAL(refreshed()), parent, SIGNAL(dataChanged()));
}

FileListItem::~FileListItem()
{
    data->deleteUser();
}

void FileListItem::setFile(const QString &file)
{
    data->setFile(file);
    refresh();
}

FileListItem::Data *FileListItem::getData()
{
    return(data);
}

Tag *FileListItem::getTag()
{
    return(data->getTag());
}

AudioData *FileListItem::getAudioData()
{
    return(data->getAudioData());
}

// QFileInfo-ish methods

QString FileListItem::fileName() const { return(data->fileName()); }
QString FileListItem::filePath() const { return(data->filePath()); }
QString FileListItem::absFilePath() const { return(data->absFilePath()); }
QString FileListItem::dirPath(bool absPath) const { return(data->dirPath(absPath)); }
bool FileListItem::isWritable() const { return(data->isWritable()); }

////////////////////////////////////////////////////////////////////////////////
// FileListItem public slots
////////////////////////////////////////////////////////////////////////////////

void FileListItem::refresh()
{
    // This should be the only function that needs to be rewritten if the structure of
    // FileListItemData changes.  Also, currently this also inserts things into the
    // album and artist registries of the FileList.  If something like sorted insert
    // happens at some point it could either be implemented here or in a subclass of
    // QValueList.  And another note: the artist/album registry doesn't remove items
    // when they no longer exist in the list view.  I decided that this is too much work
    // for something not very useful at the moment, but at some point, a QValueList of
    // a subclass of QPair could track such things...

    // if the text has changed and the artist registry of the FileList doens't contain
    // this artist, add it to the mentioned registry

    FileList *fileList = static_cast<FileList *>(listView());

    if(text(ArtistColumn) != getTag()->getArtist() &&
       fileList->getArtistList().contains(getTag()->getArtist()) == 0)
        fileList->getArtistList().append(getTag()->getArtist());

    if(text(AlbumColumn) != getTag()->getAlbum() &&
       fileList->getAlbumList().contains(getTag()->getAlbum()) == 0)
        fileList->getAlbumList().append(getTag()->getAlbum());

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
	// FileListItem::Data.  I'll work on that later.

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

	emit(refreshed());

    }
}

////////////////////////////////////////////////////////////////////////////////
// FileListItem protected slots
////////////////////////////////////////////////////////////////////////////////

void FileListItem::addSibling(const FileListItem *sibling)
{
    connect(sibling, SIGNAL(refreshed()), this, SLOT(refresh()));
}

void FileListItem::removeSibling(const FileListItem *sibling)
{
    disconnect(sibling, SIGNAL(refreshed()), this, SLOT(refresh()));
}

////////////////////////////////////////////////////////////////////////////////
// FileListItem private methods
////////////////////////////////////////////////////////////////////////////////

int FileListItem::compare(QListViewItem *item, int column, bool ascending) const
{
    // reimplemented from QListViewItem

    FileListItem *fileListItem = dynamic_cast<FileListItem *>(item);
    FileListItem *thisFileListItem = const_cast<FileListItem *>(this);

    // The following statments first check to see if you can sort based on the
    // specified column.  If the values for the two FileListItems are the same
    // in that column it then trys to sort based on columns 1, 2, 3 and 0,
    // (artist, album, track number, track name) in that order.

    if(fileListItem && thisFileListItem) {
        if(compare(thisFileListItem, fileListItem, column, ascending) != 0)
            return(compare(thisFileListItem, fileListItem, column, ascending));
        else {
            for(int i = ArtistColumn; i <= TrackNumberColumn; i++) {
                if(compare(thisFileListItem, fileListItem, i, ascending) != 0)
                    return(compare(thisFileListItem, fileListItem, i, ascending));
            }
            if(compare(thisFileListItem, fileListItem, TrackColumn, ascending) != 0)
                return(compare(thisFileListItem, fileListItem, TrackColumn, ascending));
            return(0);
        }
    }
    else
        return(0); // cast failed, something is wrong
}

int FileListItem::compare(FileListItem *firstItem, FileListItem *secondItem, int column, bool ascending) const
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
