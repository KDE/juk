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
#include <qobject.h>

#include "tag.h"
#include "audiodata.h"
#include "filelistitemdata.h"

class FileListItem : public QObject, public KListViewItem {
  Q_OBJECT
public: 
  enum ColumnType { TrackColumn = 0, ArtistColumn = 1, AlbumColumn = 2, TrackNumberColumn = 3, 
		    GenreColumn = 4, YearColumn = 5, LengthColumn = 6, FileNameColumn = 7 };
  
  FileListItem(QFileInfo &file, KListView *parent);
  FileListItem(FileListItem &item, KListView *parent);
  ~FileListItem();

  FileListItemData *getData();
  Tag *getTag();
  AudioData *getAudioData();

  void setFile(QString file);
  
  // QFileInfo-ish methods
  QString fileName() const;
  QString filePath() const;
  QString absFilePath() const;
  QString dirPath(bool absPath = false) const;
  bool isWritable() const;

public slots:
  void refresh();
  void addSibling(FileListItem *sibling);
  void removeSibling(FileListItem *sibling);

signals:
  void refreshed();

private:
  int compare(QListViewItem *item, int column, bool ascending) const;
  int compare(FileListItem *firstItem, FileListItem *secondItem, int column, bool ascending) const;

  FileListItemData *data;
};

#endif
