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

class FileList;

class FileListItem : public QObject, public KListViewItem 
{
    Q_OBJECT
public:
    enum ColumnType { TrackColumn = 0, ArtistColumn = 1, AlbumColumn = 2, TrackNumberColumn = 3,
                      GenreColumn = 4, YearColumn = 5, LengthColumn = 6, FileNameColumn = 7 };

    FileListItem(const QFileInfo &file, FileList *parent);
    FileListItem(FileListItem &item, FileList *parent);
    virtual ~FileListItem();

    // these can't be const members because they fetch the data "on demand"

    Tag *getTag();
    AudioData *getAudioData();

    void setFile(const QString &file);

    // QFileInfo-ish methods

    QString fileName() const;
    QString filePath() const;
    QString absFilePath() const;
    QString dirPath(bool absPath = false) const;
    bool isWritable() const;

public slots:
    void refresh();

protected slots:
    void addSibling(const FileListItem *sibling);
    void removeSibling(const FileListItem *sibling);

signals:
    void refreshed();

private:
    class Data;

    Data *getData();

    int compare(QListViewItem *item, int column, bool ascending) const;
    int compare(FileListItem *firstItem, FileListItem *secondItem, int column, bool ascending) const;

    Data *data;
};

#endif
