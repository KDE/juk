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
class PlaylistItem;
class PlaylistCollection;
class PlaylistToolTip;
class CollectionListItem;

typedef QList<PlaylistItem *> PlaylistItemList;

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
                    FullPathColumn    = 11 };


    explicit Playlist(PlaylistCollection *collection, const QString &name = QString(),
             const QString &iconName = "audio-midi");
    Playlist(PlaylistCollection *collection, const PlaylistItemList &items,
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
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    int columnCount (const QModelIndex & parent = QModelIndex()) const;
    Qt::ItemFlags flags (const QModelIndex & index) const;
    bool setData (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    bool insertRows (int row, int count, const QModelIndex & parent = QModelIndex());
    bool removeRows (int row, int count, const QModelIndex & parent = QModelIndex());
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    bool hasChildren(const QModelIndex& index) const;
    using QAbstractTableModel::sort;
    

    void insertItem(int pos , const QModelIndex& item);
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
     * Removes \a item from the Playlist, but not from the disk.  If
     * \a emitChanged is true this will also notify relevant classes
     * that the content of the list has changed.
     *
     * In some situations, for instance when removing items in a loop, it is
     * preferable to delay this notification until after other operations have
     * completed.  In those cases set \a emitChanged to false and call the
     * signal directly.
     */
    virtual void clearItem(PlaylistItem *item, bool emitChanged = true);

    /**
     * Remove \a items from the playlist and emit a signal indicating
     * that the number of items in the list has changed.
     */
    virtual void clearItems(const PlaylistItemList &items);
    
    /**
     * Removes all items.
     */
    void clear() { clearItems(m_items); }

    /**
     * All of the (media) files in the list.
     */
    QStringList files() const;

    /**
     * Returns a list of all of the \e visible items in the playlist.
     */
//     PlaylistItemList visibleItems();

    /**
     * Allow duplicate files in the playlist.
     */
    void setAllowDuplicates(bool allow) { m_allowDuplicates = allow; }

    /**
     * This is being used as a mini-factory of sorts to make the construction
     * of PlaylistItems virtual.  In this case it allows for the creation of
     * both PlaylistItems and CollectionListItems.
     */
    virtual PlaylistItem *createItem(const FileHandle &file,
                                     PlaylistItem *after,
                                     bool emitChanged = true);

    /**
     * This is implemented as a template method to allow subclasses to
     * instantiate their PlaylistItem subclasses using the same method.
     */
    template <class ItemType>
    ItemType *createItem(const FileHandle &file,
                         ItemType *after = 0,
                         bool emitChanged = true);

    virtual void createItems(const PlaylistItemList &siblings, PlaylistItem *after = 0);

    /**
     * This handles adding files of various types -- music, playlist or directory
     * files.  Music files that are found will be added to this playlist.  New
     * playlist files that are found will result in new playlists being created.
     *
     * Note that this should not be used in the case of adding *only* playlist
     * items since it has the overhead of checking to see if the file is a playlist
     * or directory first.
     */
    virtual void addFiles(const QStringList &files, PlaylistItem *after = 0);

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
    
    PlaylistItem *lastItem() { return m_items.last(); }
    PlaylistItem *firstItem() { return m_items.first(); }
    const PlaylistItemList &items() const { return m_items; }
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
    void removeFromDisk(const PlaylistItemList &items);

    virtual bool hasItem(const QString &file) const { return m_members.contains(file); }


    /**
     * Do some finial initialization of created items.  Notably ensure that they
     * are shown or hidden based on the contents of the current PlaylistSearch.
     *
     * This is called by the PlaylistItem constructor.
     */
    void setupItem(PlaylistItem *item);

    /**
     * Forwards the call to the parent to enable or disable automatic deletion
     * of tree view playlists.  Used by CollectionListItem.
     */
    void setDynamicListsFrozen(bool frozen);

    template <class ItemType, class SiblingType>
    ItemType *createItem(SiblingType *sibling, ItemType *after = 0);

    /**
     * As a template this allows us to use the same code to initialize the items
     * in subclasses. ItemType should be a PlaylistItem subclass.
     */
    template <class ItemType, class SiblingType>
    void createItems(const QList<SiblingType *> &siblings, ItemType *after = 0);

signals:

    /**
     * This is connected to the PlaylistBox::Item to let it know when the
     * playlist's name has changed.
     */
    void signalNameChanged(const QString &name);

    /**
     * This signal is emitted just before a playlist item is removed from the
     * list allowing for any cleanup that needs to happen.  Typically this
     * is used to remove the item from the history and safeguard against
     * dangling pointers.
     */
    void signalAboutToRemove(PlaylistItem *item);

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
                 PlaylistItem **after);
    void addFileHelper(FileHandleList &files, PlaylistItem **after,
                       bool ignoreTimer = false);

    /**
     * Sets the cover for items to the cover identified by id.
     */
    void refreshAlbums(const PlaylistItemList &items, coverKey id = CoverManager::NoMatch);

    void refreshAlbum(const QString &artist, const QString &album);

    /**
     * This function should be called when item is deleted to ensure that any
     * internal bookkeeping is performed.  It is automatically called by
     * PlaylistItem::~PlaylistItem and by clearItem() and clearItems().
     */
    void updateDeletedItem(PlaylistItem *item);

    /**
     * Used as a helper to implement template<> createItem().  This grabs the
     * CollectionListItem for file if it exists, otherwise it creates a new one and
     * returns that.  If 0 is returned then some kind of error occurred, such as file not
     * found and probably nothing should be done with the FileHandle you have.
     */
    CollectionListItem *collectionListItem(const FileHandle &file);
    void refresh(const QModelIndex &index);

    /**
     * This class is used internally to store settings that are shared by all
     * of the playlists, such as column order.  It is implemented as a singleton.
     */
    class SharedSettings;

private:
    friend class PlaylistItem;

    PlaylistCollection *m_collection;
    
    PlaylistItemList m_items;

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
    mutable PlaylistItemList m_addTime;
    mutable PlaylistItemList m_subtractTime;

    /**
     * The average minimum widths of columns to be used in balancing calculations.
     */
    QVector<int> m_columnWeights;
    QVector<int> m_columnFixedWidths;
    bool m_widthsDirty;

    static PlaylistItemList m_history;
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

// template method implementations

template <class ItemType>
ItemType *Playlist::createItem(const FileHandle &file, ItemType *after,
                               bool emitChanged)
{
    CollectionListItem *item = collectionListItem(file);
    if(item && (!m_members.insert(file.absFilePath()) || m_allowDuplicates)) {

        ItemType *i = new ItemType(item, this);
                
        m_items.insert(m_items.indexOf(after), i);

        setupItem(i);

        if(emitChanged)
            weChanged();

        return i;
    }
    else
        return 0;
}

template <class ItemType, class SiblingType>
ItemType *Playlist::createItem(SiblingType *sibling, ItemType *after)
{
    m_disableColumnWidthUpdates = true;

    if(!m_members.insert(sibling->file().absFilePath()) || m_allowDuplicates) {
        after = new ItemType(sibling->collectionItem(), this);
        setupItem(after);
    }

    m_disableColumnWidthUpdates = false;

    return after;
}

template <class ItemType, class SiblingType>
void Playlist::createItems(const QList<SiblingType *> &siblings, ItemType *after)
{
    if(siblings.isEmpty())
        return;

    foreach(SiblingType *sibling, siblings)
        after = createItem(sibling, after);

    weChanged();
    emit dataChanged(index(m_items.indexOf(siblings.first()), 0), index(m_items.indexOf(siblings.last()), 0));
//     slotWeightDirty();
}

#endif

// vim: set et sw=4 tw=0 sta:
