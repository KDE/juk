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

class Playlist : public KListView
{
    Q_OBJECT

public:
    Playlist(QWidget *parent = 0, const char *name = 0);
    Playlist(const QFileInfo &playlistFile, QWidget *parent = 0, const char *name = 0);
    virtual ~Playlist();

    virtual void save();
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
     * of PlaylistItems virtual. 
     */
    virtual PlaylistItem *createItem(const QFileInfo &file, QListViewItem *after = 0);

    bool isInternalFile() const;
    QString fileName() const;

    // static methods

    /** 
     * This gets the next item to be played in the specified playlist.
     */
    static PlaylistItem *nextItem(PlaylistItem *current, bool random = false);

protected:
    virtual QDragObject *dragObject();
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);

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
    void fileNameChanged(const QString &fileName);
};

#endif
