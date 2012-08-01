/***************************************************************************
    begin                : Sun Feb 17 2002
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

#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <ksharedptr.h>
#include <kdebug.h>

#include <QVector>
#include <QPixmap>
#include <QList>

#include "tagguesser.h"
#include "filehandle.h"

class Playlist;
class Playlist;
class PlaylistItem;
class CollectionListItem;
class CollectionList;

typedef QList<PlaylistItem *> PlaylistItemList;

/**
 * Items for the Playlist and the baseclass for CollectionListItem.
 * The constructors and destructor are protected and new items should be
 * created via Playlist::createItem().  Items should be removed by
 * Playlist::clear(), Playlist::deleteFromDisk(), Playlist::clearItem() or
 * Playlist::clearItem().
 */

class PlaylistItem
{
    friend class Playlist;
    friend class SearchPlaylist;
    friend class UpcomingPlaylist;
    friend class CollectionList;
    friend class CollectionListItem;
    friend class Pointer;

public:

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
        static QMap<PlaylistItem *, QList<Pointer *> > m_map;
    };
    friend class Pointer;

    static int lastColumn() { return 11; }

    void setFile(const FileHandle &file);
    void setFile(const QString &file);
    FileHandle file() const;

    virtual const QPixmap *pixmap(int column) const;
//     virtual QString text(int column) const;
    virtual void setText(int column, const QString &text);

    void setPlaying(bool playing = true, bool master = true);

    virtual void setSelected(bool selected);
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
     * Returns a reference to the list of the currnetly playing items, with the
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

    /**
     * This is the constructor that shold be used by subclasses.
     */
    PlaylistItem(CollectionList *parent);

    /**
     * See the class documentation for an explanation of construction and deletion
     * of PlaylistItems.
     */
    virtual ~PlaylistItem();

    virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align);
    virtual void paintFocus(QPainter *, const QColorGroup &, const QRect &) {}

    virtual int compare(PlaylistItem *item, int column, bool ascending) const;
    int compare(const PlaylistItem *firstItem, const PlaylistItem *secondItem, int column, bool ascending) const;

    bool isValid() const;

    void setTrackId(quint32 id);

    /**
     * Shared data between all PlaylistItems from the same track (incl. the CollectionItem
     * representing said track.
     */
    struct Data : public KShared
    {
        Data() {}
        Data(const QFileInfo &info, const QString &path) : fileHandle(info, path) {}
        Data(const QString &path) : fileHandle(path) {}

        FileHandle fileHandle;
        QVector<QString> metadata; ///< Artist, album, or genre tags.  Other columns unfilled
        QVector<int> cachedWidths;
    };

    KSharedPtr<Data> data() const { return d; }

private:
    KSharedPtr<Data> d;

    void setup(CollectionListItem *item);

    CollectionListItem *m_collectionItem;
    quint32 m_trackId;
    bool m_watched;
    static PlaylistItemList m_playingItems;
    Playlist *m_playlist;
};

inline QDebug operator<<(QDebug s, const PlaylistItem &item)
{
    // TODO: FIXME ###
//     if(&item == 0)
//         s << "(nil)";
//     else
//         s << item.text(0);

    return s;
}

#endif

// vim: set et sw=4 tw=0 sta:
