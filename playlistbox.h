/***************************************************************************
                          playlistbox.h  -  description
                             -------------------
    begin                : Thu Sep 12 2002
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

#ifndef PLAYLISTBOX_H
#define PLAYLISTBOX_H

#include <klistbox.h>

#include <qwidgetstack.h>
#include <qptrdict.h>
#include <qmap.h>

#include "playlist.h"
#include "listboxpixmap.h"

class PlaylistItem;
class PlaylistSplitter;

class KPopupMenu;

/** 
 * This is the play list selection box that is by default on the right side of
 * JuK's main widget (PlaylistSplitter). 
 */

class PlaylistBox : public KListBox
{
    Q_OBJECT

public: 
    PlaylistBox(PlaylistSplitter *parent = 0, const char *name = 0);
    virtual ~PlaylistBox();

    void createItem(Playlist *playlist, const char *icon = 0, bool raise = false);

    void sort();
    void raise(Playlist *playlist);
    QStringList names() const { return m_names; }
    PlaylistList playlists() const;

    // All of the methods use the selected item.
    void save();
    void saveAs();
    void rename();
    void duplicate();
    void deleteItem();

    class Item;
    friend class Item;

public slots:
    void paste();
    void clear() {} // override the (destructive) default

signals:
    void signalCurrentChanged(const PlaylistList &);
    void signalDoubleClicked();

private:
    void save(Item *item);
    void saveAs(Item *item);
    void rename(Item *item);
    void duplicate(Item *item);
    void deleteItem(Item *item);
    void reload(Item *item);

    virtual void resizeEvent(QResizeEvent *e);
    virtual void decode(QMimeSource *s, Item *item);
    virtual void dropEvent(QDropEvent *e);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    /** 
     * This is used by PlaylistItemBox (a friend class) to add names to the name
     * list returned by names(). 
     */
    void addName(const QString &name) { m_names.append(name); }

    QValueList<Item *> selectedItems() const;

private slots:
    /** 
     * Catches QListBox::currentChanged(QListBoxItem *), does a cast and then re-emits
     * the signal as currentChanged(Item *). 
     */
    void slotPlaylistChanged(QListBoxItem *);
    void slotDoubleClicked(QListBoxItem *);
    void slotShowContextMenu(QListBoxItem *item, const QPoint &point);

    // context menu entries
    void slotContextSave();
    void slotContextSaveAs();
    void slotContextRename();
    void slotContextDuplicate();
    void slotContextDeleteItem();
    void slotContextReload();

private:
    PlaylistSplitter *m_splitter;
    QStringList m_names;
    KPopupMenu *m_playlistContextMenu;
    Item *m_contextMenuOn;
    bool m_updatePlaylistStack;
    QPtrDict<Item> m_playlistDict;
    QMap<QString, int> m_popupIndex;
};



class PlaylistBox::Item : public QObject, public ListBoxPixmap
{
    friend class PlaylistBox;

    Q_OBJECT

    // moc won't let me create private QObject subclasses and Qt won't let me
    // make the destructor protected, so here's the closest hack that will
    // compile.

public:
    virtual ~Item();

protected:
    Item(PlaylistBox *listbox, const QPixmap &pix, const QString &text, Playlist *l = 0);
    Item(PlaylistBox *listbox, const QString &text, Playlist *l = 0);

    Playlist *playlist() const { return m_list; }
    PlaylistBox *listBox() const { return static_cast<PlaylistBox *>(ListBoxPixmap::listBox()); }

public slots:
    void slotSetName(const QString &name);
    
private:
    Playlist *m_list;
};

#endif
