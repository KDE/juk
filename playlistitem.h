/***************************************************************************
                          playlistitem.h  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <config.h>

#include <klistview.h>

#include <qfileinfo.h>
#include <qobject.h>
#include <qptrstack.h>

#include "musicbrainzquery.h"
#include "tag.h"
#include "cache.h"

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

class PlaylistItem : public QObject, public KListViewItem
{
    friend class Playlist;
    friend class CollectionList;
    friend class QPtrList<PlaylistItem>;

    /**
     * Needs access to the destuctor, even though the destructor isn't used by QPtrStack.
     */
    friend class QPtrStack<PlaylistItem>;

    Q_OBJECT

public:
    enum ColumnType { TrackColumn = 0, ArtistColumn = 1, AlbumColumn = 2, TrackNumberColumn = 3,
                      GenreColumn = 4, YearColumn = 5, LengthColumn = 6, CommentColumn = 7,
		      FileNameColumn = 8 };

    static int lastColumn() { return FileNameColumn; }

    void setFile(const QString &file);
    Tag *tag();
    const Tag *tag() const;


    // These are just forwarding methods to PlaylistItem::Data, a QFileInfo
    // subclass.

    QString fileName() const;
    QString filePath() const;
    QString absFilePath() const;
    QString dirPath(bool absPath = false) const;
    bool isWritable() const;
    void setPlaying(bool playing = true) { m_playing = playing; }

    void guessTagInfoFromFile();
    void guessTagInfoFromInternet();
    void renameFile();

public slots:
    /**
     * This just refreshes from the in memory data.  This may seem pointless at
     * first, but this data is shared between all of the list view items that are
     * based on the same file, so if another one of those items changes its data
     * it is important to refresh the others.
     */
    virtual void slotRefresh();

    /**
     * This rereads the tag from disk.  This affects all PlaylistItems based on
     * the same file.
     */
    virtual void slotRefreshFromDisk();

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

    class Data;
    Data *data() { return m_data; }
    const Data *data() const { return m_data; }
    void setData(Data *d) { m_data = d; }

    virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align);

    virtual int compare(QListViewItem *item, int column, bool ascending) const;
    int compare(const PlaylistItem *firstItem, const PlaylistItem *secondItem, int column, bool ascending) const;

    bool isValid() const;

    virtual CollectionListItem *collectionItem() const { return m_collectionItem; }

protected slots:
    void slotRefreshImpl();
#ifdef HAVE_MUSICBRAINZ
    void slotTagGuessResults(const MusicBrainzQuery::TrackList &);
#endif

signals:
    void signalRefreshed();

private:
    void setup(CollectionListItem *item, Playlist *parent);

    CollectionListItem *m_collectionItem;
    Data *m_data;
    bool m_playing;
};

/**
 * This is the data class for PlaylistItems.  Several PlaylistItems that are
 * based on the same file will share the m_data member.  This has both the
 * advantages of being memory efficient and allowing the PlaylistItems to stay
 * synchronized.
 *
 * The sharing is implemented through a refcount and protected constructors and
 * destructors that make it necessary to obtain pointers via newUser() and to
 * free an instance using deleteUser().
 */

class PlaylistItem::Data
{
public:
    static Data *newUser(const QFileInfo &file, const QString &path);
    Data *newUser();
    void deleteUser();

    void refresh();

    Tag *tag();
    const Tag *tag() const;

    void setFile(const QString &file);
    bool isFile() const { return m_fileInfo.isFile(); }
    bool exists() const { return m_fileInfo.exists(); }
    const QFileInfo *fileInfo() const { return &m_fileInfo; }

    QString absFilePath() const { return m_absFileName; }

protected:
    /**
     * Because we're trying to use this as a shared item, we want all access
     * to be through pointers (so that it's safe to use delete this).  Thus
     * creation of the object should be done by the newUser methods above
     * and deletion should be handled by deleteUser.  Making the constructor
     * and destructor protected ensures this.
     */
    Data(const QFileInfo &file, const QString &path);
    ~Data();

private:
    QFileInfo m_fileInfo;
    int m_referenceCount;
    Tag *m_dataTag;
    QString m_absFileName;
};

#endif
