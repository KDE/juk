/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2007 Michael Pyne <mpyne@kde.org>
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

#ifndef JUK_PLAYLIST_H
#define JUK_PLAYLIST_H

#include <QVector>
#include <QEvent>
#include <QList>
#include <QTreeWidget>
#include <QFuture>

#include "covermanager.h"
#include "stringhash.h"
#include "playlistsearch.h"
#include "tagguesser.h"
#include "playlistinterface.h"
#include "filehandle.h"
#include "juk_debug.h"

class KActionMenu;

class QAction;
class QFileInfo;
class QMimeData;
class QTimer;

class WebImageFetcher;
class PlaylistItem;
class PlaylistCollection;
class CollectionListItem;

typedef QVector<PlaylistItem *> PlaylistItemList;

class Playlist : public QTreeWidget, public PlaylistInterface
{
    Q_OBJECT

public:

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


    // The following group of functions implement the PlaylistInterface API.

    virtual QString name() const override;
    virtual FileHandle currentFile() const override;
    virtual int count() const override { return model()->rowCount(); }
    virtual int time() const override { return m_time; }
    virtual void playNext() override;
    virtual void playPrevious() override;
    virtual void stop() override;

    /**
     * Plays the top item of the playlist.
     */
    void playFirst();

    /**
     * Plays the next album in the playlist.  Only useful when in album random
     * play mode.
     */
    void playNextAlbum();

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
     * Removes \a item from the Playlist, but not from the disk.
     *
     * Since the GUI updates after an item is cleared, you should use clearItems() if you have
     * a list of items to remove, as that will remove the whole batch before updating
     * other components/GUI to the change.
     */
    virtual void clearItem(PlaylistItem *item);

    /**
     * Remove \a items from the playlist and emit a signal indicating
     * that the number of items in the list has changed.
     */
    virtual void clearItems(const PlaylistItemList &items);

    /**
     * Accessor function to return a pointer to the currently playing file.
     *
     * @return 0 if no file is playing, otherwise a pointer to the PlaylistItem
     *     of the track that is currently playing.
     */
    static PlaylistItem *playingItem();

    /**
     * All of the (media) files in the list.
     */
    QStringList files() const;

    /**
     * Returns a list of all of the items in the playlist.
     */
    virtual PlaylistItemList items();

    /**
     * Returns a list of all of the \e visible items in the playlist.
     */
    PlaylistItemList visibleItems();

    /**
     * Returns a list of the currently selected items.
     */
    PlaylistItemList selectedItems();

    /**
     * Returns properly casted first child item in list.
     */
    PlaylistItem *firstChild() const;

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
                                     QTreeWidgetItem *after = nullptr);

    /**
     * This is implemented as a template method to allow subclasses to
     * instantiate their PlaylistItem subclasses using the same method.
     */
    template <class ItemType>
    ItemType *createItem(const FileHandle &file,
                         QTreeWidgetItem *after = nullptr);

    virtual void createItems(const PlaylistItemList &siblings, PlaylistItem *after = nullptr);

    /**
     * This handles adding files of various types -- music, playlist or directory
     * files.  Music files that are found will be added to this playlist.  New
     * playlist files that are found will result in new playlists being created.
     *
     * Note that this should not be used in the case of adding *only* playlist
     * items since it has the overhead of checking to see if the file is a playlist
     * or directory first.
     */
    virtual void addFiles(const QStringList &files, PlaylistItem *after = nullptr);

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
     * Hides column \a c.  If \a updateSearch is true then a signal that the
     * visible columns have changed will be emitted and things like the search
     * will be updated.
     */
    void hideColumn(int c, bool updateSearch = true);

    /**
     * Shows column \a c.  If \a updateSearch is true then a signal that the
     * visible columns have changed will be emitted and things like the search
     * will be updated.
     */
    void showColumn(int c, bool updateSearch = true);

    void sortByColumn(int column, Qt::SortOrder order = Qt::AscendingOrder);

    /**
     * This sets a name for the playlist that is \e different from the file name.
     */
    void setName(const QString &n);

    /**
     * Returns the KActionMenu that allows this to be embedded in menus outside
     * of the playlist.
     */
    KActionMenu *columnVisibleAction() const { return m_columnVisibleAction; }

    /**
     * Set item to be the playing item.  If \a item is null then this will clear
     * the playing indicator.
     */
    void setPlaying(PlaylistItem *item, bool addToHistory = true);

    /**
     * Returns true if this playlist is currently playing.
     */
    bool playing() const override;

    /**
     * This forces an update of the left most visible column, but does not save
     * the settings for this.
     */
    void updateLeftColumn();

    /**
     * Returns the leftmost visible column of the listview.
     */
    int leftColumn() const { return m_leftColumn; }

    /**
     * Sets the items in the list to be either visible based on the value of
     * visible.  This is useful for search operations and such.
     */
    void setItemsVisible(const QModelIndexList &indexes, bool visible = true);

    /**
     * Returns the search associated with this list, or an empty search if one
     * has not yet been set.
     */
    PlaylistSearch* search() const { return m_search; }

    /**
     * Set the search associated with this playlist.
     */
    void setSearch(PlaylistSearch* s);

    /**
     * If the search is disabled then all items will be shown, not just those that
     * match the current search.
     */
    void setSearchEnabled(bool searchEnabled);

    /**
     * Subclasses of Playlist which add new columns will set this value to
     * specify how many of those columns exist.  This allows the Playlist
     * class to do some internal calculations on the number and positions
     * of columns.
     */
    virtual int columnOffset() const { return 0; }

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
     * Returns true if the playlist is a search playlist and the search should be
     * editable.
     */
    virtual bool searchIsEditable() const { return false; }

    /**
     * Synchronizes the playing item in this playlist with the playing item
     * in \a playlist.  If \a setMaster is true, this list will become the source
     * for determining the next item.
     */
    void synchronizePlayingItems(Playlist *playlist, bool setMaster);

    /**
     * Synchronizes the playing item in this playlist with the playing item
     * in \a sources.  If \a setMaster is true, this list will become the source
     * for determining the next item.
     */
    void synchronizePlayingItems(const PlaylistList &sources, bool setMaster);

    /**
     * Playlists have a common set of shared settings such as visible columns
     * that should be applied just before the playlist is shown.  Calling this
     * method applies those.
     */
    void applySharedSettings();

    void read(QDataStream &s);

    static void setShuttingDown() { m_shuttingDown = true; }

    void playlistItemsChanged() override;

public slots:
    /**
     * Remove the currently selected items from the playlist and disk.
     */
    void slotRemoveSelectedItems() { removeFromDisk(selectedItems()); }

    /*
     * The edit slots are required to use the canonical names so that they are
     * detected by the application wide framework.
     */
    virtual void cut() { copy(); clear(); }

    /**
     * Puts a list of URLs pointing to the files in the current selection on the
     * clipboard.
     */
    virtual void copy();

    /**
     * Checks the clipboard for local URLs to be inserted into this playlist.
     */
    virtual void paste();

    /**
     * Removes the selected items from the list, but not the disk.
     *
     * @see clearItem()
     * @see clearItems()
     */
    virtual void clear();

    /**
     * Refreshes the tags of the selection from disk, or all of the files in the
     * list if there is no selection.
     */
    virtual void slotRefresh();

    /**
     * Opens the containing folder of the selected files.
     */
    virtual void slotOpenItemDir();

    void slotGuessTagInfo(TagGuesser::Type type);

    /**
     * Renames the selected items' files based on their tags contents.
     *
     * @see PlaylistItem::renameFile()
     */
    void slotRenameFile();

    /**
     * Select a track to play after being stopped.
     *
     * @see playNext()
     */
    void slotBeginPlayback();

    /**
     * Sets the cover of the selected items, pass in true if you want to load from the local system,
     * false if you want to load from the internet.
     */
    void slotAddCover(bool fromLocal);

    /**
     * Shows a large image of the cover
     */
    void slotViewCover();

    /**
     * Removes covers from the selected items
     */
    void slotRemoveCover();

    /**
     * Shows the cover manager GUI dialog
     */
    void slotShowCoverManager();

    /**
     * Reload the playlist contents from the m3u file.
     */
    virtual void slotReload();

    /**
     * Ensures the random sequence of playlist items is built. Ignored if we're
     * not in random playback mode so it is safe to call in any mode.
     */
    void refillRandomList();

    /**
     * Tells the listview that the next time that it paints that the weighted
     * column widths must be recalculated.  If this is called without a column
     * all visible columns are marked as dirty.
     */
    void slotWeightDirty(int column = -1);

    void slotShowPlaying();

    void slotColumnResizeModeChanged();

protected:
    /**
     * Remove \a items from the playlist and disk.  This will ignore items that
     * are not actually in the list.
     */
    void removeFromDisk(const PlaylistItemList &items);

    /**
     * Adds and removes items from this Playlist as necessary to ensure that
     * the same items are present in this Playlist as in @p itemList.
     *
     * No ordering guarantees are imposed, just that the playlist will have the
     * same items as in the given list afterwards.
     */
    void synchronizeItemsTo(const PlaylistItemList &itemList);

    /**
     * Completes the actions with the parent PlaylistCollection needed to
     * actually start playing back the given PlaylistItem.
     */
    virtual void beginPlayingItem(PlaylistItem *itemToPlay);

    // the following are all reimplemented from base classes

    virtual bool eventFilter(QObject *watched, QEvent *e) override;
    virtual void keyPressEvent(QKeyEvent *e) override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QList<QTreeWidgetItem *> items) const override;
    virtual bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action) override;
    virtual void dropEvent(QDropEvent *e) override;
    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;
    virtual void paintEvent(QPaintEvent *pe) override;
    virtual void resizeEvent(QResizeEvent *re) override;

    virtual void drawRow(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    virtual void insertItem(QTreeWidgetItem *item);
    virtual void takeItem(QTreeWidgetItem *item);

    virtual bool hasItem(const QString &file) const { return m_members.contains(file); }

    /**
     * Do some final initialization of created items.  Notably ensure that they
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
    ItemType *createItem(SiblingType *sibling, ItemType *after = nullptr);

    /**
     * As a template this allows us to use the same code to initialize the items
     * in subclasses. ItemType should be a PlaylistItem subclass.
     */
    template <template <typename> class Container, class ItemType, class SiblingType>
    void createItems(const Container<SiblingType *> &siblings, ItemType *after = nullptr);

protected slots:
    void slotPopulateBackMenu() const;
    void slotPlayFromBackMenu(QAction *);

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

    void signalPlaylistItemsDropped(Playlist *p);

    void signalMoveFocusAway();

private:
    // Common constructor routines, inherited by other constructors
    Playlist(
            bool delaySetup, const QString &name,
            PlaylistCollection *collection, const QString &iconName,
            int extraCols);

    void setup(int numColumnsToReserve);

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

    /**
     * Writes \a text to \a item in \a column.  This is used by the inline tag
     * editor.  Returns false if the tag update failed.
     */
    bool editTag(PlaylistItem *item, const QString &text, int column);

    /**
     * Returns the index of the left most visible column in the playlist.
     *
     * \see isColumnHidden()
     */
    int leftMostVisibleColumn() const;

    /// Creates the context menu on demand
    void createPlaylistRMBMenu();

    /**
     * This method is used internally to provide the backend to the other item
     * lists.
     *
     * \see items()
     * \see visibleItems()
     * \see selectedItems()
     */
    PlaylistItemList items(QTreeWidgetItemIterator::IteratorFlags flags);

    /**
     * Build the column "weights" for the weighted width mode.
     */
    void calculateColumnWeights();

    void addPlaylistFile(const QString &m3uFile);
    QFuture<void> addFilesFromDirectory(const QString &dirPath);
    QFuture<void> addUntypedFile(const QString &file, PlaylistItem *after = nullptr);
    void cleanupAfterAllFileLoadsCompleted();
    void addFilesFromMimeData(const QMimeData *urls, PlaylistItem *after = nullptr);

    void redisplaySearch() { setSearch(m_search); }

    /**
     * Sets the cover for items to the cover identified by id.
     */
    void refreshAlbums(const PlaylistItemList &items, coverKey id = CoverManager::NoMatch);

    void refreshAlbum(const QString &artist, const QString &album);

    void updatePlaying() const;

    /**
     * This function should be called when item is deleted to ensure that any
     * internal bookkeeping is performed.  It is automatically called by
     * PlaylistItem::~PlaylistItem and by clearItem() and clearItems().
     */
    void updateDeletedItem(PlaylistItem *item);

    /**
     * Used as a helper to implement template<> createItem().  This grabs the
     * CollectionListItem for file if it exists, otherwise it creates a new one and
     * returns that.  If nullptr is returned then some kind of error occurred,
     * and you should probably do nothing with the FileHandle you have.
     */
    CollectionListItem *collectionListItem(const FileHandle &file);

    /**
     * This class is used internally to store settings that are shared by all
     * of the playlists, such as column order.  It is implemented as a singleton.
     */
    class SharedSettings;

private slots:

    /**
     * Handle the necessary tasks needed to create and setup the playlist that
     * don't need to happen in the ctor, such as setting up the columns,
     * initializing the RMB menu, and setting up signal/slot connections.
     *
     * Used to be a subclass of K3ListView::polish() but the timing of the
     * call is not consistent and therefore lead to crashes.
     */
    void slotInitialize(int numColumnsToReserve);

    void slotUpdateColumnWidths();

    void slotAddToUpcoming();

    /**
     * Show the RMB menu.  Matches the signature for the signal
     * QListView::contextMenuRequested().
     */
    void slotShowRMBMenu(const QPoint &point);

    /**
     * This slot is called when the inline tag editor has completed its editing
     * and starts the process of renaming the values.
     */
    void slotInlineEditDone(QTreeWidgetItem *, int column);

    /**
     * The image fetcher will update the cover asynchronously, this internal
     * slot is called when it happens.
     */
    void slotCoverChanged(int coverId);

    /**
     * Moves the column \a from to the position \a to.  This matches the signature
     * for the signal QHeader::indexChange().
     */
    void slotColumnOrderChanged(int, int from, int to);

    /**
     * Toggles a columns visible status.  Useful for KActions.
     *
     * \see hideColumn()
     * \see showColumn()
     */
    void slotToggleColumnVisible(QAction *action);

    /**
     * Prompts the user to create a new playlist with from the selected items.
     */
    void slotCreateGroup();

    /**
     * This slot is called when the user drags the slider in the listview header
     * to manually set the size of the column.
     */
    void columnResized(int column, int oldSize, int newSize);

    void slotPlayCurrent();
    void slotUpdateTime();

private:
    friend class PlaylistItem;

    PlaylistCollection *m_collection = nullptr;
    StringHash m_members;

    // This is only defined if the playlist name is something other than the
    // file name.
    QString m_playlistName;
    QString m_fileName;

    int  m_time            = 0;
    bool m_allowDuplicates = true;

    /**
     * The average minimum widths of columns to be used in balancing calculations.
     */
    QVector<int> m_columnWeights;
    QVector<int> m_columnFixedWidths;
    QVector<int> m_weightDirty;
    KActionMenu *m_columnVisibleAction = nullptr;
    bool m_columnWidthModeChanged      = false;
    bool m_disableColumnWidthUpdates   = true;
    bool m_widthsDirty                 = true;
    bool m_applySharedSettings         = true;

    /// Used for random play and album random play
    PlaylistItemList m_randomSequence;
    QTimer          *m_refillDebounce;

    PlaylistSearch* m_search;
    bool m_searchEnabled = true;

    int  m_itemsLoading = 0; /// Count of pending file loads outstanding
    bool m_blockDataChanged = false;

    QAction *m_rmbEdit  = nullptr;
    QMenu *m_rmbMenu    = nullptr;
    QMenu *m_headerMenu = nullptr;
    WebImageFetcher *m_fetcher = nullptr;

    /**
     * This is used to indicate if the list of visible items has changed (via a
     * call to setVisibleItems()) while random play is playing.
     */
    static bool m_visibleChanged;
    static PlaylistItemList m_history;
    static bool m_shuttingDown;
    static int m_leftColumn;
    static QVector<PlaylistItem *> m_backMenuItems;
};

typedef QVector<Playlist *> PlaylistList;

bool processEvents();

QDataStream &operator<<(QDataStream &s, const Playlist &p);
QDataStream &operator>>(QDataStream &s, Playlist &p);

// template method implementations

template <class ItemType>
ItemType *Playlist::createItem(const FileHandle &file, QTreeWidgetItem *after)
{
    CollectionListItem *item = collectionListItem(file);
    if(item && (!m_members.insert(file.absFilePath()) || m_allowDuplicates)) {
        auto i = new ItemType(item, this, after);
        setupItem(i);
        return i;
    }
    else
        return nullptr;
}

template <class ItemType, class SiblingType>
ItemType *Playlist::createItem(SiblingType *sibling, ItemType *after)
{
    m_disableColumnWidthUpdates = true;

    if(!m_members.insert(sibling->file().absFilePath()) || m_allowDuplicates) {
        after = new ItemType(sibling->collectionItem(), this, after);
        setupItem(after);
    }

    m_disableColumnWidthUpdates = false;

    return after;
}

template <template <typename> class Container, class ItemType, class SiblingType>
void Playlist::createItems(const Container<SiblingType *> &siblings, ItemType *after)
{
    if(siblings.isEmpty())
        return;

    foreach(SiblingType *sibling, siblings)
        after = createItem(sibling, after);

    playlistItemsChanged();
    slotWeightDirty();
}

#endif

// vim: set et sw=4 tw=0 sta:
