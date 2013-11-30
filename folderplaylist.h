/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FOLDERPLAYLIST_H
#define FOLDERPLAYLIST_H

#include "playlist.h"

class FolderPlaylist : public Playlist
{
    Q_OBJECT

public:
    explicit FolderPlaylist(PlaylistCollection *collection, const QString &folder = QString(),
                   const QString &name = QString());
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

// vim: set et sw=4 tw=0 sta:
