/***************************************************************************
                          collectionlist.h  -  description
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

#ifndef COLLECTIONLIST_H
#define COLLECTIONLIST_H

#include <kapplication.h>
#include <kdirwatch.h>

#include <qclipboard.h>
#include <qdict.h>

#include "playlist.h"
#include "playlistitem.h"
#include "sortedstringlist.h"
#include "stringhash.h"

class CollectionListItem;

/** 
 * This is the "collection", or all of the music files that have been opened
 * in any playlist and not explicitly removed from the collection.
 *
 * It is being implemented as a "semi-singleton" because I need universal access
 * to just one instance.  However, because the collection needs initialization 
 * parameters (that will not always be available when an instance is needed).  
 * Hence there will be the familiar singleton "instance()" method allong with an
 * "initialize()" method.
 */

class CollectionList : public Playlist
{
    friend class CollectionListItem;

    Q_OBJECT

public: 
    /**
     * A variety of unique value lists will be kept in the collection.  This
     * enum can be used as an index into those structures.
     */
    enum UniqueSetType { Artists = 0, Albums = 1, Genres = 2 };

    static CollectionList *instance();
    static void initialize(QWidget *parent, bool restoreOnLoad = true);

    /**
     * Returns a unique set of values associated with the type specified.
     */
    QStringList uniqueSet(UniqueSetType t) const { return m_uniqueSets[t].values(); }

    /**
     * This is similar to uniqueSet() but in this case only returns values for
     * which there are two or more occurances in the collection list.  This is
     * useful when grouping items.
     */
    QStringList viewModeItems(UniqueSetType t) const { return m_viewModeItems[t].values(); }

    CollectionListItem *lookup(const QString &file) { return m_itemsDict.find(file); }
    virtual PlaylistItem *createItem(const QFileInfo &file, 
				     const QString &absFilePath = QString::null, 
				     QListViewItem * = 0,
				     bool = false);

public slots:
    virtual void paste() { decode(kapp->clipboard()->data()); }
    virtual void clear();
    void slotCheckCache();
    
protected:
    CollectionList(QWidget *parent);
    virtual ~CollectionList();

    virtual void decode(QMimeSource *s);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);

    // These methods are used by CollectionListItem, which is a friend class.

    void addToDict(const QString &file, CollectionListItem *item) { m_itemsDict.replace(file, item); }
    void removeFromDict(const QString &file) { m_itemsDict.remove(file); }

    /**
     * Add a value to one of the unique value lists; use the UniqueSetType as a key.
     */
    void addUnique(UniqueSetType t, const QString &value);

    void addWatched(const QString &file) { m_dirWatch->addFile(file); }
    void removeWatched(const QString &file) { m_dirWatch->removeFile(file); }

signals:
    void signalCollectionChanged();
    void signalRequestPlaylistCreation(const PlaylistItemList &items);

private slots:
    void slotRemoveItem(const QString &file);
    void slotRefreshItem(const QString &file);
    void slotCreateGroup();
    
private:
    /**
     * Just the size of the above enum to keep from hard coding it in several
     * locations.
     */
    static const int m_uniqueSetCount = 3;

    static CollectionList *m_list;
    QDict<CollectionListItem> m_itemsDict;
    SortedStringList m_artists;
    SortedStringList m_albums;
    SortedStringList m_genres;
    KDirWatch *m_dirWatch;
    QValueVector<SortedStringList> m_viewModeItems;
    QValueVector<SortedStringList> m_uniqueSets;
    QValueVector<QString> m_uniqueSetLast;
};

class CollectionListItem : public PlaylistItem
{
    friend class Playlist;
    friend class CollectionList;
    friend class PlaylistItem;

    /** 
     * Needs access to the destuctor, even though the destructor isn't used by QDict.
     */
    friend class QDict<CollectionListItem>;

    Q_OBJECT

public slots:
    virtual void slotRefresh();

protected:
    CollectionListItem(const QFileInfo &file, const QString &path);
    virtual ~CollectionListItem();

    void addChildItem(PlaylistItem *child);

    /**
     * This slot, called from a QTimer::singleShot() set in the constructor, allows for
     * delayed consistancy checking for the cache at the cost of a few CPU cycles.  The
     * effect however is that stating files is delayed until after the GUI is shown by
     * moving this action into the event loop.
     *
     * Returns true if the item is now up to date (even if this required a refresh) or
     * false if the item is invalid.
     */
    bool checkCurrent();

    virtual CollectionListItem *collectionItem() { return this; }

private:
    QString m_path;
};

#endif
