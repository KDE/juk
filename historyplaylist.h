/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef JUK_HISTORYPLAYLIST_H
#define JUK_HISTORYPLAYLIST_H

#include <QDateTime>

#include "playlist.h"
#include "playlistitem.h"

class HistoryPlaylistItem : public PlaylistItem
{
public:
    HistoryPlaylistItem(CollectionListItem *item, Playlist *parent, QTreeWidgetItem *after = nullptr);

    QDateTime dateTime() const { return m_dateTime; }
    void setDateTime(const QDateTime &dt);

private:
    QDateTime m_dateTime;
};

class HistoryPlaylist : public Playlist
{
    Q_OBJECT

public:
    explicit HistoryPlaylist(PlaylistCollection *collection);
    virtual ~HistoryPlaylist();

    virtual HistoryPlaylistItem *createItem(const FileHandle &file, QTreeWidgetItem *after = nullptr);
    virtual void createItems(const PlaylistItemList &siblings);
    virtual int columnOffset() const { return 1; }
    virtual bool readOnly() const { return true; }

    static int delay() { return 5000; }

public slots:
    void cut() {}
    void clear() {}
    void appendProposedItem(const FileHandle &file);

private slots:
    void slotCreateNewItem();

private:
    using Playlist::createItems;

    FileHandle m_file;
    QTimer *m_timer;
};

QDataStream &operator<<(QDataStream &s, const HistoryPlaylist &p);
QDataStream &operator>>(QDataStream &s, HistoryPlaylist &p);

#endif

// vim: set et sw=4 tw=0 sta:
