/***************************************************************************
                          playlist.h  -  description
                             -------------------
    begin                : Sat Feb 16 2002
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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <klistview.h>
#include <kurldrag.h>

#include <qstringlist.h>
#include <qvaluevector.h>
#include <qptrstack.h>

#include "sortedstringlist.h"
#include "playlistsearch.h"
#include "tagguesser.h"

class KPopupMenu;
class KActionMenu;

class QEvent;
class QFileInfo;

class PlaylistSearch;

class PlaylistItem;
typedef QValueList<PlaylistItem *> PlaylistItemList;

typedef QValueList<Playlist *> PlaylistList;

class Playlist : public KListView
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
    virtual void saveAs();
    virtual void clearItem(PlaylistItem *item, bool emitChanged = true);
    virtual void clearItems(const PlaylistItemList &items);

    /**
     * All of the (media) files in the list.
     */
    QStringList files() const;

    /**
     * Returns a list of all of the \e visible items in the playlist.
     */
    virtual PlaylistItemList items();

    /**
     * Returns a list of all of the items in the playlist.
     */
    PlaylistItemList visibleItems() const;

    /**
     * Returns a list of the currently selected items.
     */
    PlaylistItemList selectedItems() const;

    /**
     * Returns a list of the last 10 played items.
     */
    PlaylistItemList historyItems(PlaylistItem *current, bool random) const;

    /**
     * Allow duplicate files in the playlist.
     */
    void setAllowDuplicates(bool allow);

    /**
     * This is being used as a mini-factory of sorts to make the construction
     * of PlaylistItems virtual.  In this case it allows for the creation of
     * both PlaylistItems and CollectionListItems.
     */
    virtual PlaylistItem *createItem(const QFileInfo &file,
				     const QString &absFilePath = QString::null,
				     QListViewItem *after = 0,
				     bool emitChanged = true);

    void createItems(const PlaylistItemList &siblings);

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

    void hideColumn(int c);
    void showColumn(int c);
    bool isColumnVisible(int c) const;

    /**
     * If m_playlistName has no value -- i.e. the name has not been set to
     * something other than the filename, this returns the filename less the
     * extension.  If m_playlistName does have a value, this returns that.
     */
    QString name() const;

    /**
     * This sets a name for the playlist that is \e different from the file name.
     */
    void setName(const QString &n);

    /**
     * Returns the number of items in the playlist.
     */
    int count() const { return childCount(); }

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

public slots:
    /**
     * Remove the currently selected items from the playlist and disk.
     */
    void slotRemoveSelectedItems() { removeFromDisk(selectedItems()); };
    void slotSetNext();

    /*
     * The edit slots are required to use the canonical names so that they are
     * detected by the application wide framework.
     */
    virtual void cut() { copy(); clear(); }
    virtual void copy();
    virtual void paste();
    virtual void clear();
    virtual void selectAll() { KListView::selectAll(true); }

    /**
     * Refreshes the tags of the selection from disk, or all of the files in the
     * list if there is no selection.
     */
    virtual void slotRefresh();

    void slotGuessTagInfo(TagGuesser::Type type);
    void slotRenameFile();

    /**
     * Reload the playlist contents from the m3u file.
     */
    virtual void slotReload();

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
    virtual void polish();

    /**
     * Though it's somewhat obvious, this function will stat the file, so only use it when
     * you're out of a performance critical loop.
     */
    static QString resolveSymLinks(const QFileInfo &file);

    KPopupMenu *rmbMenu() const { return m_rmbMenu; }

signals:
    /**
     * This is emitted when the playlist selection is changed.  This is used
     * primarily to notify the TagEditor of the new data.
     */
    void signalSelectionChanged(const PlaylistItemList &selection);
    void signalDoubleClicked();

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
     * This is emitted when the set of columns that is visible is changed.
     *
     * \see hideColumn()
     * \see showColumn()
     * \see isColumnVisible()
     */
    void signalVisibleColumnsChanged();

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
     * Save the tag for an individual item.
     */
    void applyTag(QListViewItem *item, const QString &text, int column);

    /**
     * Returns the index of the left most visible column in the playlist.
     *
     * \see isColumnVisible()
     */
    int leftMostVisibleColumn() const;

    /**
     * This class is used internally to store settings that are shared by all
     * of the playlists, such as column order.  It is implemented as a singleton.
     */
    class SharedSettings;

private slots:

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
     * This slot applys the tag for a specific item.
     *
     * \see applyTag()
     */
    void slotApplyModification(QListViewItem *, const QString &text, int column);

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

private:
    SortedStringList m_members;
    int m_currentColumn;
    int m_processed;
    bool m_allowDuplicates;
    /**
     * This is used to indicate if the list of visible items has changed (via a 
     * call to setVisibleItems()) while random play is playing.
     */
    static bool m_visibleChanged;
    PlaylistItemList m_history;

    QString m_fileName;

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

    KPopupMenu *m_rmbMenu;
    KPopupMenu *m_headerMenu;
    KActionMenu *m_columnVisibleAction;

    int m_rmbPasteID;
    int m_rmbEditID;

    static PlaylistItem *m_playingItem;
    static int m_leftColumn;

    PlaylistItemList m_randomList;
    PlaylistSearch m_search;

    bool m_polished;
};

QDataStream &operator<<(QDataStream &s, const Playlist &p);
QDataStream &operator>>(QDataStream &s, Playlist &p);

#endif
