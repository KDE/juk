/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FOLDERPLAYLIST_H
#define FOLDERPLAYLIST_H

#include "playlist.h"

class FolderPlaylist : public Playlist
{
    Q_OBJECT

public:
    FolderPlaylist(PlaylistCollection *collection, const QString &folder = QString::null,
                   const QString &name = QString::null);
    virtual ~FolderPlaylist();

    QString folder() const;
    void setFolder(const QString &s);

    virtual bool canReload() const { return true; }

public slots:
    virtual void slotReload();

private:
    QString m_folder;
};

QDataStream &operator<<(QDataStream &s, const FolderPlaylist &p);
QDataStream &operator>>(QDataStream &s, FolderPlaylist &p);

#endif
