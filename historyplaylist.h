/***************************************************************************
    begin                : Fri Aug 8 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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

#ifndef HISTORYPLAYLIST_H
#define HISTORYPLAYLIST_H


#include "playlist.h"
#include "playlistitem.h"

class HistoryPlaylistItem : public PlaylistItem
{
public:
    HistoryPlaylistItem(CollectionListItem *item, Playlist *parent, QListViewItem *after);
    HistoryPlaylistItem(CollectionListItem *item, Playlist *parent);
    virtual ~HistoryPlaylistItem();

    QDateTime dateTime() const { return m_dateTime; }
    void setDateTime(const QDateTime &dt);

private:
    QDateTime m_dateTime;
};

class HistoryPlaylist : public Playlist
{
    Q_OBJECT

public:
    HistoryPlaylist(PlaylistCollection *collection);
    virtual ~HistoryPlaylist();

    virtual HistoryPlaylistItem *createItem(const FileHandle &file, QListViewItem *after = 0,
                                            bool emitChanged = true);
    virtual void createItems(const PlaylistItemList &siblings);
    virtual int columnOffset() const { return 1; }
    virtual bool readOnly() const { return true; }

    static int delay() { return 5000; }

public slots:
    void cut() {}
    void clear() {}

protected:
    virtual void polish();

private slots:
    void slotAddPlaying();
    void slotCreateNewItem();

private:
    FileHandle m_file;
    QTimer *m_timer;
};

QDataStream &operator<<(QDataStream &s, const HistoryPlaylist &p);
QDataStream &operator>>(QDataStream &s, HistoryPlaylist &p);

#endif
