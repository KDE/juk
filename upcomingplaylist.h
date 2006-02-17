/***************************************************************************
    begin                : Thu Aug 19 2004 
    copyright            : (C) 2002 - 2004 by Michael Pyne
    email                : michael.pyne@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _UPCOMINGPLAYLIST_H
#define _UPCOMINGPLAYLIST_H


#include <qguardedptr.h>

#include "playlist.h"
#include "tracksequenceiterator.h"

class TrackSequenceManager;

/**
 * A class to implement upcoming playlist support in JuK.  It is closely
 * associated with the UpcomingSequenceIterator class.  It works by using
 * whatever iterator is currently being used by the TrackSequenceManager
 * instance when this playlist is constructed to form a list of upcoming tracks.
 * After the playlist is created, tracks are played from top to bottom until
 * the list is empty.  If loop playlist is enabled, tracks are automatically
 * added as tracks are removed.  Also, enabling this playlist causes the base
 * Playlist class to add an item to the context-menu to add the selected items
 * to this playlist.  If the user double-clicks any track to force it to play,
 * it is added to the top of this playlist automatically, replacing any
 * currently playing song.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 * @see UpcomingSequenceIterator
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
     * @param defaultSize The default number of tracks to place in the playlist.
     */
    UpcomingPlaylist(PlaylistCollection *collection, int defaultSize = 15);
    /**
     * Destructor for the UpcomingPlaylist.  This destructor will restore the
     * iterator for the TrackSequenceManager, and if a song is playing when
     * this playlist is removed, it will remain playing after the playlist is
     * destroyed.
     */
    virtual ~UpcomingPlaylist();

    /**
     * This function initializes the upcoming playlist, so that you can create
     * it before the GUI has been completely setup.  If a song is playing when
     * this function is called, then the song will be added to this playlist,
     * automatically with no interruption in playback.
     */
    void initialize();

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
    virtual void playNext();

    /**
     * Reimplemented to remove the item from the Playlist index.
     */
    virtual void clearItem(PlaylistItem *item, bool emitChanged = true);

    virtual void addFiles(const QStringList &files, PlaylistItem *after = 0);

    /**
     * Returns a reference to the index between items in the list and the
     * playlist that they came from.  This is used to remap the currently
     * playing item to the source playlist.
     */
    QMap<PlaylistItem::Pointer, QGuardedPtr<Playlist> > &playlistIndex();

    bool active() const { return m_active; }

private:

    /**
     * Internal function to restore the TrackSequenceManager to the state
     * it was in when the object was constructed, except for the playing
     * item.
     */
    void removeIteratorOverride();

    /**
     * This function returns the instance of the TrackSequenceManager.
     *
     * @return the TrackSequenceManager instance.
     * @see TrackSequenceManager::instance()
     */
    TrackSequenceManager *manager() const;

private:
    class UpcomingSequenceIterator;
    friend class UpcomingSequenceIterator;

    bool m_active;
    TrackSequenceIterator *m_oldIterator;
    int m_defaultSize;
    QMap<PlaylistItem::Pointer, QGuardedPtr<Playlist> > m_playlistIndex;
};

/**
 * An implementation of TrackSequenceIterator designed to work with
 * UpcomingPlaylist.  It is installed by UpcomingPlaylist to ensure that the
 * UpcomingPlaylist is in charge of the playback sequence.
 *
 * @see UpcomingPlaylist
 * @see TrackSequenceManager
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class UpcomingPlaylist::UpcomingSequenceIterator : public TrackSequenceIterator
{
public:
    /**
     * Default constructor.
     *
     * @param playlist The UpcomingPlaylist this iterator belongs to.
     */
    UpcomingSequenceIterator(UpcomingPlaylist *playlist);

    /**
     * Copy constructor.
     *
     * @param other The UpcomingSequenceIterator to copy.
     */
    UpcomingSequenceIterator(const UpcomingSequenceIterator &other);

    /**
     * Destructor.
     */
    virtual ~UpcomingSequenceIterator();

    /**
     * Advances to the next song in the UpcomingPlaylist.
     */
    virtual void advance();

    /**
     * This function does nothing, as the currently playing song in the
     * UpcomingPlaylist is always the first song in the sequence.
     */
    virtual void backup();

    /**
     * This function returns a perfect duplicate of the object it is called
     * on, to avoid the C++ slicing problem.
     *
     * @return A pointer to a copy of the object it is called on.
     */
    virtual UpcomingSequenceIterator *clone() const;

    /**
     * This function sets the currently playing item to @a currentItem.  If the
     * item doesn't belong to the parent UpcomingPlaylist, it will be added to
     * the UpcomingPlaylist, replacing any track that may be playing.
     * Otherwise, it is moved up and set to play, replacing any track that may
     * be playing.
     *
     * @param currentItem The PlaylistItem to play.
     */
    virtual void setCurrent(PlaylistItem *currentItem);

    /**
     * This function resets any internet state.
     */
    virtual void reset();

    /**
     * This function readies the UpcomingSequenceIterator for playback, by
     * making sure the parent UpcomingPlaylist has items to play if it is
     * empty.
     */
    virtual void prepareToPlay(Playlist *);

private:
    UpcomingPlaylist *m_playlist;
};

QDataStream &operator<<(QDataStream &s, const UpcomingPlaylist &p);
QDataStream &operator>>(QDataStream &s, UpcomingPlaylist &p);

#endif /* _UPCOMINGPLAYLIST_H */

// vim: set et sw=4 ts=4:
