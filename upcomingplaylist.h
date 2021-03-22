/**
 * Copyright (C) 2002-2004 Michael Pyne <mpyne@kde.org>
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

#ifndef UPCOMINGPLAYLIST_H
#define UPCOMINGPLAYLIST_H

#include <QPointer>
#include <QMap>

#include "playlist.h"
#include "playlistitem.h"

/**
 * A class to implement upcoming playlist support in JuK for the "Play Queue".
 * After the playlist is created, tracks are played from top to bottom until
 * the playlist is empty.  As long as this playlist is alive it will be the
 * "playing playlist" under PlaylistBox.
 *
 * Also, enabling this playlist causes the base Playlist class to add an item
 * to the context-menu to add the selected items to this playlist.  If the user
 * double-clicks any track to force it to play, it is added to the top of this
 * playlist automatically, replacing any currently playing song.
 *
 * @author Michael Pyne <mpyne@kde.org>
 */
class UpcomingPlaylist : public Playlist
{
public:
    /**
     * Constructor for the UpcomingPlaylist object.  You should only ever have
     * one instance of this class.  You should call the initialize() function
     * before using the created object.
     *
     * @see initialize
     * @param collection The PlaylistCollection that owns this playlist.
     */
    explicit UpcomingPlaylist(PlaylistCollection *collection);

    /**
     * Appends the given items to the end of the playlist.  Use this function
     * instead of createItems() since this function ensures that the items are
     * added to the end of the playlist.
     *
     * @see createItems(const PlaylistItemList &, PlaylistItem *)
     * @param itemList The list of PlaylistItems to append.
     */
    void appendItems(const PlaylistItemList &itemList);

    /**
     * Reimplemented to set the playing item in both the source playlist
     * and the upcoming playlist.
     */
    virtual void playNext() override;

    /**
     * Reimplemented to remove the item from the Playlist index.
     */
    virtual void clearItem(PlaylistItem *item) override;

    virtual void addFiles(const QStringList &files, PlaylistItem *after = nullptr) override;

    /**
     * Returns a reference to the index between items in the list and the
     * playlist that they came from.  This is used to remap the currently
     * playing item to the source playlist.
     */
    QMap<PlaylistItem::Pointer, QPointer<Playlist> > &playlistIndex();

private:
    QMap<PlaylistItem::Pointer, QPointer<Playlist> > m_playlistIndex;
};

QDataStream &operator<<(QDataStream &s, const UpcomingPlaylist &p);
QDataStream &operator>>(QDataStream &s, UpcomingPlaylist &p);

#endif /* UPCOMINGPLAYLIST_H */

// vim: set et sw=4 tw=0 sta:
