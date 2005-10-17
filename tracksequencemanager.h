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

#ifndef _TRACKSEQUENCEMANAGER_H
#define _TRACKSEQUENCEMANAGER_H

#include <qobject.h>

class KPopupMenu;
class TrackSequenceIterator;
class PlaylistItem;
class Playlist;

/**
 * This class is responsible for managing the music play sequence for JuK.
 * Instead of playlists deciding which song goes next, this class is used to
 * do so.  You can replace the iterator used as well, although the class
 * provides a default iterator that supports random play and playlist looping.
 *
 * @see Playlist
 * @see TrackSequenceIterator
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class TrackSequenceManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Destroys the track sequence manager.  The sequence iterators will also
     * be deleted, but the playlist, popup menu, and playlist items will not be
     * touched.
     */
    ~TrackSequenceManager();

    /**
     * This function installs a new iterator to be used instead of the old
     * one.  TrackSequenceManager will control the iterator after that,
     * deleting the iterator when another is installed, or when the
     * TrackSequenceManager is destroyed.
     *
     * @param iterator the iterator to install, or 0 for the default
     * @return true if installation successfully happened
     */
    bool installIterator(TrackSequenceIterator *iterator);

    /**
     * @return currently selected iterator
     */
    TrackSequenceIterator *iterator() const { return m_iterator; }

    /**
     * This function returns a pointer to the currently set iterator, and
     * then removes the TrackSequenceManager's pointer to the iterator without
     * deleting the iterator.  You should only do this if you are going to be
     * using @see installIterator to give control of the iterator back to the
     * TrackSequenceManager at some point.  Also, you must install a 
     * replacement iterator before the TrackSequenceManager is otherwise
     * used.  If you use this function, you must manually set the current
     * item of the iterator you replace the old one with (if you want).
     *
     * @see installIterator
     * @return the currently set iterator.
     */
    TrackSequenceIterator *takeIterator();

    /**
     * Returns the global TrackSequenceManager object.  This is the only way to
     * access the TrackSequenceManager.
     *
     * @return the global TrackSequenceManager
     */
    static TrackSequenceManager *instance();

    /**
     * Returns the next track, and advances in the current sequence..
     *
     * @return the next track in the current sequence, or 0 if the end has
     * been reached
     */
    PlaylistItem *nextItem();

    /**
     * Returns the previous track, and backs up in the current sequence.  Note
     * that if you have an item x, nextItem(previousItem(x)) is not guaranteed
     * to equal x, even ignoring the effect of hitting the end of list.
     *
     * @return the previous track in the current sequence, or 0 if the
     * beginning has been reached
     */
    PlaylistItem *previousItem();

    /**
     * @return the current track in the current sequence, or 0 if there is no
     * current track (for example, an empty playlist)
     */
    PlaylistItem *currentItem() const;

    /**
     * @return the current KPopupMenu used by the manager, or 0 if none is
     * set
     */
    KPopupMenu *popupMenu() const { return m_popupMenu; }

    /**
     * @return the TrackSequenceManager's idea of the current playlist
     */
    Playlist *currentPlaylist() const { return m_playlist; }

public slots:
    /**
     * Set the next item to play to @p item
     *
     * @param item the next item to play
     */
    void setNextItem(PlaylistItem *item);

    /**
     * Sets the current playlist.  This is necessary in order for some of the
     * actions in the popup menu used by this class to work.  Note that the
     * current playlist is not necessarily the same as the playlist that is
     * playlist.  The TrackSequenceManager does not own @p list after this
     * call.
     *
     * @param list the current playlist
     */
    void setCurrentPlaylist(Playlist *list);

    /**
     * Sets the current item to @p item.  You should try to avoid calling this
     * function, instead allowing the manager to perform its work.  However,
     * this function is useful for clearing the current item.  Remember that
     * you must have a valid playlist to iterate if you clear the current item.
     *
     * @param item the PlaylistItem that is currently playing.  Set to 0 if
     * there is no item playing.
     */
    void setCurrent(PlaylistItem *item);

private:
    /**
     * Sets up various connections, to be run after the GUI is running.
     * Automatically run by instance().
     *
     * @see instance
     */
    void initialize();

    /** 
     * Constructs the sequence manager.  The constructor will work even before
     * the GUI has been created.  Note that you can't actually construct an
     * object with this function, use instance().
     *
     * @see instance
     */
    TrackSequenceManager();

protected slots:
    
    /**
     * This slot should be called when @a item is about to be deleted, so that
     * the TrackSequenceManager can make sure that any pointers held pointing
     * to @a item are corrected.
     *
     * @param item The PlaylistItem about to be deleted.
     */
    void slotItemAboutToDie(PlaylistItem *item);

private:
    Playlist *m_playlist;
    PlaylistItem *m_curItem, *m_playNextItem;
    KPopupMenu *m_popupMenu;
    TrackSequenceIterator *m_iterator;
    TrackSequenceIterator *m_defaultIterator;
    bool m_initialized;
};

#endif /* _TRACKSEQUENCEMANAGER_H */

// vim: set et sw=4:
