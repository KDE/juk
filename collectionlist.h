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

#include <QHash>
#include <QVector>

#include "playlist/playlists/playlist.h"

class ViewMode;
class KFileItem;
class KFileItemList;
class KDirWatch;

/**
 * This type is for mapping QString track attributes like the album, artist
 * and track to an integer count representing the number of outstanding items
 * that hold the string.
 */

typedef QHash<QString, int> TagCountDict;
typedef QHashIterator<QString, int> TagCountDictIterator;

/**
 * We then have an array of dicts, one for each column in the list view.
 * The array is sparse (not every vector will have a TagCountDict so we use
 * pointers.
 */

typedef QVector<TagCountDict *> TagCountDicts;

class CollectionList : public Playlist
{
//     friend class CollectionListItem;

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

    const FileHandle &lookup(const QString& file) const;

    void emitVisibleColumnsChanged() { emit signalVisibleColumnsChanged(); }

    void setupTreeViewEntries(ViewMode *viewMode) const;

    virtual bool canReload() const { return true; }
    
    virtual void removeFile(const FileHandle& file);

public slots:
    virtual void paste();
    virtual void clear();
    void slotCheckCache();

    void slotRemoveItem(const QString &file);
    void slotRefreshItem(const QString &file);

    void slotNewItems(const KFileItemList &items);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &items);

protected:
    CollectionList(PlaylistCollection *collection);
    virtual ~CollectionList();

//     virtual void contentsDropEvent(QDropEvent *e);
//     virtual void contentsDragMoveEvent(QDragMoveEvent *e);

    // These methods are used by CollectionListItem, which is a friend class.

    void addToDict(const QString &file, const FileHandle &item) { m_itemsDict.insert(file, item); }
    void removeFromDict(const QString &file) { m_itemsDict.remove(file); }

    // These methods are also used by CollectionListItem, to manage the
    // strings used in generating the unique sets and tree view mode playlists.

    QString addStringToDict(const QString &value, int column);
    void removeStringFromDict(const QString &value, int column);

    void addWatched(const QString &file);
    void removeWatched(const QString &file);

    virtual bool hasItem(const QString &file) const { return m_itemsDict.contains(file); }

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

public slots:
    /**
     * Loads the CollectionListItems from the Cache.  Should be called after program
     * initialization.
     */
    void loadCachedItems();

private:
    /**
     * Just the size of the above enum to keep from hard coding it in several
     * locations.
     */
    static const int m_uniqueSetCount = 3;

    static CollectionList *m_list;
    QHash<QString, FileHandle> m_itemsDict;
    KDirWatch *m_dirWatch;
    TagCountDicts m_columnTags;
};

#endif

// vim: set et sw=4 tw=0 sta:
