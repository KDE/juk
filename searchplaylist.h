/***************************************************************************
    begin                : Mon May 5 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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

#ifndef SEARCHPLAYLIST_H
#define SEARCHPLAYLIST_H

#include "dynamicplaylist.h"

class SearchPlaylist : public DynamicPlaylist
{
    Q_OBJECT
public:
    SearchPlaylist(PlaylistCollection *collection,
                   const PlaylistSearch &search = PlaylistSearch(),
                   const QString &name = QString::null,
		   bool setupPlaylist = true,
		   bool synchronizePlaying = false);

    PlaylistSearch playlistSearch() const { return m_search; }
    void setPlaylistSearch(const PlaylistSearch &s, bool update = true);
    virtual bool searchIsEditable() const { return true; }

protected:
    /**
     * Runs the search to update the current items.
     */
    virtual void updateItems();

private:
    PlaylistSearch m_search;
};

QDataStream &operator<<(QDataStream &s, const SearchPlaylist &p);
QDataStream &operator>>(QDataStream &s, SearchPlaylist &p);

#endif
