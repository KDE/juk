/***************************************************************************
                          filelist.h  -  description
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

#ifndef FILELIST_H
#define FILELIST_H

#include <klistview.h>

#include <qstringlist.h>

#include "filelistitem.h"

class FileList : public KListView
{
  Q_OBJECT
public: 
  FileList(QWidget *parent = 0, const char *name = 0);
  FileList(QString &item, QWidget *parent = 0, const char *name = 0);
  FileList(QStringList &items, QWidget *parent = 0, const char *name = 0);
  ~FileList();

  void append(QString item);
  void append(QStringList &items);
  void append(FileListItem *item);
  void append(QPtrList<QListViewItem> &items);

  void remove(QPtrList<QListViewItem> &items);
  
  FileListItem *getSelectedItem();

private:
  void setup();
  void appendImpl(QString item);
  
  QStringList extensions;
  QStringList members;
  void processEvents();
  int processed;

signals:
  void dataChanged();
};

#endif
