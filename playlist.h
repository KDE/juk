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
#include <qptrstack.h>

#include "playlistitem.h"
#include "sortedstringlist.h"

class PlaylistSplitter;
class PlaylistBoxItem;
class KPopupMenu;

class KPopupMenu;
class KActionMenu;

class QEvent;

class Playlist : public KListView
{
    Q_OBJECT

public:
    /** 
     * Before creating a playlist directly, please see 
     * PlaylistSplitter::createPlaylist().
     */
    Playlist(PlaylistSplitter *s, QWidget *parent, const QString &name = QString::null);

    /** 
     * Before creating a playlist directly, please see 
     * PlaylistSplitter::openPlaylist().
     */
    Playlist(PlaylistSplitter *s, const QFileInfo &playlistFile, QWidget *parent, const char *name = 0);

    virtual ~Playlist();

    /**
     * Saves the file to the currently set file name.  If there is no filename
     * currently set, the default behavior is to prompt the user for a file
     * name.  
     */
    virtual void save();
    virtual void saveAs();
    virtual void refresh();
    virtual void clearItems(const PlaylistItemList &items);

    /** 
     * All of the (media) files in the list. 
     */
    QStringList files() const;

    /** 
     * Returns a list of all of the items in the playlist.
     */

    PlaylistItemList items() const;
    
    /**
     * Returns a list of the currently selected items.
     */
    PlaylistItemList selectedItems() const;
    
    /**
     * Remove \a items from the playlist and disk.  This will ignore items that
     * are not actually in the list.
     */
    void remove(const PlaylistItemList &items);

    /** 
     * Allow duplicate files in the playlist. 
     */
    void setAllowDuplicates(bool allow);

    /** 
     * This is being used as a mini-factory of sorts to make the construction
     * of PlaylistItems virtual.  In this case it allows for the creation of
     * both PlaylistItems and CollectionListItems.
     */
    virtual PlaylistItem *createItem(const QFileInfo &file, QListViewItem *after = 0);

    QString fileName() const { return playlistFileName; }
    void setFileName(const QString &n) { playlistFileName = n; }

    void hideColumn(int c);
    void showColumn(int c);
    bool isColumnVisible(int c) const;

    /**
     * If playlistName has no value -- i.e. the name has not been set to 
     * something other than the filename, this returns the filename less the
     * extension.  If playlistName does have a value, this returns that.
     */
    QString name() const;

    /**
     * This sets a name for the playlist that is \e different from the file name.
     */ 
    void setName(const QString &n);

    PlaylistBoxItem *playlistBoxItem() const { return boxItem; }
    void setPlaylistBoxItem(PlaylistBoxItem *item) { boxItem = item; }

    int count() const { return childCount(); }

    /** 
     * This gets the next item to be played.  This is static because often we 
     * know about the playing item, but not to which list it belongs.
     */
    PlaylistItem *nextItem(PlaylistItem *current, bool random = false);
    PlaylistItem *previousItem(PlaylistItem *current, bool random = false);

    KActionMenu *columnVisibleAction() const { return _columnVisibleAction; }

public slots:
    /**
     * Remove the currently selected items from the playlist and disk.
     */ 
    void removeSelectedItems() { remove(selectedItems()); };

    virtual void cut() { copy(); clear(); }
    virtual void copy();
    virtual void paste();
    virtual void clear();
    virtual void selectAll() { KListView::selectAll(true); }

protected:
    virtual bool eventFilter(QObject* watched, QEvent* e);
    virtual QDragObject *dragObject(QWidget *parent);
    virtual QDragObject *dragObject();
    virtual bool canDecode(QMimeSource *s);
    virtual void decode(QMimeSource *s);
    virtual void contentsDropEvent(QDropEvent *e);
    virtual void contentsDragMoveEvent(QDragMoveEvent *e);
    PlaylistSplitter *playlistSplitter() const { return splitter; }

signals:
    /** 
     * This signal is connected to PlaylistItem::refreshed() in the 
     * PlaylistItem class. 
     */
    void dataChanged();
    /** 
     * This signal is emitted when items are added to the collection list.  
     * This happens in the createItem() method when items are added to the 
     * collection. 
     */
    void collectionChanged();

    /** 
     * This is emitted when the playlist selection is changed.  This is used
     * primarily to notify the TagEditor of the new data. 
     */
    void selectionChanged(const PlaylistItemList &selection);
    
    /**
     * This is connected to the PlaylistBoxItem to let it know when the 
     * playlist's name has changed.
     */
    void nameChanged(const QString &fileName);
    
    void numberOfItemsChanged(Playlist *);
    
    void doubleClicked();

    /**
     * This signal is emitted just before a playlist item is removed from the 
     * list.
     */
    void aboutToRemove(PlaylistItem *item);

    void signalToggleColumnVisible(int column);

private:
    void setup();
    QPtrStack<PlaylistItem> history;

private slots:
    void emitSelected() {  emit(selectionChanged(selectedItems())); }
    void emitDoubleClicked(QListViewItem *) { emit(doubleClicked()); }
    void showRMBMenu(QListViewItem *item, const QPoint &point, int column);
    void applyTags(QListViewItem *item, const QString &text, int column);
    void renameTag();

private:
    int currentColumn;
    SortedStringList members;
    int processed;
    bool allowDuplicates;

    QString playlistFileName;

    /**
     * This is only defined if the playlist name is something other than the
     * file name.
     */
    QString playlistName;
    PlaylistSplitter *splitter;
    PlaylistBoxItem *boxItem;
   
    KPopupMenu *rmbMenu;
    KPopupMenu *headerMenu;
    KActionMenu *_columnVisibleAction;

    int rmbPasteID;
    int rmbEditID;
};

QDataStream &operator<<(QDataStream &s, const Playlist &p);
QDataStream &operator>>(QDataStream &s, Playlist &p);

#endif
