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

#ifndef _TRACKSEQUENCEITERATOR_H
#define _TRACKSEQUENCEITERATOR_H

#include "playlistitem.h"
#include "playlistsearch.h"

class Playlist;

/**
 * This abstract class defines an interface to be used by TrackSequenceManager,
 * to iterate over the items in a playlist.  Implement this class in a subclass
 * in order to define your own ordering for playlist sequences.  For an example,
 * see the UpcomingPlaylist class.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 * @see UpcomingPlaylist
 * @see TrackSequenceManager
 */
class TrackSequenceIterator
{
public:
    /**
     * Default constructor.
     */
    TrackSequenceIterator();

    /**
     * Default copy constructor.
     *
     * @param other the TrackSequenceIterator we are copying
     */
    TrackSequenceIterator(const TrackSequenceIterator & other);
    
    /**
     * Default destructor.
     */
    virtual ~TrackSequenceIterator();

    /**
     * This function moves the current item to the next track.  You must
     * reimplement this function in your subclasses.
     */
    virtual void advance() = 0;

    /**
     * This function moves the current item to the previous track.  This may
     * not always make sense, and the history functionality of the Playlist
     * class currently overrides this.  You must reimplement this function in
     * your subclass.
     */
    virtual void backup() = 0;

    /**
     * This function returns the current PlaylistItem, or 0 if the iterator is
     * not pointing at any PlaylistItem.
     *
     * @return current track
     */
    virtual PlaylistItem *current() const { return m_current; }

    /**
     * This function creates a perfect copy of the object it is called on, to
     * avoid the C++ slicing problem.  When you reimplement this function, you
     * should change the return type to the name of the subclass.
     *
     * @return pointer to a copy of the object
     */
    virtual TrackSequenceIterator *clone() const = 0;

    /**
     * This function is called by the TrackSequenceManager when current() returns
     * 0, if the TrackSequenceManager has a playlist defined.  This function
     * should choose an appropriate starting track and set it as the current
     * item.  This function must be reimplemented in subclasses.
     *
     * @param playlist the playlist to iterate over
     */
    virtual void prepareToPlay(Playlist *playlist) = 0;

    /**
     * This function is called whenever the current playlist changes, such as
     * having a new search applied, items added/removed, etc.  If you need to
     * update internal state, you should do so without affecting the current
     * playing item. Default implementation does nothing.
     */
    virtual void playlistChanged();

    /**
     * This function is called by the manager when \p item is about to be
     * removed.  Subclasses should ensure that they're not still holding a
     * pointer to the item.  The default implementation does nothing.
     *
     * @param item the item about to be removed.
     */
    virtual void itemAboutToDie(const PlaylistItem *item);

    /**
     * This function is called by the TrackSequenceManager is some situations,
     * such as when playback is being stopped.  If you subclass needs to reset
     * any internal data members, do so in this function.  This function must
     * be reimplemented in subclasses.
     */
    virtual void reset() = 0;

    /**
     * This function is a public mutator to set the current item.
     *
     * @param current the new current item
     */
    virtual void setCurrent(PlaylistItem *current);

private:
    PlaylistItem::Pointer m_current; ///< the current item
};

/**
 * This is the default iterator for JuK, supporting normal, random, and album
 * random playback with or without looping.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class DefaultSequenceIterator : public TrackSequenceIterator
{
public:
    /**
     * Default constructor.
     */
    DefaultSequenceIterator();

    /**
     * Default copy constructor.
     *
     * @param other the DefaultSequenceIterator to copy.
     */
    DefaultSequenceIterator(const DefaultSequenceIterator &other);

    /**
     * Default destructor.
     */
    virtual ~DefaultSequenceIterator();

    /**
     * This function advances to the next item in the current sequence.  The
     * algorithm used depends on what playback mode is selected.
     */
    virtual void advance();

    /**
     * This function moves to the previous item in the playlist.  This occurs
     * no matter what playback mode is selected.
     */
    virtual void backup();

    /**
     * This function prepares the class for iterator.  If no random play mode
     * is selected, the first item in the given playlist is the starting item.
     * Otherwise, an item is randomly picked to be the starting item.
     *
     * @param playlist The playlist to initialize for.
     */
    virtual void prepareToPlay(Playlist *playlist);

    /**
     * This function clears all internal state, including any random play lists,
     * and what the current album is.
     */
    virtual void reset();

    /**
     * This function recalculates the random lists, and is should be called
     * whenever its current playlist changes (at least for searches).
     */
    virtual void playlistChanged();

    /**
     * Called when \p item is about to be removed.  This function ensures that
     * it isn't remaining in the random play list.
     */
    virtual void itemAboutToDie(const PlaylistItem *item);

    /**
     * This function sets the current item, and initializes any internal lists
     * that may be needed for playback.
     *
     * @param current The new current item.
     */
    virtual void setCurrent(PlaylistItem *current);

    /**
     * This function returns a perfect copy of the object it is called on, to
     * get around the C++ slicing problem.
     *
     * @return A copy of the object the method is called on.
     */
    virtual DefaultSequenceIterator *clone() const;

private:

    /**
     * Reinitializes the internal random play list based on the playlist given
     * by \p p.  The currently playing item, if any, is automatically removed
     * from the list.
     *
     * @param p The Playlist to read items from.  If p is 0, the playlist of
     *        the currently playing item is used instead.
     */
    void refillRandomList(Playlist *p = 0);
    void initAlbumSearch(PlaylistItem *searchItem);

private:
    PlaylistItemList m_randomItems;
    PlaylistSearch m_albumSearch;
};

#endif /* _TRACKSEQUENCEITERATOR_H */

// vim: set et sw=4:
