/***************************************************************************
                          filelistitemdata.h  -  description
                             -------------------
    begin                : Fri Mar 22 2002
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

#ifndef FILELISTITEMDATA_H
#define FILELISTITEMDATA_H

#include <qfileinfo.h>

#include "tag.h"
#include "cacheitem.h"
#include "audiodata.h"

class FileListItemData : public QFileInfo
{
public: 
  FileListItemData(QFileInfo *file);
  ~FileListItemData();

  FileListItemData *newUser();
  void deleteUser();

  Tag *getTag();
  AudioData *getAudioData();

  void setFile(QString file);

private:
  int referenceCount;

  CacheItem *cache;
  Tag *tag;
  AudioData *audioData;
};

#endif
