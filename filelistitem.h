/***************************************************************************
                          filelistitem.h  -  description
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

#ifndef FILELISTITEM_H
#define FILELISTITEM_H

#include <klistview.h>
#include <qfileinfo.h>

#include "tag.h"
#include "MPEGHeader.h"

class FileListItem : public KListViewItem  {
public: 
  FileListItem(QFileInfo *file, KListView *parent);
  ~FileListItem();

  QFileInfo *getFileInfo();
  Tag *getTag();
  MPEGHeader *getHeader();

protected:

private:
  int compare(QListViewItem *item, int column, bool ascending) const;
  int compare(FileListItem *firstItem, FileListItem *secondItem, int column, bool ascending) const;
  QFileInfo *fileInfo;
  Tag *tag;
  MPEGHeader *header;
};

#endif
