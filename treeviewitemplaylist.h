/***************************************************************************
    begin                : Mon Jun 21 2004
    copyright            : (C) 2004 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TREEVIEWITEMPLAYLIST_H
#define TREEVIEWITEMPLAYLIST_H

#include "searchplaylist.h"
#include "playlistitem.h"

class QStringList;

class TreeViewItemPlaylist : public SearchPlaylist
{
    Q_OBJECT

public:
    TreeViewItemPlaylist(PlaylistCollection *collection,
                         const PlaylistSearch &search = PlaylistSearch(),
                         const QString &name = QString::null);

    virtual bool searchIsEditable() const { return false; }
    void retag(const QStringList &files, Playlist *donorPlaylist);

signals:
    void signalTagsChanged();

private:
    PlaylistItem::ColumnType m_columnType;
};

#endif // TREEVIEWITEMPLAYLIST_H
