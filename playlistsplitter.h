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
#include "tagguesser.h"

class KActionMenu;
class KDirWatch;
class PlaylistItem;
class SearchWidget;
class HistoryPlaylist;

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

    /**
     * Returns true if the selected list has a file associated with it.
     */
    bool fileBasedListSelected() { return !visiblePlaylist()->fileName().isNull(); }

    /**
     * Returns true if the selected list is read only.
     */
    bool readOnlyListSelected() { return visiblePlaylist()->readOnly(); }

    /**
     * Returns true if the currently selected playlist is a dynamic list.
     */
    bool dynamicListSelected() { return m_dynamicList && visiblePlaylist() == m_dynamicList; }

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
    void slotGuessTagInfo(TagGuesser::Type type);
    void slotRenameFile();

// Playlist slots

    /**
     * Create a playlist and prompt the user for a name if no name was
     * specified.
     */
    Playlist *slotCreatePlaylist(const QString &name = QString::null);

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
    void slotRefresh() { visiblePlaylist()->slotRefresh(); }

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
    void slotSetHistoryVisible(bool visible);
    void slotAdvancedSearch();

    /**
     * Add the file to the playlist.  If \a after is null the items will be
     * inserted at the end of the list.
     */
    void slotAddToPlaylist(const QString &file, Playlist *list, PlaylistItem *after = 0);

    /**
     * Adds the files to the playlist.  If \a after is null the items will be
     * inserted at the end of the list.
     */
    void slotAddToPlaylist(const QStringList &files, Playlist *list, PlaylistItem *after = 0);

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

    enum PlaylistType { Normal = 0, Search = 1, History = 2 };
    static const int playlistCacheVersion = 1;


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

    /**
     * Adds the file or directory \a file to the Playlist \a list.  If \a after
     * is not null the items will be inserted after it.  Returns a pointer to the
     * last item inserted.
     */
    PlaylistItem *addImpl(const QString &file, Playlist *list, PlaylistItem *after = 0);

    /**
     * If null is passed for the icon, no entry is created in the PlaylistBox
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

    void readPlaylists();
    void savePlaylists();

private slots:

    /**
     * This slot is called when the user selects a different playlist or list of
     * playlists.  To make things easy, it is always handled as a list.
     */
    void slotChangePlaylist(const PlaylistList &l);

    /**
     * This slot is called when the total numbers in the playlist splitter has
     * changed.
     */
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

    /**
     * Scans the dirs in the users music directory list and makes sure that they
     * are loaded and current (i.e. the cache is in sync with the file system).
     */
    void slotScanDirectories() { open(m_directoryList); }

    /**
     * Set the next item to be played to item.
     */
    void slotSetNextItem(PlaylistItem *item = 0) { m_nextPlaylistItem = item; }

    /**
     * This slot is called when a change in the contents of one of the dirs in
     * the user's set of music dirs has had items added or removed.
     */
    void slotDirChanged(const QString &dir) { slotAddToPlaylist(dir, m_collection); }

    /**
     * Create a playlist that contains the specified files.
     */
    void slotCreatePlaylist(const QStringList &files);

    /**
     * This slot creates a playlist that contains the listed items.  It is
     * private since we want to encapsulate the PlaylistItem handling.
     */
    void slotCreatePlaylist(const PlaylistItemList &items);

    /**
     * Updates the visible search results based on the result of the search
     * associated with the currently visible playlist.
     */
    void slotShowSearchResults();
    /**
     * This slot is called when the set of visible playlist columns changes.
     */
    void slotVisibleColumnsChanged();

    /**
     * Create a search list based on the specified search.
     */
    void slotCreateSearchList(const PlaylistSearch &search, const QString &searchCategory,
			      const QString &name);

private:
    PlaylistItem *m_playingItem;
    PlaylistBox *m_playlistBox;
    SearchWidget *m_searchWidget;
    QWidgetStack *m_playlistStack;
    TagEditor *m_editor;

    CollectionList *m_collection;
    HistoryPlaylist *m_history;
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
