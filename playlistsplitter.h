/***************************************************************************
                          playlistsplitter.h  -  description
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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

#include <qwidgetstack.h>

#include "playlistbox.h"
#include "collectionlist.h"
#include "tageditor.h"
#include "playlistinterface.h"

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

class PlaylistSplitter : public QSplitter, public PlaylistInterface
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
    QString uniquePlaylistName(const QString &startingWith = i18n("Playlist"),
			       bool useParentheses = false);

    virtual QString name() const;
    virtual FileHandle nextFile();
    virtual FileHandle currentFile();
    virtual FileHandle previousFile();
    virtual int count() const { return visiblePlaylist()->childCount(); }
    virtual int time() const  { return visiblePlaylist()->time(); }

    /**
     * Fills the menu passed in with the recently played history
     */
    void populatePlayHistoryMenu(QPopupMenu *menu, bool random);

    /**
     * Open each of \a files, where \a files is a list of playlists and music
     * files.
     */
    void open(const QStringList &files);

    /**
     * Open \a file where \a is a playlist or music file.
     */
    void open(const QString &file);

    /**
     * Create a playlist with the name \a name.  If \a raise is true then the
     * Playlist will be made the current playlist.
     */
    Playlist *createPlaylist(const QString &name, bool raise = true);

    /**
     * TODO -- REMOVE THIS -- MASSIVE HACK
     *
     * This can be used to turn on or off scanning for new files.  This is
     * presently used so that the file renamer can temporariy disable searching
     * for new files while it performs the rename.
     */
    void setDirWatchEnabled(bool enabled);

public slots:

    /**
     * Open files or playlists.
     */
    void slotOpen();

    /**
     * Open a directory recursively, grabbing all of the music and playlist files
     * in it's heirarchy.
     */
    void slotOpenDirectory();

    void slotSaveTag() { m_editor->save(); }
    void slotGuessTagInfo(TagGuesser::Type type);
    void slotRenameFile();

    /**
     * Create a playlist and prompt the user for a name if no name was
     * specified.
     *
     * If \a raise is true the playlist will be shown after being created.
     */
    Playlist *slotCreatePlaylist(const QString &name = QString::null,
				 bool raise = true);

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
    void slotReloadPlaylist();

    /**
     * If the visible playlist is a SearchPlaylist then popup a dialog to edit
     * the search.
     */
    void slotEditSearch();

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

    void slotSavePlaylist() { m_playlistBox->save(); }
    void slotSaveAsPlaylist() { m_playlistBox->saveAs(); }
    void slotRenamePlaylist() { m_playlistBox->rename(); }
    void slotDuplicatePlaylist() { m_playlistBox->duplicate(); }
    void slotDeletePlaylist();

signals:
    void signalPlaylistChanged();

private:

    enum PlaylistType { Normal = 0, Search = 1, History = 2 };
    static const int playlistCacheVersion = 2;

    /**
     * Returns a PlaylistItemList of the selected PlaylistItems in the top playlist in
     * the QWidgetStack of playlists.
     */
    PlaylistItemList playlistSelection() const { return visiblePlaylist()->selectedItems(); }

    /**
     * This returns a pointer to the first item in the playlist on the top
     * of the QWidgetStack of playlists.
     */
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
    FileHandle play(PlaylistItem *item);

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

    void slotPlayCurrent();

    /**
     * Since the player is handled at a higher level, this just clears the
     * pointer to the currently playing item and updates the icon.
     */
    void stop();

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
    QStringList m_columnNames;

    bool m_restore;
    bool m_importPlaylists;

    PlaylistItem *m_nextPlaylistItem;

    KDirWatch m_dirWatch;
};

#endif
