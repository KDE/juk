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

#include "folderplaylist.h"
#include "playlistcollection.h"

#include <qtimer.h>

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FolderPlaylist::FolderPlaylist(PlaylistCollection *collection, const QString &folder,
                               const QString &name) :
    Playlist(collection, name, "folder"),
    m_folder(folder)
{
    QTimer::singleShot(0, this, SLOT(slotReload()));
}

FolderPlaylist::~FolderPlaylist()
{

}

QString FolderPlaylist::folder() const
{
    return m_folder;
}

void FolderPlaylist::setFolder(const QString &s)
{
    m_folder = s;
    QTimer::singleShot(0, this, SLOT(slotReload()));
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void FolderPlaylist::slotReload()
{
    if(!m_folder.isNull())
        addFiles(m_folder);
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const FolderPlaylist &p)
{
    s << p.name()
      << p.folder();
    return s;
}

QDataStream &operator>>(QDataStream &s, FolderPlaylist &p)
{
    QString name;
    QString folder;
    s >> name
      >> folder;

    p.setFolder(folder);
    p.setName(name);
    return s;
}

#include "folderplaylist.moc"
