/***************************************************************************
                          playlistsplitter.h  -  description
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#ifndef PLAYLISTSPLITTER_H
#define PLAYLISTSPLITTER_H

#include <kfiledialog.h>
#include <klocale.h>

#include <qsplitter.h>
#include <qwidgetstack.h>

#include "playlistbox.h"
#include "collectionlist.h"
#include "stringhash.h"
#include "tageditor.h"

class KActionMenu;
class KDirWatch;
class PlaylistItem;
class SearchWidget;

/**
 * This is the main layout class of JuK.  It should contain a PlaylistBox and
 * a QWidgetStack of the Playlists.
 *
 * This class serves as a "mediator" (see "Design Patterns") between the JuK
 * class and the playlist classes.  Thus all access to the playlist classes from
 * non-Playlist related classes should be through the public API of this class.
 */

class PlaylistSplitter : public QSplitter
{
    Q_OBJECT

public:
    PlaylistSplitter(QWidget *parent, const char *name = 0);

    virtual ~PlaylistSplitter();

    /**
     * Returns a unique string to be used as new playlist names.  This follows
     * the format "[startingWith] i" where "i" is the first integer greater than
     * 0 that does not currently exist in the PlaylistBox.
     */
    QString uniquePlaylistName(const QString &startingWith, bool useParentheses = false);

    /* This calls the above method with startingWith == i18n("Playlist") to
     * produce "Playlist 1", "Playlist 2", ...
     */
    QString uniquePlaylistName() { return uniquePlaylistName(i18n("Playlist")); }

    ////////////////////////////////////////////////////////////////////////////
    // Variations on the theme "play stuff"
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Returns the file name of the next item to be played and advances the next
     * file.
     */
    QString playNextFile(bool random = false, bool loopPlaylist = false);

    /**
     * Returns the file name of the previous item and moves the playing indicator
     * to the previous file.
     */
    QString playPreviousFile(bool random = false);

    /**
     * Fills the menu passed in with the recently played history
     */
    void populatePlayHistoryMenu(QPopupMenu* menu, bool random);

    /**
     * Returns the name of the currently selected file and moves the playing
     * indicator to that file.
     */
    QString playSelectedFile();

    /**
     * Returns the name of the first item in the playlist and moves the playing
     * indicator to that file.
     */
    QString playFirstFile();

    /**
     * Plays a random file in the currently visible playlist and returns it's
     * name.
     */
    QString playRandomFile();

    /**
     * Since the player is handled at a higher level, this just clears the
     * pointer to the currently playing item and updates the icon.
     */
    void stop();

    ////////////////////////////////////////////////////////////////////////////

    QString playingArtist() const;
    QString playingTrack() const;
    QString playingList() const;

    /**
     * Returns the name of the currently visible playlist.
     */
    QString visiblePlaylistName() const { return visiblePlaylist()->name(); }

    /**
     * Returns the number of items in the currently selected playlist.
     */
    int selectedPlaylistCount() const { return visiblePlaylist()->childCount(); }

    /**
     * Returns true if the the collection list is the visible playlist.
     */
    bool collectionListSelected() const { return visiblePlaylist() == m_collection; }

    bool hasListSelected() const { return m_playlistBox->hasSelection(); }

    /**
     * Open each of \a files, where \a files is a list of playlists and music
     * files.
     */
    void open(const QStringList &files);

    /**
     * Open \a file where \a is a playlist or music file.
     */
    void open(const QString &file);

    QStringList columnNames() const { return m_columnNames; }

    KActionMenu *columnVisibleAction() const { return m_collection->columnVisibleAction(); }

    /**
     * Create a playlist with the named \a name.
     */
    Playlist *createPlaylist(const QString &name);

public slots:

// File slots

    /**
     * Open files or playlists.
     */
    void slotOpen();

    /**
     * Open a directory recursively, grabbing all of the music and playlist files
     * in it's heirarchy.
     */
    void slotOpenDirectory();

// Tagger slots

    void slotSaveTag() { m_editor->save(); }
    void slotGuessTagInfoFile();
    void slotGuessTagInfoInternet();
    void slotRenameFile();

// Playlist slots

    /**
     * Create a playlist and prompt the user for a name.
     */
    Playlist *slotCreatePlaylist();

    /**
     * Create a playlist from a user-defined directory, and prompt the user for
     * a name.
     */
    Playlist *slotCreatePlaylistFromDir();

    /**
     * Sets the selection to the currently playing item and ensures that it is
     * visible.
     */
    void slotSelectPlaying();

// Other slots

    /**
     * Deletes the selected items from the hard disk.
     */
    void slotDeleteSelectedItems();

    /**
     * Refresh either the selection, or if there is no selection, the contents 
     * of the currently visible playlist.  This will cause all of the audio
     * meta data to be reread from disk.
     *
     * \note Within this context "refresh" indicates reloading meta data and
     * "reloading" typically denotes reloading the playlist.
     *
     * \see slotReloadPlaylist()
     */
    void slotRefresh() { visiblePlaylist()->refresh(); }

    /**
     * If the current playlist is one that was imported from a .m3u file, this
     * will revert to the contents of that file.
     * 
     * \see slotRefresh()
     */
    void slotReloadPlaylist() { visiblePlaylist()->slotReload(); }

    /**
     * Show or hide the editor.
     */
    void slotSetEditorVisible(bool visible) { m_editor->setShown(visible); }

    void slotSetSearchVisible(bool visible);

    /**
     * Add the file to the playlist.
     */
    void slotAddToPlaylist(const QString &file, Playlist *list);

    /**
     * Adds the files to the playlist.
     */
    void slotAddToPlaylist(const QStringList &files, Playlist *list);

// PlaylistBox forwarding slots

    void slotSavePlaylist() { m_playlistBox->save(); }
    void slotSaveAsPlaylist() { m_playlistBox->saveAs(); }
    void slotRenamePlaylist() { m_playlistBox->rename(); }
    void slotDuplicatePlaylist() { m_playlistBox->duplicate(); }
    void slotDeletePlaylist();
signals:
    void signalActivated();
    void signalListBoxDoubleClicked();
    void signalPlaylistChanged();
    void signalSelectedPlaylistCountChanged(int);

private:
    /**
     * Returns a PlaylistItemList of the selected PlaylistItems in the top playlist in
     * the QWidgetStack of playlists.
     */
    PlaylistItemList playlistSelection() const { return visiblePlaylist()->selectedItems(); }

    /**
     * This returns a pointer to the first item in the playlist on the top
     * of the QWidgetStack of playlists.
     */
    PlaylistItem *playlistFirstItem() const { return static_cast<PlaylistItem *>(visiblePlaylist()->firstChild()); }

    Playlist *visiblePlaylist() const { return static_cast<Playlist *>(m_playlistStack->visibleWidget()); }

    void setupLayout();
    void readConfig();
    void saveConfig();
    void addImpl(const QString &file, Playlist *list);
    /**
     * If NULL is passed for the icon, no entry is created in the PlaylistBox
     */
    void setupPlaylist(Playlist *p, bool raise = false, const char *icon = "midi", bool sortedFirst = false);

    /**
     * Open the playlist (m3u file or simiar) at \a file.
     */
    Playlist *openPlaylist(const QString &file);

    void setupColumns(Playlist *p);

    /**
     * A convenience function that sets the playing icon, sets the playing item
     * and then returns the name of the file.
     */
    QString play(PlaylistItem *item);

    /**
     * This should be called to update the shown items -- it does not restart the
     * search.
     */
    void redisplaySearch();

private slots:
    void slotChangePlaylist(const PlaylistList &l);
    void slotPlaylistCountChanged(Playlist *p);
    /**
     * Add a directory to the directory list queue.  We need to queue these
     * rather than processing them when they become available because the user
     * could cancel the action.
     */
    void slotQueueDirectory(const QString &directory) { m_directoryQueue.append(directory); }

    /**
     * Add a directory to the queue
     */
    void slotQueueDirectoryRemove(const QString &directory) { m_directoryQueueRemove.append(directory); }

    /**
     * This should be connected to Playlist::aboutToRemove()
     */
    void slotPlaylistItemRemoved(PlaylistItem *item);

    void slotScanDirectories() { open(m_directoryList); }
    void slotSetNextItem(PlaylistItem *item = 0) { m_nextPlaylistItem = item; }
    void slotDirChanged(const QString &dir) { slotAddToPlaylist(dir, m_collection); }
    void slotCreatePlaylist(const PlaylistItemList &items);
    void slotShowSearchResults();
    void slotVisibleColumnsChanged();
    void slotCreateSearchList(const PlaylistSearch &search, const QString &searchCategory,
			      const QString &name);

private:
    PlaylistItem *m_playingItem;
    PlaylistBox *m_playlistBox;
    SearchWidget *m_searchWidget;
    QWidgetStack *m_playlistStack;
    TagEditor *m_editor;

    CollectionList *m_collection;
    Playlist *m_dynamicList;

    StringHash m_playlistFiles;

    QStringList m_directoryList;
    QStringList m_directoryQueue;
    QStringList m_directoryQueueRemove;

    QStringList m_columnNames;

    bool m_restore;

    PlaylistItem *m_nextPlaylistItem;

    KDirWatch *m_dirWatch;
};

#endif
