 /***************************************************************************
                          historyplaylist.cpp
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

#include <klocale.h>

#include "historyplaylist.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// HistoryPlaylistItem public members
////////////////////////////////////////////////////////////////////////////////

HistoryPlaylistItem::HistoryPlaylistItem(CollectionListItem *item, Playlist *parent, QListViewItem *after) :
    PlaylistItem(item, parent, after)
{
    setText(0, QDateTime::currentDateTime().toString(LocalDate));
}

HistoryPlaylistItem::~HistoryPlaylistItem()
{

}

////////////////////////////////////////////////////////////////////////////////
// HistoryPlayList public members
////////////////////////////////////////////////////////////////////////////////

HistoryPlaylist::HistoryPlaylist(QWidget *parent) : Playlist(parent, i18n("History"))
{
    setAllowDuplicates(true);
}

HistoryPlaylist::~HistoryPlaylist()
{

}

void HistoryPlaylist::createItems(const PlaylistItemList &siblings)
{
    Playlist::createItems<CollectionListItem, HistoryPlaylistItem, PlaylistItem>(siblings);
}

////////////////////////////////////////////////////////////////////////////////
// HistoryPlaylist protected members
////////////////////////////////////////////////////////////////////////////////

void HistoryPlaylist::polish()
{
    addColumn(i18n("Time"));
    Playlist::polish();
    setSorting(-1);
}

#include "historyplaylist.moc"
