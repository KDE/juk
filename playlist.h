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
    virtual ~Playlist();

    virtual void save();
    virtual void saveAs();

    /** Set sorted = false to add the items at the end of the list rather
	than adding them into the list in their sorted place. */
    virtual void add(const QString &item, bool sorted = true);
    virtual void add(const QStringList &items, bool sorted = true);

    virtual void refresh();

    virtual void clearItems(const PlaylistItemList &items);

    /** All of the files in the list. */
    QStringList files() const;
    /** All of the items in the list. */
    PlaylistItemList items() const;
    PlaylistItemList selectedItems() const;

    void remove();
    void remove(const PlaylistItemList &items);

    /** Allow duplicate files in the playlist. */
    void setAllowDuplicates(bool allow);

    // These are used in a hard-core, encapsulation breaking way and should be
    // replaced soon (see PlaylistItem).  Unfortunately they're more efficient
    // than elegant solutions.  These also should be removed by doing the
    // checking that is currently done in PlaylistItem in the CollectionList
    // subclass of Playlist.

    QStringList &getArtistList();
    QStringList &getAlbumList();

protected:
    virtual QDragObject *dragObject();
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    /** This is being used as a mini-factory of sorts to make the construction
	of PlaylistItems virtual. */
    virtual PlaylistItem *createItem(const QFileInfo &file, bool sorted = true);
    virtual void addImpl(const QString &item, bool sorted = true);

private:
    void setup();
    void processEvents();

    QStringList extensions;
    QStringList members;
    QStringList artistList;
    QStringList albumList;
    int processed;
    bool collectionListChanged;
    bool allowDuplicates;

private slots:
    void emitSelected();

signals:
    /** This signal is connected to PlaylistItem::refreshed() in the 
	PlaylistItem class. */
    void dataChanged();

    /** This signal is emitted when items are added to the collection list.  
	This happens in the createItem() method when items are added to the 
	collection. */
    void collectionChanged();

    /** This is emitted when the playlist selection is changed.  This is used
	primarily to notify the TagEditor of the new data. */
    void selectionChanged(const PlaylistItemList &selection);
};

#endif
