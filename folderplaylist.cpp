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

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FolderPlaylist::FolderPlaylist(PlaylistCollection *collection, const QString &folder,
                               const QString &name) :
    Playlist(collection, name),
    m_folder(folder)
{
    addFiles(folder, false);
}

FolderPlaylist::~FolderPlaylist()
{

}

#include "folderplaylist.moc"
