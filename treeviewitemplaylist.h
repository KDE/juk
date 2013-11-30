/**
 * Copyright (C) 2004 Michael Pyne <mpyne@kde.org>
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

#ifndef TREEVIEWITEMPLAYLIST_H
#define TREEVIEWITEMPLAYLIST_H

#include "searchplaylist.h"
#include "playlistitem.h"

class QStringList;

class TreeViewItemPlaylist : public SearchPlaylist
{
    Q_OBJECT

public:
    explicit TreeViewItemPlaylist(PlaylistCollection *collection,
                         const PlaylistSearch &search = PlaylistSearch(),
                         const QString &name = QString());

    virtual bool searchIsEditable() const { return false; }
    void retag(const QStringList &files, Playlist *donorPlaylist);

signals:
    void signalTagsChanged();

private:
    PlaylistItem::ColumnType m_columnType;
};

#endif // TREEVIEWITEMPLAYLIST_H

// vim: set et sw=4 tw=0 sta:
