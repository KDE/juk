/***************************************************************************
                          playlistitem.h  -  description
                             -------------------
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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

#include <klistview.h>
#include <ksharedptr.h>

#include <qvaluevector.h>
#include <qptrdict.h>

#include "tagguesser.h"
#include "filehandle.h"

class Playlist;
class PlaylistItem;
class CollectionListItem;

typedef QValueList<PlaylistItem *> PlaylistItemList;

/**
 * Items for the Playlist and the baseclass for CollectionListItem.
 * The constructors and destructor are protected and new items should be
 * created via Playlist::createItem().  Items should be removed by
 * Playlist::clear(), Playlist::deleteFromDisk(), Playlist::clearItem() or
 * Playlist::clearItem().
 */

class PlaylistItem : public KListViewItem
{
    friend class Playlist;
    friend class SearchPlaylist;
    friend class CollectionList;
    friend class CollectionListItem;
    friend class QPtrDict<PlaylistItem>;

public:
    enum ColumnType { TrackColumn       = 0,
		      ArtistColumn      = 1,
		      AlbumColumn       = 2,
		      TrackNumberColumn = 3,
		      GenreColumn       = 4,
		      YearColumn        = 5,
		      LengthColumn      = 6,
		      BitrateColumn     = 7,
		      CommentColumn     = 8,
		      FileNameColumn    = 9 };

    static int lastColumn() { return FileNameColumn; }

    void setFile(const FileHandle &file);
    FileHandle file() const;

    virtual QString text(int column) const;
    virtual void setText(int column, const QString &text);

    void setPlaying(bool playing = true) { m_playing = playing; }

    virtual void setSelected(bool selected);
    void guessTagInfo(TagGuesser::Type type);

    Playlist *playlist() const;

    /**
     * The widths of items are cached when they're updated for us in computations
     * in the "weighted" listview column width mode.
     */
    QValueVector<int> cachedWidths() const;

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

protected:
    /**
     * Items should always be created using Playlist::createItem() or through a
     * subclss or friend class.
     */
    PlaylistItem(CollectionListItem *item, Playlist *parent);
    PlaylistItem(CollectionListItem *item, Playlist *parent, QListViewItem *after);

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

    virtual int compare(QListViewItem *item, int column, bool ascending) const;
    int compare(const PlaylistItem *firstItem, const PlaylistItem *secondItem, int column, bool ascending) const;

    bool isValid() const;

    virtual CollectionListItem *collectionItem() const { return m_collectionItem; }

    struct Data : public KShared
    {
	Data() {}
	Data(const QFileInfo &info, const QString &path) : fileHandle(info, path) {}
	Data(const QString &path) : fileHandle(path) {}

	FileHandle fileHandle;
	QValueVector<QCString> local8Bit;
	QValueVector<int> cachedWidths;
	QCString shortFileName;
    };

    KSharedPtr<Data> data() const { return d; }

private:
    KSharedPtr<Data> d;

    void setup(CollectionListItem *item);
    CollectionListItem *m_collectionItem;
    bool m_playing;
};

#endif
