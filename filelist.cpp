/***************************************************************************
                          filelist.cpp  -  description
                             -------------------
    begin                : Sat Feb 16 2002
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

#include <klocale.h>

#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qapplication.h>

#include "filelist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

FileList::FileList(QWidget *parent, const char *name) : KListView(parent, name)
{
  setup();
}

FileList::FileList(QString item, QWidget *parent, const char *name) : KListView(parent, name)
{
  setup();
  append(item);
}

FileList::FileList(QStringList *items, QWidget *parent, const char *name) : KListView(parent, name)
{
  setup();
  append(items);
}

FileList::~FileList()
{
}

void FileList::append(QString item)
{
  QApplication::setOverrideCursor(Qt::waitCursor);
  appendImpl(item);
  QApplication::restoreOverrideCursor();
}

void FileList::append(QStringList *items)
{
  QApplication::setOverrideCursor(Qt::waitCursor);
  for(QStringList::Iterator it = items->begin(); it != items->end(); ++it)
    appendImpl(*it);
  QApplication::restoreOverrideCursor();
}

void FileList::append(FileListItem *item)
{
  
}

void FileList::append(QPtrList<QListViewItem> *items)
{
  //  for(QPtrListIterator it = items->begin(); it != items->end(); ++it)
  //    append(QString((*it)));
}

FileListItem *FileList::getSelectedItem()
{
  return(dynamic_cast<FileListItem *>(currentItem()));
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void FileList::setup()
{
  extensions.append("mp3");

  addColumn(i18n("Track Name"));
  addColumn(i18n("Artist"));
  addColumn(i18n("Album"));
  addColumn(i18n("Track"));
  addColumn(i18n("Genre"));
  addColumn(i18n("Year"));
  addColumn(i18n("File Name"));

  setAllColumnsShowFocus(true);
  setSelectionMode(QListView::Extended);
  setShowSortIndicator(true);
  setItemMargin(3);

  setSorting(1);
}

void FileList::appendImpl(QString item)
{
  QFileInfo *file = new QFileInfo(QDir::cleanDirPath(item));
  if(file->exists()) {
    if(file->isDir()) {
      QDir dir(file->filePath());
      QStringList dirContents=dir.entryList();
      for(QStringList::Iterator it = dirContents.begin(); it != dirContents.end(); ++it) {
        if(*it != "." && *it != "..") {
          appendImpl(file->filePath() + QDir::separator() + *it);
        }
      }
    }
    else {
      // QFileInfo::extension() doesn't always work, so I'm getting old-school on this. -- fixed in Qt 3
      // QString extension = file->filePath().right(file->filePath().length() - (file->filePath().findRev(".") + 1));
      QString extension = file->extension(false);
      if(extensions.contains(extension) > 0 && members.contains(file->absFilePath()) == 0) {
        members.append(file->absFilePath());
	(void) new FileListItem(file, this);
      }
    }
  }
}
