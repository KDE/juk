/***************************************************************************
    begin                : Sat Feb 16 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
    copyright            : (c) 2007 Michael Pyne
    email                : michael.pyne@kdemail.net
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

#include <kglobalsettings.h>
#include <kdebug.h>

#include <QVector>
#include <QEvent>
#include <QList>
#include <QAbstractTableModel>

#include "covermanager.h"
#include "stringhash.h"
#include "playlist/playlistsearch.h"
#include "tagguesser.h"
#include "playlist/playlistinterface.h"

class KMenu;
class KActionMenu;

class QFileInfo;
class QMimeData;
class QDrag;
class QAction;

class WebImageFetcher;
class PlaylistCollection;
class PlaylistToolTip;

class Playlist : public QAbstractTableModel, public PlaylistInterface
{
    Q_OBJECT
    
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
                    FullPathColumn    = 11,
        
                    // From other classes, you can't extend enums in c++
                    PlayedColumn      = 12 // HistoryPlaylist
                    
    };


    explicit Playlist(PlaylistCollection *collection, const QString &name = QString(),
             const QString &iconName = "audio-midi");
    Playlist(PlaylistCollection *collection, const FileHandleList &items,
             const QString &name = QString(), const QString &iconName = "audio-midi");
    Playlist(PlaylistCollection *collection, const QFileInfo &playlistFile,
             const QString &iconName = "audio-midi");

    /**
     * This constructor should generally only be used either by the cache
     * restoration methods or by subclasses that want to handle calls to
     * PlaylistCollection::setupPlaylist() differently.
     *
     * @param extraColumns is used to preallocate columns for subclasses that
     * need them (since extra columns are assumed to start from 0). extraColumns
     * should be equal to columnOffset() (we can't use columnOffset until the
     * ctor has run).
     */
    Playlist(PlaylistCollection *collection, bool delaySetup, int extraColumns = 0);

    virtual ~Playlist();
    
public:
    // The following functions implement the QAbstractListModel API
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;
    Qt::ItemFlags flags (const QModelIndex & index) const;
    bool setData (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    bool insertRows (int row, int count, const QModelIndex & parent = QModelIndex());
    bool removeRows (int row, int count, const QModelIndex & parent = QModelIndex());
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    bool hasChildren(const QModelIndex& index) const;
    using QAbstractTableModel::sort;
    

    void insertItem(int pos , const QModelIndex& item);
    void insertFile(const FileHandle& file, int pos = -1);
    /**
     * Remove all instances of a file from playlist
     */
    virtual void removeFile(const FileHandle& file);
    const FileHandleList &fileHandles() const { return m_items; }
    /**
     * Removes rows from playlist and deletes files from disk.
     */
    bool deleteRows(int row, int count, const QModelIndex & parent = QModelIndex());
    
    void clearRow(int row);
    
    /**
     * Refreshes a file from disk
     * @return True on success
     */
    void refreshRows(QModelIndexList& l);
    
    // The following group of functions implement the PlaylistInterface API.

    virtual QString name() const;
    virtual int count() const { return m_items.count(); }
    virtual int time() const;

    /**
     * Saves the file to the currently set file name.  If there is no filename
     * currently set, the default behavior is to prompt the user for a file
     * name.
     */
    virtual void save();

    /**
     * Standard "save as".  Prompts the user for a location where to save the
     * playlist to.
     */
    virtual void saveAs();

    /**
     * Removes all items.
     */
    void clear() { removeRows(0, rowCount()); }

    /**
     * All of the (media) files in the list.
     */
    QStringList files() const;

    /**
     * Allow duplicate files in the playlist.
     */
    void setAllowDuplicates(bool allow) { m_allowDuplicates = allow; }

    /**
     * This handles adding files of various types -- music, playlist or directory
     * files.  Music files that are found will be added to this playlist.  New
     * playlist files that are found will result in new playlists being created.
     *
     * Note that this should not be used in the case of adding *only* playlist
     * items since it has the overhead of checking to see if the file is a playlist
     * or directory first.
     */
    virtual void addFiles(const QStringList &files, int pos = -1);

    /**
     * Returns the file name associated with this playlist (an m3u file) or
     * an empty QString if no such file exists.
     */
    QString fileName() const { return m_fileName; }

    /**
     * Sets the file name to be associated with this playlist; this file should
     * have the "m3u" extension.
     */
    void setFileName(const QString &n) { m_fileName = n; }

    /**
     * This sets a name for the playlist that is \e different from the file name.
     */
    void setName(const QString &n);

    /**
     * Returns the search associated with this list, or an empty search if one
     * has not yet been set.
     */
    PlaylistSearch search() const { return m_search; }

    /**
     * Some subclasses of Playlist will be "read only" lists (i.e. the history
     * playlist).  This is a way for those subclasses to indicate that to the
     * Playlist internals.
     */
    virtual bool readOnly() const { return false; }

    /**
     * Returns true if it's possible to reload this playlist.
     */
    virtual bool canReload() const { return !m_fileName.isEmpty(); }

    /**
     * Playlists have a common set of shared settings such as visible columns
     * that should be applied just before the playlist is shown.  Calling this
     * method applies those.
     */
    void applySharedSettings();

    void read(QDataStream &s);

    static void setShuttingDown() { m_shuttingDown = true; }
    
    void moveItem(int from, int to) { m_items.move(from, to); }

public slots:
    /**
     * Reload the playlist contents from the m3u file.
     */
    virtual void slotReload();

    virtual void weChanged();

protected:
    /**
     * Remove \a items from the playlist and disk.  This will ignore items that
     * are not actually in the list.
     */
    void removeFromDisk(const QList<int> &rows);

    virtual bool hasItem(const QString &file) const { return m_members.contains(file); }

    /**
     * Forwards the call to the parent to enable or disable automatic deletion
     * of tree view playlists.  Used by CollectionListItem.
     */
    void setDynamicListsFrozen(bool frozen);

signals:

    /**
     * This is connected to the PlaylistBox::Item to let it know when the
     * playlist's name has changed.
     */
    void signalNameChanged(const QString &name);

    void signalEnableDirWatch(bool enable);

    void coverChanged();

    void signalPlaylistItemsDropped(Playlist *p);
    
    void dataChanged(const QModelIndex&, const QModelIndex&);

private:
    /**
     * This function is called to let the user know that JuK has automatically enabled
     * manual column width adjust mode.
     */
    void notifyUserColumnWidthModeChanged();

    /**
     * Load the playlist from a file.  \a fileName should be the absolute path.
     * \a fileInfo should point to the same file as \a fileName.  This is a
     * little awkward API-wise, but keeps us from throwing away useful
     * information.
     */
    void loadFile(const QString &fileName, const QFileInfo &fileInfo);

    void addFile(const QString &file, FileHandleList &files, bool importPlaylists,
                 int row = -1);
    void addFileHelper(FileHandleList &files, int row = -1,
                       bool ignoreTimer = false);

    /**
     * Sets the cover for items to the cover identified by id.
     */
    void refreshAlbums(const QList<int> &rows, coverKey id = CoverManager::NoMatch);

    void refreshAlbum(const QString &artist, const QString &album);

    void refresh(const QModelIndex &index);

private:
    PlaylistCollection *m_collection;
    
    FileHandleList m_items;

    StringHash m_members;

    WebImageFetcher *m_fetcher;

    int m_currentColumn;
    int m_processed;
    int m_selectedCount;

    bool m_allowDuplicates;
    bool m_applySharedSettings;
    bool m_columnWidthModeChanged;

    QList<int> m_weightDirty;
    bool m_disableColumnWidthUpdates;

    mutable int m_time;
    mutable FileHandleList m_addTime;
    mutable FileHandleList m_subtractTime;

    static FileHandleList m_history;
    PlaylistSearch m_search;

    bool m_searchEnabled;

    /**
     * This is only defined if the playlist name is something other than the
     * file name.
     */
    QString m_playlistName;
    QString m_fileName;

    static bool m_shuttingDown;
    static int m_leftColumn;

    bool m_blockDataChanged;
};

typedef QList<Playlist *> PlaylistList;

bool processEvents();

class FocusUpEvent : public QEvent
{
public:
    FocusUpEvent() : QEvent(id) {}
    Type type() const { return id; }

    static const Type id = static_cast<Type>(QEvent::User + 1);
};

QDataStream &operator<<(QDataStream &s, const Playlist &p);
QDataStream &operator>>(QDataStream &s, Playlist &p);

#endif

// vim: set et sw=4 tw=0 sta:
