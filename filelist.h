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
    virtual ~FileList();

    void append(const QString &item);
    void append(const QStringList &items);
    void append(FileListItem *item);
    void append(QPtrList<FileListItem> &items);

    void remove(QPtrList<FileListItem> &items);

    QPtrList<FileListItem> selectedItems();

    // yay for implicit sharing!

    QStringList getArtistList() const;
    QStringList getAlbumList() const;

private:
    void setup();
    void appendImpl(const QString &item);

    QStringList extensions;
    QStringList members;
    QStringList artistList;
    QStringList albumList;
    void processEvents();
    int processed;

signals:
    void dataChanged();
};

#endif
