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

#ifndef JUK_PLAYLISTITEM_H
#define JUK_PLAYLISTITEM_H

#include <QExplicitlySharedDataPointer>
#include <QVector>
#include <QHash>
#include <QTreeWidgetItem>

#include "tagguesser.h"
#include "filehandle.h"
#include "juk_debug.h"

class Playlist;
class PlaylistItem;
class CollectionListItem;
class CollectionList;

typedef QVector<PlaylistItem *> PlaylistItemList;

/**
 * Items for the Playlist and the baseclass for CollectionListItem.
 * The constructors and destructor are protected and new items should be
 * created via Playlist::createItem().  Items should be removed by
 * Playlist::clear(), Playlist::deleteFromDisk(), Playlist::clearItem() or
 * Playlist::clearItem().
 */

class PlaylistItem : public QTreeWidgetItem
{
    friend class Playlist;
    friend class SearchPlaylist;
    friend class UpcomingPlaylist;
    friend class CollectionList;
    friend class CollectionListItem;
    friend class Pointer;

public:
    enum ColumnType { TrackColumn       = 0,
                      ArtistColumn      = 1,
                      AlbumColumn       = 2,
                      CoverColumn       = 3,
                      TrackNumberColumn = 4,
                      GenreColumn       = 5,
                      YearColumn        = 6,
                      LengthColumn      = 7,
                      BitrateColumn     = 8,
                      CommentColumn     = 9,
                      FileNameColumn    = 10,
                      FullPathColumn    = 11 };

    /**
     * A helper class to implement guarded pointer semantics.
     */

    class Pointer
    {
    public:
        Pointer() : m_item(0) {}
        Pointer(PlaylistItem *item);
        Pointer(const Pointer &p);
        ~Pointer();
        Pointer &operator=(PlaylistItem *item);
        bool operator==(const Pointer &p) const { return m_item == p.m_item; }
        bool operator!=(const Pointer &p) const { return m_item != p.m_item; }
        PlaylistItem *operator->() const { return m_item; }
        PlaylistItem &operator*() const { return *m_item; }
        operator PlaylistItem*() const { return m_item; }
        static void clear(PlaylistItem *item);

    private:
        PlaylistItem *m_item;
        static QHash<PlaylistItem *, QVector<Pointer *> > m_itemPointers;
    };
    friend class Pointer;

    static int lastColumn() { return FullPathColumn; }

    void setFile(const FileHandle &file);
    void setFile(const QString &file);
    FileHandle file() const;

    virtual QString text(int column) const;
    virtual void setText(int column, const QString &text);

    bool isPlaying() const;
    void setPlaying(bool playing = true, bool master = true);

    void guessTagInfo(TagGuesser::Type type);

    Playlist *playlist() const;

    virtual CollectionListItem *collectionItem() { return m_collectionItem; }

    /**
     * This is an identifier for the playlist item which will remain unique
     * throughout the process lifetime. It stays constant once the PlaylistItem
     * is created.
     */
    quint32 trackId() const { return m_trackId; }

    /**
     * The widths of items are cached when they're updated for us in computations
     * in the "weighted" listview column width mode.
     */
    QVector<int> cachedWidths() const;

    /**
     * This just refreshes from the in memory data.  This may seem pointless at
     * first, but this data is shared between all of the list view items that are
     * based on the same file, so if another one of those items changes its data
     * it is important to refresh the others.
     */
    virtual void refresh();

    /**
     * This rereads the tag from disk.  This affects all PlaylistItems based on
     * the same file.
     */
    virtual void refreshFromDisk();

    /**
     * Asks the item's playlist to remove the item (which uses deleteLater()).
     */
    virtual void clear();

    /**
     * Returns properly casted item below this one.
     */
    PlaylistItem *itemBelow() { return static_cast<PlaylistItem *>(treeWidget()->itemBelow(this)); }

    /**
     * Returns properly casted item above this one.
     */
    PlaylistItem *itemAbove() { return static_cast<PlaylistItem *>(treeWidget()->itemAbove(this)); }

    /**
     * Returns a reference to the list of the currently playing items, with the
     * first being the "master" item (i.e. the item from which the next track is
     * chosen).
     */
    static const PlaylistItemList &playingItems() { return m_playingItems; }

protected:
    /**
     * Items should always be created using Playlist::createItem() or through a
     * subclass or friend class.
     */
    PlaylistItem(CollectionListItem *item, Playlist *parent);
    PlaylistItem(CollectionListItem *item, Playlist *parent, QTreeWidgetItem *after);

    /**
     * See the class documentation for an explanation of construction and deletion
     * of PlaylistItems.
     */
    virtual ~PlaylistItem();

    virtual int compare(const QTreeWidgetItem *item, int column, bool ascending) const;
    int compare(const PlaylistItem *firstItem, const PlaylistItem *secondItem, int column, bool ascending) const;

    bool operator<(const QTreeWidgetItem &other) const override;

    bool isValid() const;

    void setTrackId(quint32 id);

    /**
     * Shared data between all PlaylistItems from the same track (incl. the CollectionItem
     * representing said track.
     */
    struct Data : public QSharedData
    {
        FileHandle fileHandle; // Set within CollectionList
        QVector<QString> metadata; ///< Artist, album, or genre tags.  Other columns unfilled
        QVector<int> cachedWidths;
    };

    using DataPtr = QExplicitlySharedDataPointer<Data>;
    DataPtr sharedData() const { return d; }

private:
    DataPtr d;

    /**
     * This is the constructor that should be used by CollectionList.
     */
    PlaylistItem(CollectionList *parent);
    friend class CollectionList;

    void setup(CollectionListItem *item);

    CollectionListItem *m_collectionItem;
    quint32 m_trackId;
    bool m_watched;
    static PlaylistItemList m_playingItems;
};

inline QDebug operator<<(QDebug s, const PlaylistItem &item)
{
    s << item.text(PlaylistItem::TrackColumn);
    return s;
}

#endif

// vim: set et sw=4 tw=0 sta:
