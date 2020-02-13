/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef SEARCHPLAYLIST_H
#define SEARCHPLAYLIST_H

#include "dynamicplaylist.h"

class SearchPlaylist : public DynamicPlaylist
{
    Q_OBJECT
public:
    explicit SearchPlaylist(PlaylistCollection *collection,
                   const PlaylistSearch& search = PlaylistSearch(),
                   const QString &name = QString(),
                   bool setupPlaylist = true,
                   bool synchronizePlaying = false);

    const PlaylistSearch* playlistSearch() const { return m_search; }
    void setPlaylistSearch ( const PlaylistSearch* s, bool update = true );
    virtual bool searchIsEditable() const override { return true; }

protected:
    /**
     * Runs the search to update the current items.
     */
    virtual void updateItems() override;

private:
    const PlaylistSearch* m_search;
};

QDataStream &operator<<(QDataStream &s, const SearchPlaylist &p);
QDataStream &operator>>(QDataStream &s, SearchPlaylist &p);

#endif

// vim: set et sw=4 tw=0 sta:
