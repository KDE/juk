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

FileListItem::FileListItem(QFileInfo *file, KListView *parent) : KListViewItem(parent)
{
  header = 0;
  fileInfo = file;
  tag = new Tag(fileInfo->filePath());

  refresh();
}

FileListItem::~FileListItem()
{
  if(tag)
    delete(tag);
 
  if(header)
    delete(header);
}

QFileInfo *FileListItem::getFileInfo()
{
  return(fileInfo);
}

Tag *FileListItem::getTag()
{
  if(!tag) {
    tag = new Tag(fileInfo->filePath());
  }
  return(tag);
}

MPEGHeader *FileListItem::getHeader()
{
  if(!header) {
    header = new MPEGHeader(fileInfo->filePath());
  }
  return(header);
}

void FileListItem::setFile(QString fileName)
{
  if(fileInfo) 
    fileInfo->setFile(fileName);
  
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
  setText(0, tag->getTrack());
  setText(1, tag->getArtist());
  setText(2, tag->getAlbum());
  setText(3, tag->getTrackNumberString());
  setText(4, tag->getGenre());
  setText(5, tag->getYearString());
  setText(6, fileInfo->filePath());
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
      for(int i = 1; i <= 3; i++) {
	if(compare(thisFileListItem, fileListItem, i, ascending) != 0)
	  return(compare(thisFileListItem, fileListItem, i, ascending));
      }
      if(compare(thisFileListItem, fileListItem, 0, ascending) != 0)
	return(compare(thisFileListItem, fileListItem, 0, ascending));
      return(0);
    }
  }
  else 
    return(0); // cast failed, something is wrong
}

int FileListItem::compare(FileListItem *firstItem, FileListItem *secondItem, int column, bool ascending) const
{
  if(column == 3) {
    if(firstItem->getTag()->getTrackNumber() > secondItem->getTag()->getTrackNumber())
      return(1);
    else if(firstItem->getTag()->getTrackNumber() < secondItem->getTag()->getTrackNumber())
      return(-1);
    else
      return(0);
  }
  else {
    return(firstItem->key(column, ascending).compare(secondItem->key(column, ascending)));
  }
}
