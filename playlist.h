/***************************************************************************
                          playlist.h  -  description
                             -------------------
    begin                : Sat Feb 16 2002
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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <klistview.h>
#include <kurldrag.h>
#include <kdebug.h>
#include <kglobalsettings.h>

#include <qvaluevector.h>
#include <qfileinfo.h>

#include "stringhash.h"
#include "playlistsearch.h"
#include "tagguesser.h"
#include "playlistinterface.h"

class KPopupMenu;
class KActionMenu;

class QEvent;

class PlaylistSearch;

class PlaylistItem;
typedef QValueList<PlaylistItem *> PlaylistItemList;

typedef QValueList<Playlist *> PlaylistList;

class Playlist : public KListView /*, PlaylistInterface */
{
    Q_OBJECT

public:

    /**
     * Before creating a playlist directly, please see
     * PlaylistSplitter::createPlaylist().
     */
    Playlist(QWidget *parent, const QString &name = QString::null);

    /**
     * Before creating a playlist directly, please see
     * PlaylistSplitter::openPlaylist().
     */
    Playlist(const QFileInfo &playlistFile, QWidget *parent, const QString &name);

    virtual ~Playlist();

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
     *
     * @see signalCountChanged()
     * @see emitCountChanged()
     */
    virtual void clearItem(PlaylistItem *item, bool emitChanged = true);

    /**
     * Remove \a items from the playlist and emit a signal indicating
     * that the number of items in the list has changed.
     */
    virtual void clearItems(const PlaylistItemList &items);

    /**
     * All of the (media) files in the list.
     */
    QStringList files();

    /**
     * Returns a list of all of the \e visible items in the playlist.
     */
    virtual PlaylistItemList items();

    /**
     * Returns a list of all of the items in the playlist.
     */
    PlaylistItemList visibleItems();

    /**
     * Returns a list of the currently selected items.
     */
    PlaylistItemList selectedItems();

    /**
     * Returns a list of the last 10 played items.
     */
    PlaylistItemList historyItems(PlaylistItem *current, bool random) const;

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

    virtual void createItems(const PlaylistItemList &siblings);

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
     * Hides column \a c.  If \a emitChanged is true then a signal that the
     * visible columns have changed will be emitted and things like the search
     * will be udated.
     */
    void hideColumn(int c, bool emitChanged = true);

    /**
     * Shows column \a c.  If \a emitChanged is true then a signal that the
     * visible columns have changed will be emitted and things like the search
     * will be udated.
     */
    void showColumn(int c, bool emitChanged = true);
    bool isColumnVisible(int c) const;

    /**
     * If m_playlistName has no value -- i.e. the name has not been set to
     * something other than the filename, this returns the filename less the
     * extension.  If m_playlistName does have a value, this returns that.
     */
    virtual QString name() const;

    /**
     * Returns the number of items in the playlist.
     */
    virtual int count() const { return childCount(); }
    
    /**
     * Returns the combined time of all the itens.
     */
    virtual int time();

    /**
     * This sets a name for the playlist that is \e different from the file name.
     */
    void setName(const QString &n);

    /**
     * Returns the next item to be played.  If random is false this is just
     * the next item in the playlist (or null if the current items is the last
     * item in the list).  If random is true, then it will select an item at
     * random from this list (and try to be a bit clever about it to not repeat
     * items before everything has been played at least once).
     */
    PlaylistItem *nextItem(PlaylistItem *current, bool random = false);

    /**
     * Returns the item played before the currently playing item.  If random is
     * false, this is simply the item above the currently playing item in the
     * list.  If random is true this checks the history of recently played items.
     */
    PlaylistItem *previousItem(PlaylistItem *current, bool random = false);

    /**
     * Returns the KActionMenu that allows this to be embedded in menus outside
     * of the playlist.
     */
    KActionMenu *columnVisibleAction() const { return m_columnVisibleAction; }

    /**
     * Set item to be the playing item; also set this list to be the playing list.
     */
    static void setPlaying(PlaylistItem *item, bool p = true);

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
     *
     * \note This does not cause the search to be rerun.
     */
    void setSearch(const PlaylistSearch &s) { m_search = s; }

    /**
     * Emits a signal indicating that the number of items have changed.  This
     * is useful in conjunction with createItem() where emitChanged is false.
     *
     * In many situations it is not practical for speed reasons to trigger the
     * actions associated with signalCountChanged() after each insertion.
     */
    void emitCountChanged() { emit signalCountChanged(this); }

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

    void setColumnWidthUpdatesDisabled(bool disabled) { m_disableColumnWidthUpdates = disabled; }

    /**
     * Playlists have a common set of shared settings such as visible columns
     * that should be applied just before the playlist is shown.  Calling this
     * method applies those.
     */
    void applySharedSettings();

    void emitDataChanged() { emit signalDataChanged(); }

    /**
     * Returns true if full path sort is currently enabled for the file column.
     */
    bool fileColumnFullPathSort() const { return m_fileColumnFullPathSort; }

    /**
     * Reimplemented to add toggling of the file column sorting mode.
     *
     * \see fileColumnFullPathSort()
     */
    virtual void setSorting(int column, bool ascending = true);

public slots:
    /**
     * Remove the currently selected items from the playlist and disk.
     */
    void slotRemoveSelectedItems() { removeFromDisk(selectedItems()); };

    /**
     * Set the first selected item to be the next item returned by nextItem().
     */
    void slotSetNext();

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
     * Reload the playlist contents from the m3u file.
     */
    virtual void slotReload();

    /**
     * Tells the listview that the next time that it paints that the weighted
     * column widths must be recalculated.  If this is called without a column
     * all visible columns are marked as dirty.
     */
    void slotWeightDirty(int column = -1);

protected:
    /**
     * Remove \a items from the playlist and disk.  This will ignore items that
     * are not actually in the list.
     */
    void removeFromDisk(const PlaylistItemList &items);

    // the following are all reimplemented from base classes

    virtual bool eventFilter(QObject *watched, QEvent *e);
    virtual QDragObject *dragObject(QWidget *parent);
    virtual QDragObject *dragObject() { return dragObject(this); }
    virtual bool canDecode(QMimeSource *s);
    virtual void decode(QMimeSource *s, PlaylistItem *after = 0);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void showEvent(QShowEvent *e);
    virtual bool acceptDrag(QDropEvent *e) const { return KURLDrag::canDecode(e); }
    virtual void viewportPaintEvent(QPaintEvent *pe);
    virtual void viewportResizeEvent(QResizeEvent *re);

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
     * As a template this allows us to use the same code to initialize the items
     * in subclasses.  CollectionItemType should always be CollectionListItem and
     * ItemType should be a PlaylistItem subclass.
     */
    template <class CollectionItemType, class ItemType, class SiblingType>
    void createItems(const QValueList<SiblingType *> &siblings);

signals:
    /**
     * This is emitted when the playlist selection is changed.  This is used
     * primarily to notify the TagEditor of the new data.
     */
    void signalSelectionChanged(const PlaylistItemList &selection);

    /**
     * This is connected to the PlaylistBox::Item to let it know when the
     * playlist's name has changed.
     */
    void signalNameChanged(const QString &fileName);

    /**
     * This signal is emitted when items are added to or removed from the list.
     *
     * \see signalDataChanged()
     * \see signalChanged()
     */
    void signalCountChanged(Playlist *);

    /**
     * This signal is connected to PlaylistItem::refreshed() in the PlaylistItem
     * class.  It is emitted when a playlist item's data has been changed.
     *
     * \see signalCountChanged()
     * \see signalChanged()
     */
    void signalDataChanged();

    /**
     * This is the union of signalDataChanged() and signalCountChanged().
     * It is emitted with either quantity or value of the PlaylistItems are
     * changed.
     */
    void signalChanged();

    /**
     * This signal is emitted just before a playlist item is removed from the
     * list allowing for any cleanup that needs to happen.  Typically this
     * is used to remove the item from the history and safeguard against
     * dangling pointers.
     */
    void signalAboutToRemove(PlaylistItem *item);

    /**
     * This is emitted when \a files are dropped on a specific playlist.
     */
    void signalFilesDropped(const QStringList &files, Playlist *, PlaylistItem *after);

    /**
     * Set the next item to be played in the current playlist.  This is used by
     * the "Play Next" feature.
     */
    void signalSetNext(PlaylistItem *item);

    /**
     * Request creation of a playlist based on \a items.
     */
    void signalCreatePlaylist(const PlaylistItemList &items);

private:
    void setup();

    /**
     * Load the playlist from a file.  \a fileName should be the absolute path.
     * \a fileInfo should point to the same file as \a fileName.  This is a
     * little awkward API-wise, but keeps us from throwing away useful
     * information.
     */
    void loadFile(const QString &fileName, const QFileInfo &fileInfo);

    /**
     * Writes \a text to \a item in \a column.  This is used by the inline tag
     * editor.
     */
    void editTag(PlaylistItem *item, const QString &text, int column);

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

    /**
     * This class is used internally to store settings that are shared by all
     * of the playlists, such as column order.  It is implemented as a singleton.
     */
    class SharedSettings;

private slots:

    void slotUpdateColumnWidths();

    /**
     * This is just used to emit the selection as a list of PlaylistItems when
     * the selection changes.
     */
    void slotEmitSelected() { emit signalSelectionChanged(selectedItems()); }

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
    void slotCreateGroup() { emit signalCreatePlaylist(selectedItems()); }

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

private:
    StringHash m_members;

    int m_currentColumn;
    int m_processed;

    int m_rmbPasteID;
    int m_rmbEditID;

    int m_selectedCount;

    bool m_allowDuplicates;
    bool m_polished;
    bool m_applySharedSettings;
    bool m_fileColumnFullPathSort;

    QValueList<int> m_weightDirty;
    bool m_disableColumnWidthUpdates;
    /**
     * The average minimum widths of columns to be used in balancing calculations.
     */
    QValueVector<int> m_columnWeights;
    QValueVector<int> m_columnFixedWidths;
    bool m_widthsDirty;

    PlaylistItemList m_randomList;
    PlaylistItemList m_history;
    PlaylistSearch m_search;

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

    /**
     * This is used to indicate if the list of visible items has changed (via a 
     * call to setVisibleItems()) while random play is playing.
     */
    static bool m_visibleChanged;
    static int m_leftColumn;
    static PlaylistItem *m_playingItem;
};

QDataStream &operator<<(QDataStream &s, Playlist &p);
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

        if(!m_randomList.isEmpty() && !m_visibleChanged)
            m_randomList.append(i);

	emit signalCountChanged(this);

	if(emitChanged)
	    emit signalCountChanged(this);

	return i;
    }
    else
	return 0;
}

template <class CollectionItemType, class ItemType, class SiblingType>
void Playlist::createItems(const QValueList<SiblingType *> &siblings)
{
    if(siblings.isEmpty())
	return;

    m_disableColumnWidthUpdates = true;
    ItemType *newItem = 0;

    QValueListConstIterator<SiblingType *> it = siblings.begin();
    for(; it != siblings.end(); ++it) {
	if(!m_members.insert((*it)->file().absFilePath()) || m_allowDuplicates) {
	    newItem = new ItemType((*it)->collectionItem(), this, newItem);
	    setupItem(newItem);
	    if(!m_randomList.isEmpty() && !m_visibleChanged)
		m_randomList.append(newItem);
	}
    }

    emit signalCountChanged(this);
    m_disableColumnWidthUpdates = false;
    slotWeightDirty();
}

#endif
