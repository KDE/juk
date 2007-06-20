/***************************************************************************
    begin                : Sat Feb 16 2002
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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <klistview.h>
#include <kurldrag.h>
#include <kdebug.h>
#include <kglobalsettings.h>

#include <qvaluevector.h>
#include <qfileinfo.h>

#include "covermanager.h"
#include "stringhash.h"
#include "playlistsearch.h"
#include "tagguesser.h"
#include "playlistinterface.h"
#include "playlistitem.h"

class KPopupMenu;
class KActionMenu;

class QEvent;

class PlaylistCollection;
class WebImageFetcher;
class PlaylistToolTip;
class UpcomingPlaylist;

typedef QValueList<PlaylistItem *> PlaylistItemList;

class Playlist : public KListView, public PlaylistInterface
{
    Q_OBJECT

public:

    Playlist(PlaylistCollection *collection, const QString &name = QString::null,
	     const QString &iconName = "midi");
    Playlist(PlaylistCollection *collection, const PlaylistItemList &items,
	     const QString &name = QString::null, const QString &iconName = "midi");
    Playlist(PlaylistCollection *collection, const QFileInfo &playlistFile,
	     const QString &iconName = "midi");

    /**
     * This constructor should generally only be used either by the cache
     * restoration methods or by subclasses that want to handle calls to
     * PlaylistCollection::setupPlaylist() differently.
     */
    Playlist(PlaylistCollection *collection, bool delaySetup);

    virtual ~Playlist();


    // The following group of functions implement the PlaylistInterface API.

    virtual QString name() const;
    virtual FileHandle currentFile() const;
    virtual int count() const { return childCount(); }
    virtual int time() const;
    virtual void playNext();
    virtual void playPrevious();
    virtual void stop();

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
				     QListViewItem *after = 0,
				     bool emitChanged = true);

    /**
     * This is implemented as a template method to allow subclasses to
     * instantiate their PlaylistItem subclasses using the same method.  Some
     * of the types here are artificially templatized (i.e. CollectionListType and
     * CollectionItemType) to avoid recursive includes, but in fact will always
     * be the same.
     */
    template <class ItemType, class CollectionItemType, class CollectionListType>
    ItemType *createItem(const FileHandle &file,
			 QListViewItem *after = 0,
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
     * QString::null if no such file exists.
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
     * will be udated.
     */
    void hideColumn(int c, bool updateSearch = true);

    /**
     * Shows column \a c.  If \a updateSearch is true then a signal that the
     * visible columns have changed will be emitted and things like the search
     * will be udated.
     */
    void showColumn(int c, bool updateSearch = true);
    bool isColumnVisible(int c) const;

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
    static void setPlaying(PlaylistItem *item, bool addToHistory = true);

    /**
     * Returns true if this playlist is currently playing.
     */
    bool playing() const;

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
    static void setItemsVisible(const PlaylistItemList &items, bool visible = true);

    /**
     * Returns the search associated with this list, or an empty search if one
     * has not yet been set.
     */
    PlaylistSearch search() const { return m_search; }

    /**
     * Set the search associtated with this playlist.
     */
    void setSearch(const PlaylistSearch &s);

    /**
     * If the search is disabled then all items will be shown, not just those that
     * match the current search.
     */
    void setSearchEnabled(bool searchEnabled);

    /**
     * Marks \a item as either selected or deselected based.
     */
    void markItemSelected(PlaylistItem *item, bool selected);

    /**
     * Subclasses of Playlist which add new columns will set this value to
     * specify how many of those colums exist.  This allows the Playlist
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
    virtual bool canReload() const { return !m_fileName.isNull(); }

    /**
     * Returns true if the playlist is a search playlist and the search should be
     * editable.
     */
    virtual bool searchIsEditable() const { return false; }

    /**
     * Synchronizes the the playing item in this playlist with the playing item
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

public slots:
    /**
     * Remove the currently selected items from the playlist and disk.
     */
    void slotRemoveSelectedItems() { removeFromDisk(selectedItems()); };

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
    virtual void selectAll() { KListView::selectAll(true); }

    /**
     * Refreshes the tags of the selection from disk, or all of the files in the
     * list if there is no selection.
     */
    virtual void slotRefresh();

    void slotGuessTagInfo(TagGuesser::Type type);

    /**
     * Renames the selected items' files based on their tags contents.
     *
     * @see PlaylistItem::renameFile()
     */
    void slotRenameFile();

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
     * Tells the listview that the next time that it paints that the weighted
     * column widths must be recalculated.  If this is called without a column
     * all visible columns are marked as dirty.
     */
    void slotWeightDirty(int column = -1);

    void slotShowPlaying();

    void slotColumnResizeModeChanged();

    virtual void dataChanged();

protected:
    /**
     * Remove \a items from the playlist and disk.  This will ignore items that
     * are not actually in the list.
     */
    void removeFromDisk(const PlaylistItemList &items);

    // the following are all reimplemented from base classes

    virtual bool eventFilter(QObject *watched, QEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual QDragObject *dragObject(QWidget *parent);
    virtual QDragObject *dragObject() { return dragObject(this); }
    virtual bool canDecode(QMimeSource *s);
    virtual void decode(QMimeSource *s, PlaylistItem *item = 0);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent *e);
    virtual void contentsDragEnterEvent(QDragEnterEvent *e);
    virtual void showEvent(QShowEvent *e);
    virtual bool acceptDrag(QDropEvent *e) const;
    virtual void viewportPaintEvent(QPaintEvent *pe);
    virtual void viewportResizeEvent(QResizeEvent *re);

    virtual void insertItem(QListViewItem *item);
    virtual void takeItem(QListViewItem *item);

    virtual bool hasItem(const QString &file) const { return m_members.contains(file); }

    void addColumn(const QString &label);

    /**
     * Here I'm using delayed setup of some things that aren't quite intuitive.
     * Creating columns and setting up connections are both time consuming if
     * there are a lot of playlists to initialize.  This moves that cost from the
     * startup time to the time when the widget is "polished" -- i.e. just before
     * it's painted the first time.
     */
    virtual void polish();

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
     * in subclasses.  CollectionItemType should always be CollectionListItem and
     * ItemType should be a PlaylistItem subclass.
     */
    template <class CollectionItemType, class ItemType, class SiblingType>
    void createItems(const QValueList<SiblingType *> &siblings, ItemType *after = 0);

protected slots:
    void slotPopulateBackMenu() const;
    void slotPlayFromBackMenu(int number) const;

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

private:
    void setup();

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
     * \see isColumnVisible()
     */
    int leftMostVisibleColumn() const;

    /**
     * This method is used internally to provide the backend to the other item
     * lists.
     *
     * \see items()
     * \see visibleItems()
     * \see selectedItems()
     */
    PlaylistItemList items(QListViewItemIterator::IteratorFlag flags);

    /**
     * Build the column "weights" for the weighted width mode.
     */
    void calculateColumnWeights();

    void addFile(const QString &file, FileHandleList &files, bool importPlaylists,
		 PlaylistItem **after);
    void addFileHelper(FileHandleList &files, PlaylistItem **after,
		       bool ignoreTimer = false);

    void redisplaySearch() { setSearch(m_search); }

    /**
     * Sets the cover for items to the cover identified by id.
     */
    void refreshAlbums(const PlaylistItemList &items, coverKey id = CoverManager::NoMatch);

    void refreshAlbum(const QString &artist, const QString &album);

    /**
     * Returns the number of PlaylistItems in @p items that can be assigned a
     * cover.  Used to avoid wasting the users' time setting the cover for 20
     * items when none are eligible.
     */
    unsigned int eligibleCoverItems(const PlaylistItemList &items);

    void updatePlaying() const;

    /**
     * This class is used internally to store settings that are shared by all
     * of the playlists, such as column order.  It is implemented as a singleton.
     */
    class SharedSettings;

private slots:

    void slotUpdateColumnWidths();

    void slotAddToUpcoming();

    /**
     * Show the RMB menu.  Matches the signature for the signal 
     * QListView::contextMenuRequested().
     */
    void slotShowRMBMenu(QListViewItem *item, const QPoint &point, int column);

    /**
     * This slot is called when the inline tag editor has completed its editing
     * and starts the process of renaming the values.
     *
     * \see editTag()
     */
    void slotInlineEditDone(QListViewItem *, const QString &, int column);

    /**
     * This starts the renaming process by displaying a line edit if the mouse is in 
     * an appropriate position.
     */
    void slotRenameTag();

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
    void slotToggleColumnVisible(int column);

    /**
     * Prompts the user to create a new playlist with from the selected items.
     */
    void slotCreateGroup();

    /**
     * This slot is called when the user drags the slider in the listview header
     * to manually set the size of the column.
     */
    void slotColumnSizeChanged(int column, int oldSize, int newSize);

    /**
     * The slot is called when the completion mode for the line edit in the
     * inline tag editor is changed.  It saves the settings and through the
     * magic of the SharedSettings class will apply it to the other playlists as
     * well.
     */
    void slotInlineCompletionModeChanged(KGlobalSettings::Completion mode);

    void slotPlayCurrent();

private:
    PlaylistCollection *m_collection;

    StringHash m_members;

    WebImageFetcher *m_fetcher;

    int m_currentColumn;
    int m_processed;
    int m_rmbEditID;
    int m_rmbUpcomingID;
    int m_selectedCount;

    bool m_allowDuplicates;
    bool m_polished;
    bool m_applySharedSettings;
    bool m_columnWidthModeChanged;

    QValueList<int> m_weightDirty;
    bool m_disableColumnWidthUpdates;

    mutable int m_time;
    mutable PlaylistItemList m_addTime;
    mutable PlaylistItemList m_subtractTime;

    /**
     * The average minimum widths of columns to be used in balancing calculations.
     */
    QValueVector<int> m_columnWeights;
    QValueVector<int> m_columnFixedWidths;
    bool m_widthsDirty;

    static PlaylistItemList m_history;
    PlaylistSearch m_search;

    bool m_searchEnabled;

    PlaylistItem *m_lastSelected;

    /**
     * Used to store the text for inline editing before it is changed so that
     * we can know if something actually changed and as such if we need to save
     * the tag.
     */
    QString m_editText;

    /**
     * This is only defined if the playlist name is something other than the
     * file name.
     */
    QString m_playlistName;
    QString m_fileName;

    KPopupMenu *m_rmbMenu;
    KPopupMenu *m_headerMenu;
    KActionMenu *m_columnVisibleAction;
    PlaylistToolTip *m_toolTip;

    /**
     * This is used to indicate if the list of visible items has changed (via a
     * call to setVisibleItems()) while random play is playing.
     */
    static bool m_visibleChanged;
    static bool m_shuttingDown;
    static int m_leftColumn;
    static QMap<int, PlaylistItem *> m_backMenuItems;

    bool m_blockDataChanged;
};

bool processEvents();

class FocusUpEvent : public QCustomEvent
{
public:
    FocusUpEvent() : QCustomEvent(id) {}
    static const int id = 999;
};

QDataStream &operator<<(QDataStream &s, const Playlist &p);
QDataStream &operator>>(QDataStream &s, Playlist &p);

// template method implementations

template <class ItemType, class CollectionItemType, class CollectionListType>
ItemType *Playlist::createItem(const FileHandle &file, QListViewItem *after,
			       bool emitChanged)
{
    CollectionItemType *item = CollectionListType::instance()->lookup(file.absFilePath());

    if(!item) {
	item = new CollectionItemType(file);
	setupItem(item);

	// If a valid tag was not created, destroy the CollectionListItem.

	if(!item->isValid()) {
	    kdError(65432) << "Playlist::createItem() -- A valid tag was not created for \""
			   << file.absFilePath() << "\"" << endl;
	    delete item;
	    return 0;
	}
    }

    if(item && !m_members.insert(file.absFilePath()) || m_allowDuplicates) {

	ItemType *i = after ? new ItemType(item, this, after) : new ItemType(item, this);
	setupItem(i);

	if(emitChanged)
	    dataChanged();

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
	after = new ItemType(sibling->collectionItem(), this, after);
	setupItem(after);
    }

    m_disableColumnWidthUpdates = false;

    return after;
}

template <class CollectionItemType, class ItemType, class SiblingType>
void Playlist::createItems(const QValueList<SiblingType *> &siblings, ItemType *after)
{
    if(siblings.isEmpty())
	return;

    QValueListConstIterator<SiblingType *> it = siblings.begin();
    for(; it != siblings.end(); ++it)
	after = createItem(*it, after);

    dataChanged();
    slotWeightDirty();
}

#endif
