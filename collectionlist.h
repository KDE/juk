/***************************************************************************
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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
#include <kfileitem.h>

#include <qdict.h>
#include <qclipboard.h>
#include <qvaluevector.h>

#include "playlist.h"
#include "playlistitem.h"
#include "sortedstringlist.h"

class CollectionListItem;
class ViewMode;

/**
 * This type is for mapping QString track attributes like the album, artist
 * and track to an integer count representing the number of outstanding items
 * that hold the string.
 */

typedef QDict<int> TagCountDict;
typedef QDictIterator<int> TagCountDictIterator;

/**
 * We then have an array of dicts, one for each column in the list view.  We
 * use pointers to TagCountDicts because QDict has a broken copy ctor, which
 * doesn't copy the case sensitivity setting.
 */

typedef QValueVector<TagCountDict*> TagCountDicts;

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
    static void initialize(PlaylistCollection *collection);

    /**
     * Returns a unique set of values associated with the type specified.
     */
    QStringList uniqueSet(UniqueSetType t) const;

    CollectionListItem *lookup(const QString &file) { return m_itemsDict.find(file); }
    
    virtual PlaylistItem *createItem(const FileHandle &file,
				     QListViewItem * = 0,
				     bool = false);

    void emitVisibleColumnsChanged() { emit signalVisibleColumnsChanged(); }

    virtual void clearItems(const PlaylistItemList &items);

    void setupTreeViewEntries(ViewMode *viewMode) const;

    virtual bool canReload() const { return true; }

public slots:
    virtual void paste() { decode(kapp->clipboard()->data()); }
    virtual void clear();
    void slotCheckCache();

    void slotRemoveItem(const QString &file);
    void slotRefreshItem(const QString &file);
    
    void slotNewItems(const KFileItemList &items);
    void slotRefreshItems(const KFileItemList &items);
    void slotDeleteItem(KFileItem *item);

protected:
    CollectionList(PlaylistCollection *collection);
    virtual ~CollectionList();

    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);

    // These methods are used by CollectionListItem, which is a friend class.

    void addToDict(const QString &file, CollectionListItem *item) { m_itemsDict.replace(file, item); }
    void removeFromDict(const QString &file) { m_itemsDict.remove(file); }

    // These methods are also used by CollectionListItem, to manage the
    // strings used in generating the unique sets and tree view mode playlists.

    QString addStringToDict(const QString &value, unsigned column);
    void removeStringFromDict(const QString &value, unsigned column);

    void addWatched(const QString &file) { m_dirWatch->addFile(file); }
    void removeWatched(const QString &file) { m_dirWatch->removeFile(file); }

    virtual bool hasItem(const QString &file) const { return m_itemsDict.find(file); }

signals:
    void signalCollectionChanged();

    /**
     * This is emitted when the set of columns that is visible is changed.
     *
     * \see Playlist::hideColumn()
     * \see Playlist::showColumn()
     * \see Playlsit::isColumnVisible()
     */
    void signalVisibleColumnsChanged();
    void signalNewTag(const QString &, unsigned);
    void signalRemovedTag(const QString &, unsigned);

private:
    /**
     * Just the size of the above enum to keep from hard coding it in several
     * locations.
     */
    static const int m_uniqueSetCount = 3;

    static CollectionList *m_list;
    QDict<CollectionListItem> m_itemsDict;
    KDirWatch *m_dirWatch;
    TagCountDicts m_columnTags;
};

class CollectionListItem : public PlaylistItem
{
    friend class Playlist;
    friend class CollectionList;
    friend class PlaylistItem;

    /** 
     * Needs access to the destructor, even though the destructor isn't used by QDict.
     */
    friend class QDict<CollectionListItem>;

public:
    virtual void refresh();
    PlaylistItem *itemForPlaylist(const Playlist *playlist);
    void updateCollectionDict(const QString &oldPath, const QString &newPath);
    void repaint() const;
    PlaylistItemList children() const { return m_children; }

protected:
    CollectionListItem(const FileHandle &file);
    virtual ~CollectionListItem();

    void addChildItem(PlaylistItem *child);
    void removeChildItem(PlaylistItem *child);

    /**
     * Returns true if the item is now up to date (even if this required a refresh) or
     * false if the item is invalid.
     */
    bool checkCurrent();

    virtual CollectionListItem *collectionItem() { return this; }

private:
    bool m_shuttingDown;
    PlaylistItemList m_children;
};

#endif
