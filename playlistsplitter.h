/***************************************************************************
                          playlistsplitter.h  -  description
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
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

class Playlist;
class PlaylistBoxItem;
class CollectionList;
class TagEditor;

/**
 * This is the main layout class of JuK.  It should contain a PlaylistBox and
 * a QWidgetStack of the Playlists.  This like CollectionList, has been 
 * implemented as a pseudo-singleton to provide global access without passing
 * pointers all over the place. */

class PlaylistSplitter : public QSplitter
{
    Q_OBJECT

public:
    /**
     * Since this class is something of a singleton, all instances should be
     * refered to by the instance() method.  Also see initialize().
     */
    static PlaylistSplitter *instance();
    /**
     * This method must be called to construct the object.  It creates the 
     * instance of the object that can be referred to by instance().
     */
    static void initialize(QWidget *parent = 0);

    /**
     * Returns a unique string to be used as new playlist names.  This follows
     * the format "[startingWith] i" where "i" is the first integer greater than 0
     * that does not currently exist in the PlaylistBox.
     */
    QString uniquePlaylistName(const QString &startingWith, bool useParentheses = false);
    /* This calls the above method with startingWith == i18n("Playlist") to 
     * produce "Playlist 1", "Playlist 2", ...
     */
    QString uniquePlaylistName();
    /**
     * Returns a QPtrList of the selected PlaylistItems in the top playlist in 
     * the QWidgetStack of playlists.
     */
    PlaylistItemList playlistSelection() const;
    /**
     * This returns a pointer to the first item in the playlist on the top
     * of the QWidgetStack of playlists.
     */
    PlaylistItem *playlistFirstItem() const;

    // static (non-initialization) methods

    /** 
     * Merges a list of file extensions, and a description of those types into a
     * format that makes sense to KFileDialog.  If type = QString::null then no
     * description is appended.
     */
    static QString extensionsString(const QStringList &extensions, const QString &type = QString::null);
    /**
     * Returns a lif of the extensions that are used for playlists.
     */
    static QStringList playlistExtensions();

public slots:
    void open();
    void openDirectory();
    void open(const QStringList &files);
    void open(const QString &file);
    void save();
    /**
     * Deletes the selected items from the hard disk. 
     */
    void remove();
    void refresh();
    /**
     * Removes the selected items from the playlist. 
     */
    void clearSelectedItems();
    void selectAll(bool select = true);

    void setEditorVisible(bool visible);

    Playlist *createPlaylist();
    Playlist *createPlaylist(const QString &name);
    void openPlaylist();
    Playlist *openPlaylist(const QString &playlistFile);

    void add(const QString &file, Playlist *list);
    void add(const QStringList &files, Playlist *list);

    // PlaylistBox forwarding methods
    void savePlaylist() { playlistBox->save(); }
    void saveAsPlaylist() { playlistBox->saveAs(); }
    void renamePlaylist() { playlistBox->rename(); }
    void duplicatePlaylist() { playlistBox->duplicate(); }
    void deleteItemPlaylist() { playlistBox->deleteItem(); }

protected:
    PlaylistSplitter(QWidget *parent = 0);
    virtual ~PlaylistSplitter();

private:
    void setupLayout();
    void readConfig();
    void saveConfig();
    void addImpl(const QString &file, Playlist *list);

    static PlaylistSplitter *splitter;

    PlaylistBox *playlistBox;
    QWidgetStack *playlistStack;
    TagEditor *editor;

    CollectionList *collection;

    QStringList mediaExtensions;
    QStringList listExtensions;

    bool showEditor;

private slots:
    // playlist box slots
    void changePlaylist(PlaylistBoxItem *item);
    void playlistBoxDoubleClicked(PlaylistBoxItem *item);

signals:
    void playlistDoubleClicked(QListViewItem *);
    void playlistChanged(Playlist *);
};

#endif
