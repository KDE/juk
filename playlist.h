/***************************************************************************
                          playlist.h  -  description
                             -------------------
    begin                : Sat Feb 16 2002
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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <klistview.h>

#include <qstringlist.h>

#include "playlistitem.h"

class PlaylistSplitter;

class Playlist : public KListView
{
    Q_OBJECT

public:
    /** 
     * Before creating a playlist directly, please see 
     * PlaylistSplitter::createPlaylist().
     */
    Playlist(PlaylistSplitter *s, QWidget *parent, const QString &name = QString::null);
    /** 
     * Before creating a playlist directly, please see 
     * PlaylistSplitter::openPlaylist().
     */
    Playlist(PlaylistSplitter *s, const QFileInfo &playlistFile, QWidget *parent, const char *name = 0);
    virtual ~Playlist();

    /**
     * Saves the file to the currently set file name.  If there is no filename
     * currently set, the default behavior is to prompt the user for a file
     * name.  However, by setting autoGenerateFileName, you can tell save to 
     * pick a file name.
     */
    virtual void save(bool autoGenerateFileName = false);
    virtual void saveAs();
    virtual void refresh();
    virtual void clearItems(const PlaylistItemList &items);

    /** 
     * All of the (media) files in the list. 
     */
    QStringList files() const;
    /** 
     * All of the items in the list.
     */
    PlaylistItemList items() const;
    PlaylistItemList selectedItems() const;

    void remove();
    void remove(const PlaylistItemList &items);

    /** 
     * Allow duplicate files in the playlist. 
     */
    void setAllowDuplicates(bool allow);

    /** 
     * This is being used as a mini-factory of sorts to make the construction
     * of PlaylistItems virtual.  In this case it allows for the creation of
     * both PlaylistItems and CollectionListItems.
     */
    virtual PlaylistItem *createItem(const QFileInfo &file, QListViewItem *after = 0);

    /**
     * Internal files are files which have not been saved by the user, but rather
     * are stored in JuK's data directory and are restored by session management.
     */
    bool isInternalFile() const;
    void setInternal(bool internal);
    QString fileName() const;
    void setFileName(const QString &n);
    QString name() const;
    void setName(const QString &n);

    // static methods

    /** 
     * This gets the next item to be played in the specified playlist.
     */
    static PlaylistItem *nextItem(PlaylistItem *current, bool random = false);

protected:
    virtual QDragObject *dragObject();
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    PlaylistSplitter *playlistSplitter() const;

private:
    void setup();

    QStringList members;
    int processed;
    bool allowDuplicates;

    /**
     * If a file is "internal" it is not one that the user has yet chosen to 
     * save.  However for the purposes of being able to restore a user's 
     * loaded playlists it will be saved "internally" in:
     * $KDEHOME/share/apps/juk/playlists.
     */
    bool internalFile;
    QString playlistFileName;
    QString playlistName;
    PlaylistSplitter *splitter;

private slots:
    void emitSelected();

signals:
    /** 
     * This signal is connected to PlaylistItem::refreshed() in the 
     * PlaylistItem class. 
     */
    void dataChanged();
    /** 
     * This signal is emitted when items are added to the collection list.  
     * This happens in the createItem() method when items are added to the 
     * collection. 
     */
    void collectionChanged();

    /** 
     * This is emitted when the playlist selection is changed.  This is used
     * primarily to notify the TagEditor of the new data. 
     */
    void selectionChanged(const PlaylistItemList &selection);
    void nameChanged(const QString &fileName);
};

#endif
