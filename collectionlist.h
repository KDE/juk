/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COLLECTIONLIST_H
#define COLLECTIONLIST_H

#include <QHash>
#include <QVector>

#include "playlist.h"
#include "playlistitem.h"

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

class CollectionListItem : public PlaylistItem
{
    friend class Playlist;
    friend class CollectionList;
    friend class PlaylistItem;

public:
    virtual void refresh();
    PlaylistItem *itemForPlaylist(const Playlist *playlist);
    void updateCollectionDict(const QString &oldPath, const QString &newPath);
    void repaint() const;
    PlaylistItemList children() const { return m_children; }

protected:
    CollectionListItem(CollectionList *parent, const FileHandle &file);
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

    CollectionListItem *lookup(const QString &file) const;

    virtual CollectionListItem *createItem(const FileHandle &file,
                                     Q3ListViewItem * = 0,
                                     bool = false);

    void emitVisibleColumnsChanged() { emit signalVisibleColumnsChanged(); }

    virtual void clearItems(const PlaylistItemList &items);

    void setupTreeViewEntries(ViewMode *viewMode) const;

    virtual bool canReload() const { return true; }

    void saveItemsToCache() const;

public slots:
    virtual void paste();
    virtual void clear();
    void slotCheckCache();

    void slotRemoveItem(const QString &file);
    void slotRefreshItem(const QString &file);

    void slotNewItems(const KFileItemList &items);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &items);
    void slotDeleteItem(const KFileItem &item);

protected:
    CollectionList(PlaylistCollection *collection);
    virtual ~CollectionList();

    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);

    // These methods are used by CollectionListItem, which is a friend class.

    void addToDict(const QString &file, CollectionListItem *item) { m_itemsDict.insert(file, item); }
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

    // Emitted once cached items are loaded, which allows for folder scanning
    // and invalid track detection to proceed.
    void cachedItemsLoaded();

public slots:
    /**
     * Loads the CollectionListItems from the Cache.  Should be called after program
     * initialization.
     */
    void startLoadingCachedItems();

    /**
     * Loads a few items at a time. Intended to be single-shotted into the event
     * loop so that loading the music doesn't freeze the GUI.
     */
    void loadNextBatchCachedItems();

    /**
     * Teardown from cache loading (e.g. splash screen, sorting, etc.). Should
     * always be called if startLoadingCachedItems is called.
     */
    void completedLoadingCachedItems();

private:
    /**
     * Just the size of the above enum to keep from hard coding it in several
     * locations.
     */
    static const int m_uniqueSetCount = 3;

    static CollectionList *m_list;
    QHash<QString, CollectionListItem *> m_itemsDict;
    KDirWatch *m_dirWatch;
    TagCountDicts m_columnTags;
};

#endif

// vim: set et sw=4 tw=0 sta:
