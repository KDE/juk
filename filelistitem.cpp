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

FileListItem::FileListItem(QFileInfo *file, QListView *parent) : QListViewItem(parent)
{
  header = 0;
  fileInfo = file;
  tag = new Tag(fileInfo->filePath());

  setText(0, tag->getTrack());
  setText(1, tag->getArtist());
  setText(2, tag->getAlbum());
  setText(3, tag->getTrackNumberString());
  setText(4, tag->getGenre());
  setText(5, tag->getYearString());
  setText(6, fileInfo->filePath());
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
