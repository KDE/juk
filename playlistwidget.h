/***************************************************************************
                           playlistwidget.h  -  description
                             -------------------
    begin                : Tue Feb 5 2002
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

#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <klistview.h>

#include <qptrlist.h>

#include "filelist.h"
#include "filelistitem.h"

class PlaylistWidget : public QWidget
{
public: 
  PlaylistWidget(QWidget *parent);
  ~PlaylistWidget();

  void add(QString item);
  void add(QStringList *items);
  void add(FileListItem *item);
  void add(QPtrList<QListViewItem> *items);

  FileList *getPlaylistList();
  QPtrList<QListViewItem> *getSelectedItems();
  FileListItem *firstItem();
private:
  void setupLayout();

  // main visual objects
  FileList *playlistList;
};

#endif
