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

#include "folderplaylist.h"
#include "playlistcollection.h"
#include "juk-exception.h"

#include <QTimer>

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FolderPlaylist::FolderPlaylist(PlaylistCollection *collection, const QString &folder,
                               const QString &name) :
    Playlist(collection, name, "folder"),
    m_folder(folder)
{
    setAllowDuplicates(false);
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
    if(!m_folder.isEmpty())
        addFiles(QStringList(m_folder));
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

    if(folder.isEmpty() || name.isEmpty())
        throw BICStreamException();

    p.setFolder(folder);
    p.setName(name);
    return s;
}

#include "folderplaylist.moc"

// vim: set et sw=4 tw=0 sta:
