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

#include "playlistitem.h"
#include "playlistbox.h"
#include "collectionlist.h"
#include "tageditor.h"
#include "playlist.h"
#include "stringhash.h"

class PlaylistBoxItem;

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
    PlaylistSplitter(QWidget *parent, bool restoreOnLoad = true, const char *name = 0);

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
    QString uniquePlaylistName() { return(uniquePlaylistName(i18n("Playlist"))); }
    
    /**
     * Returns the file name of the next item to be played and advances the next
     * file.
     */
    QString playNextFile(bool random = false);
    
    /**
     * Returns the file name of the previous item and moves the playing indicator
     * to the previous file.
     */
    QString playPreviousFile(bool random = false);

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
     * Returns a list of the extensions that are used for playlists.
     */
    QStringList playlistExtensions() const { return(listExtensions); }

    /**
     * Returns the name of the currently selected playlist.
     */
    QString selectedPlaylistName() const { return(visiblePlaylist()->name()); }

    /**
     * Returns the number of items in the currently selected playlist.
     */
    int selectedPlaylistCount() const { return(visiblePlaylist()->childCount()); }

    /**
     * Add the file to the playlist.
     */
    void add(const QString &file, Playlist *list);
    
    /**
     * Adds the files to the playlist.
     */
    void add(const QStringList &files, Playlist *list);

    /**
     * Returns true if the the collection list is the visible playlist.
     */
    bool collectionListSelected() const { return(visiblePlaylist() == CollectionList::instance()); }

// static methods

    /** 
     * Merges a list of file extensions, and a description of those types into a
     * format that makes sense to KFileDialog.  If type = QString::null then no
     * description is appended.
     */
    static QString extensionsString(const QStringList &extensions, const QString &type = QString::null);

public slots:

// File slots

    /**
     * Open files or playlists.
     */
    void open();

    /**
     * Open a directory recursively, grabbing all of the music and playlist files
     * in it's heirarchy.
     */
    void openDirectory();

    /**
     * Open each of \a files, where \a files is a list of playlists and music
     * files.
     */
    void open(const QStringList &files) { add(files, visiblePlaylist()); }

    /**
     * Open \a file where \a is a playlist or music file.
     */
    void open(const QString &file) { add(file, visiblePlaylist()); }

    /**
     * Save.
     */
    void saveItem() { editor->save(); }

// Edit slots
    
    void copy() {}
    void paste() {}
    void clear() { clearSelectedItems(); }

// Playlist slots

    /**
     * Create a playlist and prompt the user for a name.
     */
    Playlist *createPlaylist();

    /**
     * Create a playlist with the named \a name.
     */
    Playlist *createPlaylist(const QString &name);

    /**
     * Prompt the user for a playlist to open.
     */
    void openPlaylist();

    /**
     * Open the playlist (m3u file or simiar) at \a playlistFile.
     */
    Playlist *openPlaylist(const QString &playlistFile);

    /**
     * Sets the selection to the currently playing item and ensures that it is
     * visible.
     */
    void selectPlaying();

    QString playingArtist() const;
    QString playingTrack() const;
    QString playingList() const;

// Other slots
    
    /**
     * Since the player is handled at a higher level, this just clears the 
     * pointer to the currently playing item and updates the icon.
     */
    void stop();

    /**
     * Deletes the selected items from the hard disk. 
     */
    void removeSelectedItems();
    
    /**
     * Refresh the contents of the currently visible playlist.  This will cause
     * all of the audio meta data to be reread from disk.
     */
    void refresh() { visiblePlaylist()->refresh(); }

    /**
     * Removes the selected items from the playlist. 
     */
    void clearSelectedItems();

    /**
     * Select (or deselect) all of the items in the currently visible playlist.
     */
    void selectAll(bool select = true) { visiblePlaylist()->selectAll(select); }

    /**
     * Show or hide the editor.
     */
    void setEditorVisible(bool visible);

    /**
     * Add a directory to the directory list queue.  We need to queue these 
     * rather than processing them when they become available because the user
     * could cancel the action.
     */
    void queueDirectory(const QString &directory) { directoryQueue.append(directory); }

    /**
     * Add a directory to the queue 
     */
    void queueDirectoryRemove(const QString &directory) { directoryQueueRemove.append(directory); }

// PlaylistBox forwarding slots

    void savePlaylist() { playlistBox->save(); }
    void saveAsPlaylist() { playlistBox->saveAs(); }
    void renamePlaylist() { playlistBox->rename(); }
    void duplicatePlaylist() { playlistBox->duplicate(); }
    void deleteItemPlaylist() { playlistBox->deleteItem(); }

signals:
    void doubleClicked();
    void listBoxDoubleClicked();
    void playlistChanged();
    void selectedPlaylistCountChanged(int);

private:
    /**
     * Returns a QPtrList of the selected PlaylistItems in the top playlist in 
     * the QWidgetStack of playlists.
     */
    PlaylistItemList playlistSelection() const { return(visiblePlaylist()->selectedItems()); }
    /**
     * This returns a pointer to the first item in the playlist on the top
     * of the QWidgetStack of playlists.
     */
    PlaylistItem *playlistFirstItem() const { return(static_cast<PlaylistItem *>(visiblePlaylist()->firstChild())); }

    Playlist *visiblePlaylist() const { return(static_cast<Playlist *>(playlistStack->visibleWidget())); }

    void setupLayout();
    void readConfig();
    void saveConfig();
    void addImpl(const QString &file, Playlist *list);
    void setupPlaylist(Playlist *p, bool raise = false, const char *icon = "midi");
    void checkPlayingItemBeforeRemove(PlaylistItemList &items);
    
private slots:
    void changePlaylist(PlaylistBoxItem *item);
    void playlistCountChanged(Playlist *p);

private:
    PlaylistItem *playingItem;
    PlaylistBox *playlistBox;
    QWidgetStack *playlistStack;
    TagEditor *editor;

    CollectionList *collection;

    StringHash playlistFiles;

    QStringList mediaExtensions;
    QStringList listExtensions;

    QStringList directoryList;
    QStringList directoryQueue;
    QStringList directoryQueueRemove;

    bool showEditor;
    bool restore;
};

#endif
