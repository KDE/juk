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

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FileListItem::FileListItem(QFileInfo *file, KListView *parent) : QObject(parent), KListViewItem(parent), QFileInfo(*file)
{
  header = 0;
  tag = new Tag(filePath());
  
  refresh();
}

FileListItem::FileListItem(FileListItem *item, KListView *parent) : QObject(parent), KListViewItem(parent), QFileInfo(*item)
{
  //  kdDebug() << "FileListItem(FileListItem *item, KListView *parent)" << endl;

  if(item) {
    tag = item->getTag();
    header = item->getHeader();
    connect(item, SIGNAL(destroyed(FileListItem *)), this, SLOT(parentDestroyed(FileListItem *)));
    addSibling(item);
    
    refresh();
  }
}

FileListItem::~FileListItem()
{
  if(tag)
    delete(tag);
 
  if(header)
    delete(header);

  emit(destroyed(this));
}

Tag *FileListItem::getTag()
{
  if(!tag)
    tag = new Tag(filePath());
  return(tag);
}

MPEGHeader *FileListItem::getHeader()
{
  if(!header) {
    header = new MPEGHeader(filePath());
  }
  return(header);
}

void FileListItem::setFile(QString fileName)
{
  setFile(fileName);
  
  if(header) {
    delete(header);
    (void) getHeader();
  }
  if(tag) {
    delete(tag);
    (void) getTag();
  }
}

void FileListItem::refresh()
{
  setText(TrackColumn,       getTag()->getTrack());
  setText(ArtistColumn,      getTag()->getArtist());
  setText(AlbumColumn,       getTag()->getAlbum());
  setText(TrackNumberColumn, getTag()->getTrackNumberString());
  setText(GenreColumn,       getTag()->getGenre());
  setText(YearColumn,        getTag()->getYearString());
  setText(LengthColumn,      getHeader()->getLengthChar());
  setText(FileNameColumn,    filePath());

  emit(refreshed());
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void FileListItem::addSibling(FileListItem *sibling)
{
  connect(sibling, SIGNAL(refreshed()), this, SLOT(refresh()));
  connect(sibling, SIGNAL(destroyed(FileListItem *)), this, SLOT(removeSibling(FileListItem *)));
}

void FileListItem::removeSibling(FileListItem *sibling)
{
  disconnect(sibling, SIGNAL(refreshed()), this, SLOT(refresh()));
  disconnect(sibling, SIGNAL(destroyed(FileListItem *)), this, SLOT(removeSibling(FileListItem *)));  
}

////////////////////////////////////////////////////////////////////////////////
// private methods
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
    if(compare(thisFileListItem, fileListItem, column, ascending) != 0) {
      return(compare(thisFileListItem, fileListItem, column, ascending));
    }
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
    if(firstItem->getHeader()->getLength() > secondItem->getHeader()->getLength())
      return(1);
    else if(firstItem->getHeader()->getLength() < secondItem->getHeader()->getLength())
      return(-1);
    else
      return(0);    
  }
  else {
    return(firstItem->key(column, ascending).compare(secondItem->key(column, ascending)));
  }
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void FileListItem::parentDestroyed(FileListItem *parent)
{
  header = 0;
  tag = 0;
  disconnect(parent, SIGNAL(destroyed(FileListItem *)), this, SLOT(parentDestroyed(FileListItem *)));
}
