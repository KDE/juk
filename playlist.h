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

#include <qstringlist.h>
#include <qvaluevector.h>
#include <qptrstack.h>

#include "playlistitem.h"
#include "sortedstringlist.h"

class KPopupMenu;

class KPopupMenu;
class KActionMenu;

class QEvent;

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
    Playlist(const QFileInfo &playlistFile, QWidget *parent, const char *name = 0);

    virtual ~Playlist();

    /**
     * Saves the file to the currently set file name.  If there is no filename
     * currently set, the default behavior is to prompt the user for a file
     * name.
     */
    virtual void save();
    virtual void saveAs();
    virtual void refresh();
    virtual void clearItem(PlaylistItem *item, bool emitChanged = true);
    virtual void clearItems(const PlaylistItemList &items);

    /**
     * All of the (media) files in the list.
     */
    QStringList files() const;

    /**
     * Returns a list of all of the \e visible items in the playlist.
     */
    PlaylistItemList items();

    /**
     * Returns a list of all of the items in the playlist.
     */
    PlaylistItemList visibleItems() const;

    /**
     * Returns a list of the currently selected items.
     */
    PlaylistItemList selectedItems() const;

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
				     QListViewItem *after = 0);

    void createItems(const PlaylistItemList &siblings);

    QString fileName() const { return m_fileName; }
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

    int count() const { return childCount(); }

    /**
     * This gets the next item to be played.  This is static because often we
     * know about the playing item, but not to which list it belongs.
     */
    PlaylistItem *nextItem(PlaylistItem *current, bool random = false);
    PlaylistItem *previousItem(PlaylistItem *current, bool random = false);

    KActionMenu *columnVisibleAction() const { return m_columnVisibleAction; }

    void setPlaying(PlaylistItem *item, bool playing = true);

    /**
     * This forces an update of the left most visible column, but does not save
     * the settings for this.
     */
    void updateLeftColumn();

    static void setItemsVisible(const PlaylistItemList &items, bool visible = true);

public slots:
    /**
     * Remove the currently selected items from the playlist and disk.
     */
    void slotDeleteSelectedItems() { deleteFromDisk(selectedItems()); };
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

    void slotGuessTagInfo();
    void slotRenameFile();

    /**
     * Reload the playlist contents from the m3u file.
     */
    void slotReload();

protected:
    /**
     * Remove \a items from the playlist and disk.  This will ignore items that
     * are not actually in the list.
     */
    void deleteFromDisk(const PlaylistItemList &items);

    virtual bool eventFilter(QObject* watched, QEvent* e);
    virtual QDragObject *dragObject(QWidget *parent);
    virtual QDragObject *dragObject() { return dragObject(this); }
    virtual bool canDecode(QMimeSource *s);
    virtual void decode(QMimeSource *s);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    virtual void showEvent(QShowEvent *e);

    /**
     * Though it's somewhat obvious, this function will stat the file, so only use it when
     * you're out of a performance critical loop.
     */
    static QString resolveSymLinks(const QFileInfo &file);

    KPopupMenu *rmbMenu() { return m_rmbMenu; }
    const KPopupMenu *rmbMenu() const { return m_rmbMenu; }

signals:
    /**
     * This signal is connected to PlaylistItem::refreshed() in the
     * PlaylistItem class.
     */
    void signalDataChanged();

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

    void signalNumberOfItemsChanged(Playlist *);

    void signalDoubleClicked();

    /**
     * This signal is emitted just before a playlist item is removed from the
     * list.
     */
    void signalAboutToRemove(PlaylistItem *item);
    void signalFilesDropped(const QStringList &files, Playlist *);
    void signalSetNext(PlaylistItem *item);
    void signalVisibleColumnsChanged();

private:
    void setup();
    void loadFile(const QString &fileName, const QFileInfo &fileInfo);
    void applyTag(QListViewItem *item, const QString &text, int column);
    int leftMostVisibleColumn() const;

    /**
     * This class is used internally to store settings that are shared by all
     * of the playlists, such as column order.  It is implemented as a singleton.
     */
    class SharedSettings;

private slots:
    void slotEmitSelected() { emit signalSelectionChanged(selectedItems()); }
    void slotEmitDoubleClicked(QListViewItem *) { emit signalDoubleClicked(); }
    void slotShowRMBMenu(QListViewItem *item, const QPoint &point, int column);
    void slotApplyModification(QListViewItem *item, const QString &text, int column);
    void slotRenameTag();
    void slotColumnOrderChanged(int, int from, int to);
    void slotToggleColumnVisible(int column);

private:
    int m_currentColumn;
    SortedStringList m_members;
    int m_processed;
    bool m_allowDuplicates;
    QPtrStack<PlaylistItem> m_history;

    QString m_fileName;

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

    PlaylistItem *m_playingItem;
    int m_leftColumn;

    PlaylistItemList m_randomList;
};

QDataStream &operator<<(QDataStream &s, const Playlist &p);
QDataStream &operator>>(QDataStream &s, Playlist &p);

#endif
