/***************************************************************************
                          historyplaylist.h
                             -------------------
    begin                : Fri Aug 8 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#ifndef HISTORYPLAYLIST_H
#define HISTORYPLAYLIST_H

#include "playlist.h"
#include "playlistitem.h"

class HistoryPlaylistItem;

class HistoryPlaylist : public Playlist
{
public:
    HistoryPlaylist(QWidget *parent);
    virtual ~HistoryPlaylist();

    virtual void createItems(const PlaylistItemList &siblings);
};


class HistoryPlaylistItem : public PlaylistItem
{
public:
    HistoryPlaylistItem(CollectionListItem *item, Playlist *parent, QListViewItem *after);
    virtual ~HistoryPlaylistItem();
};

#endif
